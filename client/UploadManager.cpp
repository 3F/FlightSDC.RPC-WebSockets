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
#include "UploadManager.h"

#include <cmath>

#include "ConnectionManager.h"
#include "LogManager.h"
#include "ShareManager.h"
#include "ClientManager.h"
#include "FilteredFile.h"
#include "ZUtils.h"
#include "ResourceManager.h"
#include "HashManager.h"
#include "AdcCommand.h"
#include "FavoriteManager.h"
#include "CryptoManager.h"
#include "Upload.h"
#include "UserConnection.h"
#include "QueueManager.h"
#include "FinishedManager.h"
#include "PGLoader.h"
#include "SharedFileStream.h"

namespace dcpp
{

static const string UPLOAD_AREA = "Uploads";

UploadManager::UploadManager() noexcept :
running(0), extra(0), lastGrant(0), lastFreeSlots(-1),
        fireballStartTick(0), isFireball(false), isFileServer(false), extraPartial(0)
{
	ClientManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
}

UploadManager::~UploadManager()
{
	TimerManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);
	{
		Lock l(cs);
		for (auto ii = uploadQueue.cbegin(); ii != uploadQueue.cend(); ++ii)
		{
			for (auto i = ii->files.cbegin(); i != ii->files.cend(); ++i)
			{
				(*i)->dec();
			}
		}
		uploadQueue.clear();
	}
	
