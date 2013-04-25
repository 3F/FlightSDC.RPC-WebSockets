/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "version.h"
#include "Client.h"

#include "BufferedSocket.h"

#include "FavoriteManager.h"
#include "TimerManager.h"
#include "ResourceManager.h"
#include "ClientManager.h"
#include "LogManager.h"

namespace dcpp
{

boost::atomic<long> Client::counts[COUNT_UNCOUNTED];

Client::Client(const string& hubURL, char separator_, bool secure_) :
	myIdentity(ClientManager::getInstance()->getMe(), 0),
	reconnDelay(120), lastActivity(GET_TICK()), registered(false), autoReconnect(false),
	m_last_encoding(Text::systemCharset), state(STATE_DISCONNECTED), sock(0),
	hubUrl(hubURL), port(0), separator(separator_),
	secure(secure_), countType(COUNT_UNCOUNTED), availableBytes(0)
{
	string file, proto, query, fragment;
	Util::decodeUrl(hubURL, proto, address, port, file, query, fragment);
	if (!query.empty())
	{
		keyprint = Util::decodeQuery(query)["kp"];
#ifdef _DEBUG
		LogManager::getInstance()->message("keyprint = " + keyprint);
#endif
	}
#ifdef _DEBUG
	else
		LogManager::getInstance()->message("hubURL = " + hubURL + " query.empty()");
#endif
	TimerManager::getInstance()->addListener(this);
}

Client::~Client()
{
	dcassert(!sock);
	if (sock)
		LogManager::getInstance()->message("[Error] Client::~Client() sock == nullptr");
	// In case we were deleted before we Failed
	FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
	TimerManager::getInstance()->removeListener(this);
	updateCounts(true);
}

void Client::reconnect()
{
	disconnect(true);
	setAutoReconnect(true);
	setReconnDelay(0);
}

void Client::shutdown()
{
	state = STATE_DISCONNECTED; // [+]
	if (sock)
	{
		sock->removeListeners(); //[+] brain-ripper
		BufferedSocket::putSocket(sock);
		sock = 0;
	}
}

void Client::reloadSettings(bool updateNick)
{
	const FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(getHubUrl());
	// !SMT!-S
	string ClientId;
	const bool l_isADC = _strnicmp("adc://", getHubUrl().c_str(), 6) == 0 || _strnicmp("adcs://", getHubUrl().c_str(), 7) == 0;
	if (hub && hub->getOverrideId())
	{
		ClientId = hub->getClientId(); // !SMT!-S
	}
	else if (hub && hub->getStealth())
	{
		ClientId =  "++ ";
		ClientId += l_isADC ? "" : "V:";
		ClientId += DCVERSIONSTRING;
	}
	else
	{
		ClientId =  "SSQLite++ ";
		ClientId += (l_isADC ? "" : "V:");
		ClientId += "2.43." REVISION_STR;
		if (g_isWineCheck)
			ClientId += "-wine";
	}
	
	if (hub)
	{
	
		if (updateNick)
		{
			setCurrentNick(checkNick(hub->getNick(true)));
		}
		
		if (!hub->getUserDescription().empty())
		{
			setCurrentDescription(hub->getUserDescription());
		}
		else
		{
			setCurrentDescription(SETTING(DESCRIPTION));
		}
		if (!hub->getPassword().empty())
			setPassword(hub->getPassword());
		setStealth(hub->getStealth());
		setFavIp(hub->getIP());
		
		if (!hub->getEncoding().empty())
			setEncoding(hub->getEncoding());
			
		if (hub->getSearchInterval() < 10)
			setSearchInterval(SETTING(MINIMUM_SEARCH_INTERVAL) * 1000);
		else
			setSearchInterval(hub->getSearchInterval() * 1000);
	}
	else
	{
		if (updateNick)
		{
			setCurrentNick(checkNick(SETTING(NICK)));
		}
		setCurrentDescription(SETTING(DESCRIPTION));
		setStealth(false);
		setFavIp(Util::emptyString);
		setSearchInterval(SETTING(MINIMUM_SEARCH_INTERVAL) * 1000);
	}
	// !SMT!-S
	for (string::size_type i = 0; i < ClientId.length(); i++)
		if (ClientId[i] == '<' || ClientId[i] == '>' || ClientId[i] == ',' || ClientId[i] == '$' || ClientId[i] == '|')
		{
			ClientId = ClientId.substr(0, i);
			break;
		}
	setClientId(ClientId);
}

bool Client::isActive() const
{
	return ClientManager::getInstance()->isActive(hubUrl);
}

void Client::connect()
{
	if (sock)
		BufferedSocket::putSocket(sock);
		
	availableBytes = 0;
	
	setAutoReconnect(true);
	setReconnDelay(120 + Util::rand(0, 60));
	reloadSettings(true);
	setRegistered(false);
	setMyIdentity(Identity(ClientManager::getInstance()->getMe(), 0));
	setHubIdentity(Identity());
	
	state = STATE_CONNECTING;
	
	try
	{
		sock = BufferedSocket::getSocket(separator);
		sock->addListener(this);
		sock->connect(address, port, secure, BOOLSETTING(ALLOW_UNTRUSTED_HUBS), true);
	}
	catch (const Exception& e)
	{
		shutdown();
		/// @todo at this point, this hub instance is completely useless
		fire(ClientListener::Failed(), this, e.getError());
	}
	updateActivity();
}

void Client::send(const char* aMessage, size_t aLen)
{
	if (!isReady())
	{
		dcassert(0);
		fire(ClientListener::Failed(), this, "Client::send - !isReady()");//[+] IRainman
		return;
	}
	updateActivity();
	sock->write(aMessage, aLen);
	COMMAND_DEBUG(aMessage, DebugManager::HUB_OUT, getIpPort());
}

void Client::on(Connected) noexcept
{
	updateActivity();
	ip = sock->getIp();
	localIp = sock->getLocalIp();
	
	if (sock->isSecure() && keyprint.compare(0, 7, "SHA256/") == 0)
	{
		auto kp = sock->getKeyprint();
		if (!kp.empty())
		{
			vector<uint8_t> kp2v(kp.size());
			Encoder::fromBase32(keyprint.c_str() + 7, &kp2v[0], kp2v.size());
			if (!std::equal(kp.begin(), kp.end(), kp2v.begin()))
			{
				state = STATE_DISCONNECTED;
				sock->removeListener(this);
				fire(ClientListener::Failed(), this, "Keyprint mismatch");
				return;
			}
		}
	}
	
	fire(ClientListener::Connected(), this);
	state = STATE_PROTOCOL;
}

void Client::on(Failed, const string& aLine) noexcept
{
	state = STATE_DISCONNECTED;
	FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
	sock->removeListener(this);
	fire(ClientListener::Failed(), this, aLine);
}

void Client::disconnect(bool graceLess)
{
	if (sock)
		sock->disconnect(graceLess);
}

bool Client::isSecure() const
{
	return isReady() && sock->isSecure();
}

bool Client::isTrusted() const
{
	return isReady() && sock->isTrusted();
}

std::string Client::getCipherName() const
{
	return isReady() ? sock->getCipherName() : Util::emptyString;
}

vector<uint8_t> Client::getKeyprint() const
{
	return isReady() ? sock->getKeyprint() : vector<uint8_t>();
}

void Client::updateCounts(bool aRemove)
{
	// We always remove the count and then add the correct one if requested...
	if (countType != COUNT_UNCOUNTED)
	{
		--counts[countType];
		countType = COUNT_UNCOUNTED;
	}
	
	if (!aRemove)
	{
		if (getMyIdentity().isOp())
		{
			countType = COUNT_OP;
		}
		else if (getMyIdentity().isRegistered())
		{
			countType = COUNT_REGISTERED;
		}
		else
		{
			countType = COUNT_NORMAL;
		}
		++counts[countType];
	}
}

string Client::getLocalIp() const
{
	// Favorite hub Ip
	if (!getFavIp().empty())
		return Socket::resolve(getFavIp());
		
	// Best case - the server detected it
	if ((!BOOLSETTING(NO_IP_OVERRIDE) || SETTING(EXTERNAL_IP).empty()) && !getMyIdentity().getIp().empty())
	{
		return getMyIdentity().getIp();
	}
	
	if (!SETTING(EXTERNAL_IP).empty())
	{
		return Socket::resolve(SETTING(EXTERNAL_IP));
	}
	
	if (localIp.empty())
	{
		return Util::getLocalIp();
	}
	
	return localIp;
}

uint64_t Client::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList, void* owner)
{
	dcdebug("Queue search %s\n", aString.c_str());
	
	if (searchQueue.interval)
	{
		Search s;
		s.fileType = aFileType;
		s.size     = aSize;
		s.query    = aString;
		s.sizeType = aSizeMode;
		s.token    = aToken;
		s.exts     = aExtList;
		s.owners.insert(owner);
		
		searchQueue.add(s);
		
		return searchQueue.getSearchTime(owner) - GET_TICK();
	}
	
	search(aSizeMode, aSize, aFileType , aString, aToken, aExtList);
	return 0;
	
}

void Client::on(Line, const string& aLine) noexcept
{
	updateActivity();
	COMMAND_DEBUG(aLine, DebugManager::HUB_IN, getIpPort());
}

void Client::on(Second, uint64_t aTick) noexcept
{
	if (state == STATE_DISCONNECTED && getAutoReconnect() && (aTick > (getLastActivity() + getReconnDelay() * 1000)))
	{
		// Try to reconnect...
		connect();
	}
	
	if (!searchQueue.interval) return;
	
	if (isConnected())
	{
		Search s;
		
		if (searchQueue.pop(s))
		{
			search(s.sizeType, s.size, s.fileType , s.query, s.token, s.exts);
		}
	}
}

} // namespace dcpp

/**
 * @file
 * $Id: Client.cpp 568 2011-07-24 18:28:43Z bigmuscle $
 */
