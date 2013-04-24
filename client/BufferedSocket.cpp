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
#include "BufferedSocket.h"
#include "ResourceManager.h"
#include "TimerManager.h"
#include "SettingsManager.h"
#include "Streams.h"
#include "SSLSocket.h"
#include "CryptoManager.h"
#include "ZUtils.h"
#include "ThrottleManager.h"
#include "LogManager.h"

namespace dcpp
{

// Polling is used for tasks...should be fixed...
static const uint64_t POLL_TIMEOUT = 250;

BufferedSocket::BufferedSocket(char aSeparator) :
	separator(aSeparator), mode(MODE_LINE), dataBytes(0), rollback(0), state(STARTING),
	disconnecting(false)
{
	start(); // [!] IRainamn fix: ThreadID is set in thread. Please not set here.
	
	++sockets;
}

boost::atomic<long> BufferedSocket::sockets(0);

BufferedSocket::~BufferedSocket()
{
	--sockets;
}

void BufferedSocket::setMode(Modes aMode, size_t aRollback)
{
	if (mode == aMode)
	{
		dcdebug("WARNING: Re-entering mode %d\n", mode);
		return;
	}
	switch (aMode)
	{
		case MODE_LINE:
			rollback = aRollback;
			break;
		case MODE_ZPIPE:
			filterIn = std::unique_ptr<UnZFilter>(new UnZFilter);
			break;
		case MODE_DATA:
			break;
	}
	mode = aMode;
}

void BufferedSocket::setSocket(std::unique_ptr<Socket>& s)
{
	if (sock.get())
	{
		LogManager::getInstance()->message("BufferedSocket::setSocket - dcassert(!sock.get())");
	}
	dcassert(!sock.get());
	sock = move(s);
}

void BufferedSocket::setOptions()
{
#ifdef FLYLINKDC_BUG_WIN8_DP
	if (SETTING(SOCKET_IN_BUFFER) > 0)
		sock->setSocketOpt(SO_RCVBUF, SETTING(SOCKET_IN_BUFFER));
#endif
	if (SETTING(SOCKET_OUT_BUFFER) > 0)
		sock->setSocketOpt(SO_SNDBUF, SETTING(SOCKET_OUT_BUFFER));
}

void BufferedSocket::accept(const Socket& srv, bool secure, bool allowUntrusted)
{
	dcdebug("BufferedSocket::accept() %p\n", (void*)this);
	
	std::unique_ptr<Socket> s(secure ? CryptoManager::getInstance()->getServerSocket(allowUntrusted) : new Socket);
	
	s->accept(srv);
	
	setSocket(s); // [!] IRainman fix setSocket(move(s));
	setOptions();
	
	Lock l(cs);
	addTask(ACCEPTED, nullptr);
}

void BufferedSocket::connect(const string& aAddress, uint16_t aPort, bool secure, bool allowUntrusted, bool proxy)
{
	connect(aAddress, aPort, 0, NAT_NONE, secure, allowUntrusted, proxy);
}

void BufferedSocket::connect(const string& aAddress, uint16_t aPort, uint16_t localPort, NatRoles natRole, bool secure, bool allowUntrusted, bool proxy)
{
	dcdebug("BufferedSocket::connect() %p\n", (void*)this);
	std::unique_ptr<Socket> s(secure ? (natRole == NAT_SERVER ? CryptoManager::getInstance()->getServerSocket(allowUntrusted) : CryptoManager::getInstance()->getClientSocket(allowUntrusted)) : new Socket);
	
	s->create();
	setSocket(s); // [!] IRainman fix setSocket(move(s));
	sock->bind(localPort, Socket::getBindAddress()); //[!]
	// 2012-05-13_06-33-00_EU5MQ7O5OZE5BX5DMZLIDQK3EVWCIHALIZKXW2A_F2EBE18F_crash-strong-stack-r9957.dmp
	// 2012-05-23_22-40-43_R37OYYA4ZJUN3U2ULIIIYZXSFYL6CLT45GZ54RA_4F9B4AAC_crash-strong-stack-r10096-x64.dmp
	// 2012-05-23_22-40-43_VGXV62VKTPFKHTWIDG6YRET3IHKRY3PRAW6HKNI_5D005D81_crash-strong-stack-r10096-x64.dmp
	// 2012-05-23_22-40-43_WQRN3ZPMMQHK7UIOUHH6NTME7M75QXTU5UZW5FA_B2E86D23_crash-strong-stack-r10096-x64.dmp
	Lock l(cs);
	addTask(CONNECT, new ConnectInfo(aAddress, aPort, localPort, natRole, proxy && (SETTING(OUTGOING_CONNECTIONS) == SettingsManager::OUTGOING_SOCKS5)));
}

static const uint64_t LONG_TIMEOUT = 30000;
static const uint64_t SHORT_TIMEOUT = 1000;
void BufferedSocket::threadConnect(const string& aAddr, uint16_t aPort, uint16_t localPort, NatRoles natRole, bool proxy)
{
	dcassert(state == STARTING);
	
	dcdebug("threadConnect %s:%d/%d\n", aAddr.c_str(), (int)localPort, (int)aPort);
	fire(BufferedSocketListener::Connecting());
	
	const uint64_t endTime = GET_TICK() + LONG_TIMEOUT;
	state = RUNNING;
	
	while (hasSocket() &&
	        GET_TICK() < endTime)
	{
		dcdebug("threadConnect attempt to addr \"%s\"\n", aAddr.c_str());
		try
		{
			if (proxy)
			{
				sock->socksConnect(aAddr, aPort, LONG_TIMEOUT);
			}
			else
			{
				sock->connect(aAddr, aPort);
			}
			
			setOptions();
			
			bool connSucceeded;
			while (!(connSucceeded = sock->waitConnected(POLL_TIMEOUT)) && endTime >= GET_TICK())
			{
				if (socketIsDisconecting()) return;
			}
			
			if (connSucceeded)
			{
				inbuf.resize(sock->getSocketOptInt(SO_RCVBUF));
				
				fire(BufferedSocketListener::Connected());
				return;
			}
		}
		catch (const SSLSocketException&)
		{
			throw;
		}
		catch (const SocketException&)
		{
			if (natRole == NAT_NONE)
				throw;
			Thread::sleep(SHORT_TIMEOUT);
		}
	}
	
	throw SocketException(STRING(CONNECTION_TIMEOUT));
}

void BufferedSocket::threadAccept()
{
	dcassert(state == STARTING);
	
	dcdebug("threadAccept\n");
	
	state = RUNNING;
	
	inbuf.resize(sock->getSocketOptInt(SO_RCVBUF));
	
	const uint64_t startTime = GET_TICK();
	while (!sock->waitAccepted(POLL_TIMEOUT))
	{
		if (socketIsDisconecting())
			return;
			
		if ((startTime + LONG_TIMEOUT) < GET_TICK())
		{
			throw SocketException(STRING(CONNECTION_TIMEOUT));
		}
	}
}

void BufferedSocket::threadRead()
{
	if (state != RUNNING)
		return;
		
	int left = (mode == MODE_DATA) ? ThrottleManager::getInstance()->read(sock.get(), &inbuf[0], (int)inbuf.size()) : sock->read(&inbuf[0], (int)inbuf.size());
	if (left == -1)
	{
		// EWOULDBLOCK, no data received...
		return;
	}
	else if (left == 0)
	{
		// This socket has been closed...
		throw SocketException(STRING(CONNECTION_CLOSED));
	}
	
	string::size_type pos = 0;
	// always uncompressed data
	string l;
	int bufpos = 0, total = left;
	
	while (left > 0)
	{
		switch (mode)
		{
			case MODE_ZPIPE:
			{
				const int BUF_SIZE = 1024;
				// Special to autodetect nmdc connections...
				string::size_type pos = 0; //  warning C6246: Local declaration of 'pos' hides declaration of the same name in outer scope.
				std::unique_ptr<char[]> buffer(new char[BUF_SIZE]);
				l = line;
				// decompress all input data and store in l.
				while (left)
				{
					size_t in = BUF_SIZE;
					size_t used = left;
					bool ret = (*filterIn)(&inbuf[0] + total - left, used, &buffer[0], in);
					left -= used;
					l.append(&buffer[0], in);
					// if the stream ends before the data runs out, keep remainder of data in inbuf
					if (!ret)
					{
						bufpos = total - left;
						setMode(MODE_LINE, rollback);
						break;
					}
				}
				// process all lines
				while ((pos = l.find(separator)) != string::npos)
				{
					if (pos > 0) // check empty (only pipe) command and don't waste cpu with it ;o)
						fire(BufferedSocketListener::Line(), l.substr(0, pos));
					l.erase(0, pos + 1 /* separator char */);
				}
				// store remainder
				line = l;
				
				break;
			}
			case MODE_LINE:
				// Special to autodetect nmdc connections...
				if (separator == 0)
				{
					if (inbuf[0] == '$')
					{
						separator = '|';
					}
					else
					{
						separator = '\n';
					}
				}
				l = line + string((char*)&inbuf[bufpos], left);
				while ((pos = l.find(separator)) != string::npos)
				{
					if (pos > 0) // check empty (only pipe) command and don't waste cpu with it ;o)
						fire(BufferedSocketListener::Line(), l.substr(0, pos));
					l.erase(0, pos + 1 /* separator char */);
					if (l.length() < (size_t)left) left = l.length();
					if (mode != MODE_LINE)
					{
						// we changed mode; remainder of l is invalid.
						l.clear();
						bufpos = total - left;
						break;
					}
				}
				if (pos == string::npos)
					left = 0;
				line = l;
				break;
			case MODE_DATA:
				while (left > 0)
				{
					if (dataBytes == -1)
					{
						fire(BufferedSocketListener::Data(), &inbuf[bufpos], left);
						bufpos += (left - rollback);
						left = rollback;
						rollback = 0;
					}
					else
					{
						const int high = (int)min(dataBytes, (int64_t)left);
						dcassert(high != 0);
						if (high != 0) // [+] IRainman fix.
						{
							fire(BufferedSocketListener::Data(), &inbuf[bufpos], high);
							bufpos += high;
							left -= high;
							
							dataBytes -= high;
						}
						if (dataBytes == 0)
						{
							mode = MODE_LINE;
							fire(BufferedSocketListener::ModeChange());
						}
					}
				}
				break;
		}
	}
	
	if (mode == MODE_LINE && line.size() > 16777216)
	{
		throw SocketException(STRING(COMMAND_TOO_LONG));
	}
}

void BufferedSocket::threadSendFile(InputStream* file)
{
	if (state != RUNNING)
		return;
		
	if (socketIsDisconecting())
		return;
	dcassert(file != NULL);
	size_t sockSize = (size_t)sock->getSocketOptInt(SO_SNDBUF);
	size_t bufSize = max(sockSize, (size_t)64 * 1024);
	
	ByteVector readBuf(bufSize);
	ByteVector writeBuf(bufSize);
	
	size_t readPos = 0;
	
	bool readDone = false;
	dcdebug("Starting threadSend\n");
	while (!socketIsDisconecting())
	{
		if (!readDone && readBuf.size() > readPos)
		{
			// Fill read buffer
			size_t bytesRead = readBuf.size() - readPos;
			size_t actual = file->read(&readBuf[readPos], bytesRead);
			
			if (actual > 0)
			{
				fire(BufferedSocketListener::BytesSent(), bytesRead, actual);
			}
			
			if (actual == 0)
			{
				readDone = true;
			}
			else
			{
				readPos += actual;
			}
		}
		
		if (readDone && readPos == 0)
		{
			fire(BufferedSocketListener::TransmitDone());
			return;
		}
		
		readBuf.swap(writeBuf);
		readBuf.resize(bufSize);
		writeBuf.resize(readPos);
		readPos = 0;
		
		size_t writePos = 0, writeSize = 0;
		int written = 0;
		
		while (writePos < writeBuf.size())
		{
			if (socketIsDisconecting())
				return;
				
			if (written == -1)
			{
				// workaround for OpenSSL (crashes when previous write failed and now retrying with different writeSize)
				written = sock->write(&writeBuf[writePos], writeSize);
			}
			else
			{
				writeSize = min(sockSize / 2, writeBuf.size() - writePos);
				written = ThrottleManager::getInstance()->write(sock.get(), &writeBuf[writePos], writeSize);
			}
			
			if (written > 0)
			{
				writePos += written;
				
				fire(BufferedSocketListener::BytesSent(), 0, written);
				
			}
			else if (written == -1)
			{
				if (!readDone && readPos < readBuf.size())
				{
					// Read a little since we're blocking anyway...
					size_t bytesRead = min(readBuf.size() - readPos, readBuf.size() / 2);
					size_t actual = file->read(&readBuf[readPos], bytesRead);
					
					if (bytesRead > 0)
					{
						fire(BufferedSocketListener::BytesSent(), bytesRead, actual);
					}
					
					if (actual == 0)
					{
						readDone = true;
					}
					else
					{
						readPos += actual;
					}
				}
				else
				{
					while (!socketIsDisconecting()) // [!] IRainman fix
					{
						const int w = sock->wait(POLL_TIMEOUT, Socket::WAIT_WRITE | Socket::WAIT_READ);
						if (w & Socket::WAIT_READ)
						{
							threadRead();
						}
						if (w & Socket::WAIT_WRITE)
						{
							break;
						}
					}
				}
			}
		}
	}
}

void BufferedSocket::write(const char* aBuf, size_t aLen) noexcept
{
	if (!hasSocket())
		return;
	Lock l(cs);
	if (writeBuf.empty())
		addTask(SEND_DATA, nullptr);
		
	writeBuf.insert(writeBuf.end(), aBuf, aBuf + aLen);
}

void BufferedSocket::threadSendData()
{
	if (state != RUNNING)
		return;
		
	{
		Lock l(cs);
		if (writeBuf.empty())
			return;
			
		writeBuf.swap(sendBuf);
	}
	
	size_t left = sendBuf.size();
	size_t done = 0;
	while (left > 0)
	{
		if (socketIsDisconecting())
		{
			return;
		}
		
		int w = sock->wait(POLL_TIMEOUT, Socket::WAIT_READ | Socket::WAIT_WRITE);
		
		if (w & Socket::WAIT_READ)
		{
			threadRead();
		}
		
		if (w & Socket::WAIT_WRITE)
		{
			int n = sock->write(&sendBuf[done], left);
			if (n > 0)
			{
				left -= n;
				done += n;
			}
		}
	}
	sendBuf.clear();
}

bool BufferedSocket::checkEvents()
{
	while (state == RUNNING ? taskSem.wait(0) : taskSem.wait())
	{
		pair<Tasks, TaskData* > p;
		{
			Lock l(cs);
			m_speaking = true; // [+] IRainman fix.
			dcassert(!tasks.empty());
			if (!tasks.empty())
			{
				p = tasks.front();
				tasks.pop_front(); // [!] IRainman opt.
			}
			else
			{
				m_speaking = false; // [+] IRainman fix.
				return false;
			}
		}
		std::unique_ptr<TaskData> l_task_destroy;
		if (p.second)
			l_task_destroy.reset(p.second);
			
		/* [-] IRainman fix.
		if (p.first == SHUTDOWN)
		{
		    return false;
		}
		else
		   [-] */
		// [!]
		if (state == RUNNING)
		{
			if (p.first == UPDATED)
			{
				fire(BufferedSocketListener::Updated());
				continue;
			}
			else if (p.first == SEND_DATA)
			{
				threadSendData();
			}
			else if (p.first == SEND_FILE)
			{
				threadSendFile(static_cast<SendFileInfo*>(p.second)->stream);
				break;
			}
			else if (p.first == DISCONNECT)
			{
				fail(STRING(DISCONNECTED));
			}
			else
			{
				dcdebug("%d unexpected in RUNNING state\n", p.first);
			}
		}
		else if (state == STARTING)
		{
			if (p.first == CONNECT)
			{
				ConnectInfo* ci = static_cast<ConnectInfo*>(p.second);
				threadConnect(ci->addr, ci->port, ci->localPort, ci->natRole, ci->proxy);
			}
			else if (p.first == ACCEPTED)
			{
				threadAccept();
			}
			// [+]
			else if (p.first == SHUTDOWN)
			{
				return false;
			}
			// [~]
			else
			{
				dcdebug("%d unexpected in STARTING state\n", p.first);
			}
		}
		m_speaking = false; // [+]
		// [~] IRainman fix.
	}
	return true;
}

void BufferedSocket::checkSocket()
{
	if (hasSocket())
	{
		int waitFor = sock->wait(POLL_TIMEOUT, Socket::WAIT_READ);
		if (waitFor & Socket::WAIT_READ)
		{
			threadRead();
		}
	}
}

/**
 * Main task dispatcher for the buffered socket abstraction.
 * @todo Fix the polling...
 */
int BufferedSocket::run()
{
	m_threadId = GetSelfThreadID(); // [+] IRainman fix.
	dcdebug("BufferedSocket::run() start %p\n", (void*)this);
	while (true)
	{
		try
		{
			if (!checkEvents())
			{
				break;
			}
			if (state == RUNNING)
			{
				checkSocket();
			}
		}
		catch (const Exception& e)
		{
			m_speaking = false; // [+] IRainman fix.
			fail(e.getError());
		}
	}
	dcdebug("BufferedSocket::run() end %p\n", (void*)this);
	delete this;// Cleanup the thread object
	return 0;
}

void BufferedSocket::fail(const string& aError)
{
	if (hasSocket())
		sock->disconnect();
		
	if (state == RUNNING) // https://www.box.net/shared/t9lf1bud4zylon646185
	{
		state = FAILED;
		fire(BufferedSocketListener::Failed(), aError);
	}
}

void BufferedSocket::shutdown()
{
	Lock l(cs);
	if (m_threadId != GetSelfThreadID())
	{
		while (m_speaking)
		{
			Thread::sleep(1); // [+] We are waiting for the full processing of the current event.
		}
	}
	state = STARTING; // [+]
	disconnecting = true;
	addTask(SHUTDOWN, nullptr);
}

void BufferedSocket::addTask(Tasks task, TaskData* data)
{
	dcassert(task == DISCONNECT || task == SHUTDOWN || task == UPDATED || sock.get());
	tasks.push_back(make_pair(task, data));
	taskSem.signal();
}

} // namespace dcpp

/**
 * @file
 * $Id: BufferedSocket.cpp 582 2011-12-18 10:00:11Z bigmuscle $
 */