	while (true)
	{
		{
			Lock l(cs);
			if (uploads.empty())
				break;
		}
		Thread::sleep(100);
	}
}
bool UploadManager::hasUpload(UserConnection& aSource)
{
	if (aSource.getSocket())
	{
		const string l_srcip = aSource.getSocket()->getIp();
		const int64_t l_share = ClientManager::getInstance()->getBytesShared(aSource.getUser());
		for (UploadList::const_iterator i = uploads.begin(); i != uploads.end(); ++i)
		{
			Upload* u = *i;
			if (u &&
			        u->getUserConnection().getSocket() &&
			        l_srcip == u->getUserConnection().getSocket()->getIp() &&
			        u->getUser() &&
			        l_share == ClientManager::getInstance()->getBytesShared(u->getUser())
			   )
			{
#ifdef _DEBUG
				LogManager::getInstance()->message(string("drop IP = ") + l_srcip);
#endif
				return true;
			}
		}
	}
	return false;
}
bool UploadManager::prepareFile(UserConnection& aSource, const string& aType, const string& aFile, int64_t aStartPos, int64_t& aBytes, bool listRecursive)
{
	dcdebug("Preparing %s %s " I64_FMT " " I64_FMT " %d\n", aType.c_str(), aFile.c_str(), aStartPos, aBytes, listRecursive);
	
	if (aFile.empty() || aStartPos < 0 || aBytes < -1 || aBytes == 0)
	{
		aSource.fileNotAvail("Invalid request");
		return false;
	}
	
	InputStream* is = 0;
	int64_t start = 0;
	int64_t size = 0;
	int64_t fileSize = 0;
	
	bool userlist = (aFile == Transfer::USER_LIST_NAME_BZ || aFile == Transfer::USER_LIST_NAME);
	bool free = userlist;
	bool partial = false;
	
	string sourceFile;
	Transfer::Type type;
	
	try
	{
		if (aType == Transfer::names[Transfer::TYPE_FILE])
		{
			sourceFile = ShareManager::getInstance()->toReal(aFile);
			
			if (aFile == Transfer::USER_LIST_NAME)
			{
				// Unpack before sending...
				string bz2 = File(sourceFile, File::READ, File::OPEN).read();
				string xml;
				CryptoManager::getInstance()->decodeBZ2(reinterpret_cast<const uint8_t*>(bz2.data()), bz2.size(), xml);
				// Clear to save some memory...
				string().swap(bz2);
				is = new MemoryInputStream(xml);
				start = 0;
				fileSize = size = xml.size();
			}
			else
			{
				File* f = new File(sourceFile, File::READ, File::OPEN);
				
				start = aStartPos;
				int64_t sz = f->getSize();
				size = (aBytes == -1) ? sz - start : aBytes;
				fileSize = sz;
				
				if ((start + size) > sz)
				{
					aSource.fileNotAvail();
					delete f;
					return false;
				}
				
				free = free || (sz <= (int64_t)(SETTING(SET_MINISLOT_SIZE) * 1024));
				
				f->setPos(start);
				is = f;
				if ((start + size) < sz)
				{
					is = new LimitedInputStream(is, size);
				}
			}
			type = userlist ? Transfer::TYPE_FULL_LIST : Transfer::TYPE_FILE;
		}
		else if (aType == Transfer::names[Transfer::TYPE_TREE])
		{
			//sourceFile = ShareManager::getInstance()->toReal(aFile);
			sourceFile = aFile;
			MemoryInputStream* mis = ShareManager::getInstance()->getTree(aFile);
			if (!mis)
			{
				aSource.fileNotAvail();
				return false;
			}
			
			start = 0;
			fileSize = size = mis->getSize();
			is = mis;
			free = true;
			type = Transfer::TYPE_TREE;
		}
		else if (aType == Transfer::names[Transfer::TYPE_PARTIAL_LIST])
		{
			// Partial file list
			MemoryInputStream* mis = ShareManager::getInstance()->generatePartialList(aFile, listRecursive);
			if (mis == NULL)
			{
				aSource.fileNotAvail();
				return false;
			}
			
			start = 0;
			fileSize = size = mis->getSize();
			is = mis;
			free = true;
			type = Transfer::TYPE_PARTIAL_LIST;
		}
		else
		{
			aSource.fileNotAvail("Unknown file type");
			return false;
		}
	}
	catch (const ShareException& e)
	{
		// Partial file sharing upload
		if (aType == Transfer::names[Transfer::TYPE_FILE] && aFile.compare(0, 4, "TTH/") == 0)
		{
		
			TTHValue fileHash(aFile.c_str() + 4); //[+]FlylinkDC++
			
			if (QueueManager::getInstance()->isChunkDownloaded(fileHash, aStartPos, aBytes, sourceFile))
			{
				try
				{
					SharedFileStream* ss = new SharedFileStream(sourceFile, File::READ, File::OPEN | File::SHARED | File::NO_CACHE_HINT);
					
					start = aStartPos;
					fileSize = ss->getSize();
					size = (aBytes == -1) ? fileSize - start : aBytes;
					
					if ((start + size) > fileSize)
					{
						aSource.fileNotAvail();
						delete ss;
						return false;
					}
					
					ss->setPos(start);
					is = ss;
					
					if ((start + size) < fileSize)
					{
						is = new LimitedInputStream(is, size);
					}
					
					partial = true;
					type = Transfer::TYPE_FILE;
					goto ok; // don't fix "goto", it is fixed in wx version anyway
				}
				catch (const Exception&)
				{
					delete is;
				}
			}
		}
		aSource.fileNotAvail(e.getError());
		return false;
	}
	catch (const Exception& e)
	{
		LogManager::getInstance()->message(STRING(UNABLE_TO_SEND_FILE) + " " + sourceFile + ": " + e.getError());
		aSource.fileNotAvail();
		return false;
	}
	
ok:

	Lock l(cs);
	
	uint8_t slotType = aSource.getSlotType();
	
	if (slotType != UserConnection::STDSLOT)
	{
		bool hasReserved = reservedSlots.find(aSource.getUser()) != reservedSlots.end();
		bool isFavorite = FavoriteManager::getInstance()->hasSlot(aSource.getUser());
		bool hasFreeSlot = (getFreeSlots() > 0) && ((uploadQueue.empty() && notifiedUsers.empty()) || isNotifiedUser(aSource.getUser()));
		
		if (!(hasReserved || isFavorite || getAutoSlot() || hasFreeSlot)
		        || hasUpload(aSource)
		   )
		{
			bool supportsFree = aSource.isSet(UserConnection::FLAG_SUPPORTS_MINISLOTS);
			bool allowedFree = (slotType == UserConnection::EXTRASLOT) || aSource.isSet(UserConnection::FLAG_OP) || getFreeExtraSlots() > 0;
			bool partialFree = partial && ((slotType == UserConnection::PARTIALSLOT) || (extraPartial < SETTING(EXTRA_PARTIAL_SLOTS)));
			
			if (free && supportsFree && allowedFree)
			{
				slotType = UserConnection::EXTRASLOT;
			}
			else if (partialFree)
			{
				slotType = UserConnection::PARTIALSLOT;
			}
			else
			{
				delete is;
				aSource.maxedOut(addFailedUpload(aSource, sourceFile, aStartPos, fileSize));
				aSource.disconnect();
				return false;
			}
		}
		else
		{
			slotType = UserConnection::STDSLOT;
		}
		
		setLastGrant(GET_TICK());
	}
	
	// remove file from upload queue
	clearUserFiles(aSource.getUser());
	
	// remove user from notified list
	SlotIter cu = notifiedUsers.find(aSource.getUser());
	if (cu != notifiedUsers.end())
	{
		notifiedUsers.erase(cu);
	}
	
	bool resumed = false;
	for (UploadList::iterator i = delayUploads.begin(); i != delayUploads.end(); ++i)
	{
		Upload* up = *i;
		if (&aSource == &up->getUserConnection())
		{
			delayUploads.erase(i);
			if (sourceFile != up->getPath())
			{
				logUpload(up);
			}
			else
			{
				resumed = true;
			}
			delete up;
			break;
		}
	}
	
	Upload* u = new Upload(aSource, sourceFile, TTHValue());
	u->setStream(is);
	u->setSegment(Segment(start, size));
	
	if (u->getSize() != fileSize)
		u->setFlag(Upload::FLAG_CHUNKED);
		
	if (resumed)
		u->setFlag(Upload::FLAG_RESUMED);
		
	if (partial)
		u->setFlag(Upload::FLAG_PARTIAL);
		
	u->setFileSize(fileSize);
	u->setType(type);
	
	uploads.push_back(u);
	
	if (aSource.getSlotType() != slotType)
	{
		// remove old count
		switch (aSource.getSlotType())
		{
			case UserConnection::STDSLOT:
				running--;
				break;
			case UserConnection::EXTRASLOT:
				extra--;
				break;
			case UserConnection::PARTIALSLOT:
				extraPartial--;
				break;
		}
		// set new slot count
		switch (slotType)
		{
			case UserConnection::STDSLOT:
				running++;
				break;
			case UserConnection::EXTRASLOT:
				extra++;
				break;
			case UserConnection::PARTIALSLOT:
				extraPartial++;
				break;
		}
		
		// user got a slot
		aSource.setSlotType(slotType);
	}
	
	return true;
}

int64_t UploadManager::getRunningAverage()
{
	Lock l(cs);
	int64_t avg = 0;
	for (UploadList::const_iterator i = uploads.begin(); i != uploads.end(); ++i)
	{
		Upload* u = *i;
		avg += static_cast<int64_t>(u->getAverageSpeed());
	}
	return avg;
}

bool UploadManager::getAutoSlot()
{
	/** A 0 in settings means disable */
	if (SETTING(MIN_UPLOAD_SPEED) == 0)
		return false;
	/** Max slots */
	if (getSlots() + SETTING(AUTO_SLOTS) < running)
		return false;
	/** Only grant one slot per 30 sec */
	if (GET_TICK() < getLastGrant() + 30 * 1000)
		return false;
	/** Grant if upload speed is less than the threshold speed */
	return getRunningAverage() < (SETTING(MIN_UPLOAD_SPEED) * 1024);
}

void UploadManager::removeUpload(Upload* aUpload, bool delay)
{
	Lock l(cs);
	dcassert(find(uploads.begin(), uploads.end(), aUpload) != uploads.end());
	uploads.erase(remove(uploads.begin(), uploads.end(), aUpload), uploads.end());
	
	if (delay)
	{
		delayUploads.push_back(aUpload);
	}
	else
	{
		delete aUpload;
	}
}

void UploadManager::reserveSlot(const HintedUser& aUser, uint64_t aTime)
{
	{
		Lock l(cs);
		reservedSlots[aUser] = GET_TICK() + aTime * 1000;
	}
	if (aUser.user->isOnline())
	{
		// find user in uploadqueue to connect with correct token
		auto it = find_if(uploadQueue.cbegin(), uploadQueue.cend(), [&](const UserPtr & u)
		{
			return u == aUser.user;
		});
		if (it != uploadQueue.cend())
		{
			ClientManager::getInstance()->connect(aUser, it->token);
		}
	}
}

void UploadManager::unreserveSlot(const UserPtr& aUser)
{
	Lock l(cs);
	SlotIter uis = reservedSlots.find(aUser);
	if (uis != reservedSlots.end())
		reservedSlots.erase(uis);
}

void UploadManager::on(UserConnectionListener::Get, UserConnection* aSource, const string& aFile, int64_t aResume) noexcept
{
	if (aSource->getState() != UserConnection::STATE_GET)
	{
		dcdebug("UM::onGet Bad state, ignoring\n");
		return;
	}
	
	int64_t bytes = -1;
	if (prepareFile(*aSource, Transfer::names[Transfer::TYPE_FILE], Util::toAdcFile(aFile), aResume, bytes))
	{
		aSource->setState(UserConnection::STATE_SEND);
		aSource->fileLength(Util::toString(aSource->getUpload()->getSize()));
	}
}

void UploadManager::on(UserConnectionListener::Send, UserConnection* aSource) noexcept
{
	if (aSource->getState() != UserConnection::STATE_SEND)
	{
		dcdebug("UM::onSend Bad state, ignoring\n");
		return;
	}
	
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);
	if (!u)
		return;
		
	u->setStart(GET_TICK());
	u->tick();
	aSource->setState(UserConnection::STATE_RUNNING);
	aSource->transmitFile(u->getStream());
	fire(UploadManagerListener::Starting(), u);
}

void UploadManager::on(AdcCommand::GET, UserConnection* aSource, const AdcCommand& c) noexcept
{
	if (aSource->getState() != UserConnection::STATE_GET)
	{
		dcdebug("UM::onGET Bad state, ignoring\n");
		return;
	}
	
	const string& type = c.getParam(0);
	const string& fname = c.getParam(1);
	int64_t aStartPos = Util::toInt64(c.getParam(2));
	int64_t aBytes = Util::toInt64(c.getParam(3));
	
	if (prepareFile(*aSource, type, fname, aStartPos, aBytes, c.hasFlag("RE", 4)))
	{
		Upload* u = aSource->getUpload();
		dcassert(u != NULL);
		if (!u)
			return;
		AdcCommand cmd(AdcCommand::CMD_SND);
		cmd.addParam(type).addParam(fname)
		.addParam(Util::toString(u->getStartPos()))
		.addParam(Util::toString(u->getSize()));
		
		const string l_ext = Text::toLower(Util::getFileExt(u->getPath()));
		const bool l_is_compressed = Util::is_compress_ext(l_ext); //[+]FlylinkDC++
		if (!l_is_compressed)
			if (c.hasFlag("ZL", 4))
			{
				u->setStream(new FilteredInputStream<ZFilter, true>(u->getStream()));
				u->setFlag(Upload::FLAG_ZUPLOAD);
				cmd.addParam("ZL1");
			}
			
		aSource->send(cmd);
		
		u->setStart(GET_TICK());
		u->tick();
		aSource->setState(UserConnection::STATE_RUNNING);
		aSource->transmitFile(u->getStream());
		fire(UploadManagerListener::Starting(), u);
	}
}

void UploadManager::on(UserConnectionListener::BytesSent, UserConnection* aSource, size_t aBytes, size_t aActual) noexcept
{
	dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);
	if (u)
	{
		aSource->AddRatio(aBytes);
		u->addPos(aBytes, aActual);
		u->tick();
	}
}

void UploadManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) noexcept
{
	Upload* u = aSource->getUpload();
	
	if (u)
	{
		fire(UploadManagerListener::Failed(), u, aError);
		
		dcdebug("UM::onFailed (%s): Removing upload\n", aError.c_str());
		removeUpload(u);
	}
	
	removeConnection(aSource);
}

void UploadManager::on(UserConnectionListener::TransmitDone, UserConnection* aSource) noexcept
{
	dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);
	if (!u)
		return;
	aSource->setState(UserConnection::STATE_GET);
	
	if (!u->isSet(Upload::FLAG_CHUNKED))
	{
		logUpload(u);
		removeUpload(u);
	}
	else {
		removeUpload(u, true);
	}
}

void UploadManager::logUpload(const Upload* u)
{
	if (BOOLSETTING(LOG_UPLOADS) && u->getType() != Transfer::TYPE_TREE && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || u->getType() != Transfer::TYPE_FULL_LIST))
	{
		StringMap params;
		u->getParams(u->getUserConnection(), params);
		LOG(UPLOAD, params);
	}
	
	fire(UploadManagerListener::Complete(), u);
}

size_t UploadManager::addFailedUpload(const UserConnection& source, const string& file, int64_t pos, int64_t size)
{
	size_t queue_position = 0;
	
	auto it = find_if(uploadQueue.begin(), uploadQueue.end(), [&](const UserPtr & u) -> bool { ++queue_position; return u == source.getUser(); });
	if (it != uploadQueue.end())
	{
		it->token = source.getToken();
		for (auto fileIter = it->files.cbegin(); fileIter != it->files.cend(); ++fileIter)
		{
			if ((*fileIter)->getFile() == file)
			{
				(*fileIter)->setPos(pos);
				return queue_position;
			}
		}
	}
	
	UploadQueueItem* uqi = new UploadQueueItem(source.getHintedUser(), file, pos, size);
	if (it == uploadQueue.end())
	{
		++queue_position;
		
		WaitingUser wu(source.getHintedUser(), source.getToken());
		wu.files.insert(uqi);
		uploadQueue.push_back(wu);
	}
	else
	{
		it->files.insert(uqi);
	}
	
	fire(UploadManagerListener::QueueAdd(), uqi);
	return queue_position;
}

void UploadManager::clearUserFiles(const UserPtr& aUser)
{
	Lock l(cs); //[+]PPA
	auto it = find_if(uploadQueue.cbegin(), uploadQueue.cend(), [&](const UserPtr & u)
	{
		return u == aUser;
	});
	if (it != uploadQueue.cend())
	{
		for (auto i = it->files.cbegin(); i != it->files.cend(); ++i)
		{
			fire(UploadManagerListener::QueueItemRemove(), (*i));
			(*i)->dec();
		}
		uploadQueue.erase(it);
		fire(UploadManagerListener::QueueRemove(), aUser);
	}
}

void UploadManager::addConnection(UserConnectionPtr conn)
{
	if (PGLoader::getInstance()->getIPBlockBool(conn->getRemoteIp()))
	{
		conn->error("Your IP is Blocked!");
		LogManager::getInstance()->message("IPFilter: Blocked incoming connection from " + conn->getRemoteIp());
		QueueManager::getInstance()->removeSource(conn->getUser(), QueueItem::Source::FLAG_REMOVED);
		removeConnection(conn);
		return;
	}
	conn->addListener(this);
	conn->setState(UserConnection::STATE_GET);
}

void UploadManager::removeConnection(UserConnection* aSource)
{
	dcassert(aSource->getUpload() == NULL);
	aSource->removeListener(this);
	
	// slot lost
	switch (aSource->getSlotType())
	{
		case UserConnection::STDSLOT:
			running--;
			break;
		case UserConnection::EXTRASLOT:
			extra--;
			break;
		case UserConnection::PARTIALSLOT:
			extraPartial--;
			break;
	}
	aSource->setSlotType(UserConnection::NOSLOT);
}

void UploadManager::notifyQueuedUsers()
{
	if (uploadQueue.empty()) return;        //no users to notify
	
	int freeslots = getFreeSlots();
	if (freeslots > 0)
	{
		freeslots -= notifiedUsers.size();
		while (!uploadQueue.empty() && freeslots > 0)
		{
			// let's keep him in the notifiedList until he asks for a file
			WaitingUser wu = uploadQueue.front();
			clearUserFiles(wu.user);
			
			notifiedUsers[wu.user] = GET_TICK();
			
			ClientManager::getInstance()->connect(wu.user, wu.token);
			
			freeslots--;
		}
	}
}

void UploadManager::on(TimerManagerListener::Minute, uint64_t aTick) noexcept
{
	UserList disconnects;
	{
		Lock l(cs);
		for (SlotIter j = reservedSlots.begin(); j != reservedSlots.end();)
		{
			if (j->second < aTick)
			{
				reservedSlots.erase(j++);
			}
			else
			{
				++j;
			}
		}
		
		for (SlotIter i = notifiedUsers.begin(); i != notifiedUsers.end();)
		{
			if ((i->second + (90 * 1000)) < aTick)
			{
				clearUserFiles(i->first);
				notifiedUsers.erase(i++);
			}
			else
				++i;
		}
		
		if (BOOLSETTING(AUTO_KICK))
		{
			for (UploadList::const_iterator i = uploads.begin(); i != uploads.end(); ++i)
			{
				Upload* u = *i;
				if (u->getUser()->isOnline())
				{
					u->unsetFlag(Upload::FLAG_PENDING_KICK);
					continue;
				}
				
				if (u->isSet(Upload::FLAG_PENDING_KICK))
				{
					disconnects.push_back(u->getUser());
					continue;
				}
				
				if (BOOLSETTING(AUTO_KICK_NO_FAVS) && FavoriteManager::getInstance()->isFavoriteUser(u->getUser()))
				{
					continue;
				}
				
				u->setFlag(Upload::FLAG_PENDING_KICK);
			}
		}
	}
	
	for (UserList::const_iterator i = disconnects.begin(); i != disconnects.end(); ++i)
	{
		LogManager::getInstance()->message(STRING(DISCONNECTED_USER) + " " + Util::toString(ClientManager::getInstance()->getNicks((*i)->getCID(), Util::emptyString)));
		ConnectionManager::getInstance()->disconnect(*i, false);
	}
	
	int freeSlots = getFreeSlots();
	if (freeSlots != lastFreeSlots)
	{
		lastFreeSlots = freeSlots;
		/* I don't like spamming every minute so disabled it for now
		    ClientManager::getInstance()->infoUpdated();
		*/
	}
}

void UploadManager::on(GetListLength, UserConnection* conn) noexcept
{
	conn->error("GetListLength not supported");
	conn->disconnect(false);
}

void UploadManager::on(AdcCommand::GFI, UserConnection* aSource, const AdcCommand& c) noexcept
{
	if (aSource->getState() != UserConnection::STATE_GET)
	{
		dcdebug("UM::onSend Bad state, ignoring\n");
		return;
	}
	
	if (c.getParameters().size() < 2)
	{
		aSource->send(AdcCommand(AdcCommand::SEV_RECOVERABLE, AdcCommand::ERROR_PROTOCOL_GENERIC, "Missing parameters"));
		return;
	}
	
	const string& type = c.getParam(0);
	const string& ident = c.getParam(1);
	
	if (type == Transfer::names[Transfer::TYPE_FILE])
	{
		try
		{
			aSource->send(ShareManager::getInstance()->getFileInfo(ident));
		}
		catch (const ShareException&)
		{
			aSource->fileNotAvail();
		}
	}
	else {
		aSource->fileNotAvail();
	}
}

// TimerManagerListener
void UploadManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept
{
	{
		Lock l(cs);
		UploadList ticks;
		
		for (UploadList::iterator i = delayUploads.begin(); i != delayUploads.end();)
		{
			Upload* u = *i;
			
			if (++u->delayTime > 10)
			{
				logUpload(u);
				delete u;
				
				delayUploads.erase(i);
				i = delayUploads.begin();
			}
			else
			{
				++i;
			}
		}
		
		for (UploadList::const_iterator i = uploads.begin(); i != uploads.end(); ++i)
		{
			if ((*i)->getPos() > 0)
			{
				ticks.push_back(*i);
				(*i)->tick();
			}
		}
		
		if (ticks.size() > 0)
			fire(UploadManagerListener::Tick(), ticks);
			
		notifyQueuedUsers();
		fire(UploadManagerListener::QueueUpdate());
	}
	
	if (!isFireball)
	{
		if (getRunningAverage() >= 1024 * 1024)
		{
			if (fireballStartTick == 0)
			{
				// >= 1 MiB/s detected for the first time
				fireballStartTick = aTick;
			}
			else if ((aTick - fireballStartTick) > 60 * 1000)
			{
				// fine, speed is over 100 kB/s for more than 60 seconds
				isFireball = true;
				ClientManager::getInstance()->infoUpdated();
			}
		}
		else
		{
			// speed dropped below 100 kB/s
			fireballStartTick = 0;
		}
		
		if (!isFireball && !isFileServer)
		{
			if (((time(NULL) - Util::getStartTime()) > 10 * 24 * 60 * 60) && // > 10 days uptime
			        (Socket::getTotalUp() > 100ULL * 1024 * 1024 * 1024) && // > 100 GiB uploaded
			        (ShareManager::getInstance()->getSharedSize() > 1.5 * 1024 * 1024 * 1024 * 1024)) // > 1.5 TiB shared
			{
				isFileServer = true;
				ClientManager::getInstance()->infoUpdated();
			}
		}
	}
}

void UploadManager::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept
{
	if (!aUser->isOnline())
	{
		Lock l(cs);
		clearUserFiles(aUser);
	}
}

void UploadManager::removeDelayUpload(const UserPtr& aUser)
{
	Lock l(cs);
	for (UploadList::iterator i = delayUploads.begin(); i != delayUploads.end(); ++i)
	{
		Upload* up = *i;
		if (aUser == up->getUser())
		{
			delayUploads.erase(i);
			delete up;
			break;
		}
	}
}

/**
 * Abort upload of specific file
 */
void UploadManager::abortUpload(const string& aFile, bool waiting)
{
	bool nowait = true;
	
	{
		Lock l(cs);
		
		for (UploadList::const_iterator i = uploads.begin(); i != uploads.end(); ++i)
		{
			Upload* u = (*i);
			
			if (u->getPath() == aFile)
			{
				u->getUserConnection().disconnect(true);
				nowait = false;
			}
		}
	}
	
	if (nowait) return;
	if (!waiting) return;
	
	for (int i = 0; i < 20 && nowait == false; i++)
	{
		Thread::sleep(250);
		{
			Lock l(cs);
			
			nowait = true;
			for (UploadList::const_iterator i = uploads.begin(); i != uploads.end(); ++i)
			{
				Upload* u = (*i);
				
				if (u->getPath() == aFile)
				{
					dcdebug("upload %s is not removed\n", aFile.c_str());
					nowait = false;
					break;
				}
			}
		}
	}
	
	if (!nowait)
		dcdebug("abort upload timeout %s\n", aFile.c_str());
}

} // namespace dcpp

/**
 * @file
 * $Id: UploadManager.cpp 581 2011-11-02 18:59:46Z bigmuscle $
 */
