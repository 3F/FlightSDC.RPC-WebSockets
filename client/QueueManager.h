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

#ifndef DCPLUSPLUS_DCPP_QUEUE_MANAGER_H
#define DCPLUSPLUS_DCPP_QUEUE_MANAGER_H

#include "TimerManager.h"
#include "ClientManager.h"

#include "Exception.h"
#include "User.h"
#include "File.h"
#include "QueueItem.h"
#include "Singleton.h"
#include "DirectoryListing.h"
#include "MerkleTree.h"

#include "QueueManagerListener.h"
#include "SearchManagerListener.h"
#include "ClientManagerListener.h"
#include "LogManager.h"

namespace dcpp
{

STANDARD_EXCEPTION(QueueException);

class UserConnection;

class ConnectionQueueItem;
class QueueLoader;

class QueueManager : public Singleton<QueueManager>, public Speaker<QueueManagerListener>, private TimerManagerListener,
	private SearchManagerListener, private ClientManagerListener
{
	public:
		typedef list<QueueItemPtr> QueueItemList;
		
		/** Add a file to the queue. */
		void add(const string& aTarget, int64_t aSize, const TTHValue& root, const HintedUser& aUser,
		         Flags::MaskType aFlags = 0, bool addBad = true) throw(QueueException, FileException);
		/** Add a user's filelist to the queue. */
		void addList(const HintedUser& HintedUser, Flags::MaskType aFlags, const string& aInitialDir = Util::emptyString) throw(QueueException, FileException);
		
		/** Readd a source that was removed */
		void readd(const string& target, const HintedUser& aUser) throw(QueueException);
		/** Add a directory to the queue (downloads filelist and matches the directory). */
		void addDirectory(const string& aDir, const HintedUser& aUser, const string& aTarget,
		                  QueueItem::Priority p = QueueItem::DEFAULT) noexcept;
		                  
		int matchListing(const DirectoryListing& dl) noexcept;
		
		bool getTTH(const string& name, TTHValue& tth) const noexcept;
		
		/** Move the target location of a queued item. Running items are silently ignored */
		void move(const string& aSource, const string& aTarget) noexcept;
		
		void remove(const string& aTarget) noexcept;
		void removeSource(const string& aTarget, const UserPtr& aUser, Flags::MaskType reason, bool removeConn = true) noexcept;
		void removeSource(const UserPtr& aUser, Flags::MaskType reason) noexcept;
		
		void recheck(const string& aTarget);
		
		void setPriority(const string& aTarget, QueueItem::Priority p) noexcept;
		void setAutoPriority(const string& aTarget, bool ap) noexcept;
		
		void getTargets(const TTHValue& tth, StringList& sl);
		const QueueItem::StringMap& lockQueue() noexcept { cs.lock(); return fileQueue.getQueue(); } ;
		void unlockQueue() noexcept { cs.unlock(); }
		
		QueueItem::SourceList getSources(const QueueItem* qi) const
		{
			Lock l(cs);
			return qi->getSources();
		}
		QueueItem::SourceList getBadSources(const QueueItem* qi) const
		{
			Lock l(cs);
			return qi->getBadSources();
		}
		size_t countOnlineUsers(const QueueItem* p_qi) const //[+]FlylinkDC++ Team
		{
			Lock l(cs);
			return p_qi->countOnlineUsers();
		}
		size_t getSourcesCount(const QueueItem* qi) const
		{
			Lock l(cs);
			return qi->getSources().size();
		}
		void getChunksVisualisation(const QueueItem* qi, int type, vector<Segment>& p_segments) const
		{
			Lock l(cs);
			qi->getChunksVisualisation(type, p_segments);
		}
		bool getQueueInfo(const UserPtr& aUser, string& aTarget, int64_t& aSize, int& aFlags) noexcept;
		Download* getDownload(UserConnection& aSource, string& aMessage) noexcept;
		void putDownload(Download* aDownload, bool finished, bool reportFinish = true) noexcept;
		void setFile(Download* download);
		
		/** @return The highest priority download the user has, PAUSED may also mean no downloads */
		QueueItem::Priority hasDownload(const UserPtr& aUser) noexcept;
		
		void loadQueue() noexcept;
		void saveQueue(bool force = false) noexcept;
		
		void noDeleteFileList(const string& path);
		
		bool handlePartialSearch(const TTHValue& tth, PartsInfo& _outPartsInfo);
		bool handlePartialResult(const HintedUser& aUser, const TTHValue& tth, const QueueItem::PartialSource& partialSource, PartsInfo& outPartialInfo);
		
		bool dropSource(Download* d);
		
		const QueueItemList getRunningFiles() const noexcept
		{
		    QueueItemList ql;
		    for (auto i = fileQueue.getQueue().begin(); i != fileQueue.getQueue().end(); ++i)
	{
		QueueItem* q = i->second;
		if (q->isRunning())
			{
				ql.push_back(q);
			}
		}
		return ql;
		}
		
		bool getTargetByRoot(const TTHValue& tth, string& target, string& tempTarget)
		{
			Lock l(cs);
			QueueItemList ql;
			fileQueue.find(ql, tth);
			
			if (ql.empty()) return false;
			
			target = ql.front()->getTarget();
			tempTarget = ql.front()->getTempTarget();
			return true;
		}
		
		bool isChunkDownloaded(const TTHValue& tth, int64_t startPos, int64_t& bytes, string& target)
		{
			Lock l(cs);
			QueueItemList ql;
			fileQueue.find(ql, tth);
			
			if (ql.empty()) return false;
			
			QueueItem* qi = ql.front();
			
			target = qi->isFinished() ? qi->getTarget() : qi->getTempTarget();
			
			return qi->isChunkDownloaded(startPos, bytes);
		}
		
		GETSET(uint64_t, lastSave, LastSave);
		GETSET(string, queueFile, QueueFile);
		
		enum { MOVER_LIMIT = 10 * 1024 * 1024 };
		class FileMover : public Thread
		{
			public:
				FileMover() : active(false) { }
				virtual ~FileMover()
				{
					join();
				}
				
				void moveFile(const string& source, const string& target);
				virtual int run();
			private:
				typedef pair<string, string> FilePair;
				typedef vector<FilePair> FileList;
				typedef FileList::iterator FileIter;
				
				bool active;
				
				FileList files;
				CriticalSection cs;
		} mover;
		
		typedef vector<pair<QueueItem::SourceConstIter, QueueItem*> > PFSSourceList;
		
		class Rechecker : public Thread
		{
				struct DummyOutputStream : OutputStream
				{
					virtual size_t write(const void*, size_t n)
					{
						return n;
					}
					virtual size_t flush()
					{
						return 0;
					}
				};
				
			public:
				explicit Rechecker(QueueManager* qm_) : qm(qm_), active(false) { }
				virtual ~Rechecker()
				{
					join();
				}
				
				void add(const string& file);
				virtual int run();
				
			private:
				QueueManager* qm;
				bool active;
				
				StringList files;
				CriticalSection cs;
		} rechecker;
		
		/** All queue items by target */
		class FileQueue
		{
			public:
				FileQueue() { }
				~FileQueue();
				void add(QueueItem* qi);
				QueueItem* add(const string& aTarget, int64_t aSize, Flags::MaskType aFlags, QueueItem::Priority p,
				               const string& aTempTarget, time_t aAdded, const TTHValue& root)
				throw(QueueException, FileException);
				
				QueueItem* find(const string& target) const;
				void find(QueueItemList& sl, int64_t aSize, const string& ext);
				uint8_t getMaxSegments(int64_t filesize) const;
				void find(StringList& sl, int64_t aSize, const string& ext);
				void find(QueueItemList& ql, const TTHValue& tth);
				
				// find some PFS sources to exchange parts info
				void findPFSSources(PFSSourceList&);
				
				// return a PFS tth to DHT publish
				TTHValue* findPFSPubTTH();
				
				QueueItem* findAutoSearch(deque<string>& recent) const;
				size_t getSize() const
				{
					return queue.size();
				}
				const QueueItem::StringMap& getQueue() const
				{
					return queue;
				}
				void move(QueueItem* qi, const string& aTarget);
				void remove(QueueItem* qi);
			private:
				QueueItem::StringMap queue;
		};
		
		/** QueueItems by target */
		FileQueue fileQueue;
		
	private:
		/** All queue items indexed by user (this is a cache for the FileQueue really...) */
		class UserQueue
		{
			public:
				void add(QueueItem* qi);
				void add(QueueItem* qi, const UserPtr& aUser);
				QueueItem* getNext(const UserPtr& aUser, QueueItem::Priority minPrio = QueueItem::LOWEST, int64_t wantedSize = 0, int64_t lastSpeed = 0, bool allowRemove = false);
				QueueItem* getRunning(const UserPtr& aUser);
				void addDownload(QueueItem* qi, Download* d);
				void removeDownload(QueueItem* qi, const UserPtr& d);
				
				void remove(QueueItem* qi, bool removeRunning = true);
				void remove(QueueItem* qi, const UserPtr& aUser, bool removeRunning = true);
				void setPriority(QueueItem* qi, QueueItem::Priority p);
				
				const unordered_map<UserPtr, QueueItemList, User::Hash>& getList(size_t i) const
				{
					return userQueue[i];
				}
				const unordered_map<UserPtr, QueueItemPtr, User::Hash>& getRunning() const
				{
					return m_running;
				}
				
				string getLastError()
				{
					string tmp = lastError;
					lastError = Util::emptyString;
					return tmp;
				}
				
			private:
				/** QueueItems by priority and user (this is where the download order is determined) */
				unordered_map<UserPtr, QueueItemList, User::Hash> userQueue[QueueItem::LAST];
				/** Currently running downloads, a QueueItem is always either here or in the userQueue */
				unordered_map<UserPtr, QueueItemPtr, User::Hash> m_running;
				/** Last error message to sent to TransferView */
				string lastError;
		};
		
		friend class QueueLoader;
		friend class Singleton<QueueManager>;
		
		QueueManager();
		~QueueManager();
		
		mutable CriticalSection cs;
		
		/** QueueItems by user */
		UserQueue userQueue;
		/** Directories queued for downloading */
		unordered_multimap<UserPtr, DirectoryItemPtr, User::Hash> directories;
		/** Recent searches list, to avoid searching for the same thing too often */
		deque<string> recent;
		/** The queue needs to be saved */
		bool dirty;
		/** Next search */
		uint64_t nextSearch;
		/** File lists not to delete */
		StringList protectedFileLists;
		/** Sanity check for the target filename */
		static string checkTarget(const string& aTarget, bool checkExsistence) throw(QueueException, FileException);
		/** Add a source to an existing queue item */
		bool addSource(QueueItem* qi, const HintedUser& aUser, Flags::MaskType addBad) throw(QueueException, FileException);
		
		void processList(const string& name, const HintedUser& user, int flags);
		
		void load(const SimpleXML& aXml);
		void moveFile(const string& source, const string& target);
		static void moveFile_(const string& source, const string& target);
		void moveStuckFile(QueueItem* qi);
		void rechecked(QueueItem* qi);
		
		void setDirty();
		
		string getListPath(const HintedUser& user);
		
		// TimerManagerListener
		void on(TimerManagerListener::Second, uint64_t aTick) noexcept;
		void on(TimerManagerListener::Minute, uint64_t aTick) noexcept;
		
		// SearchManagerListener
		void on(SearchManagerListener::SR, const SearchResultPtr&) noexcept;
		
		// ClientManagerListener
		void on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept;
		void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept;
};

} // namespace dcpp

#endif // !defined(QUEUE_MANAGER_H)

/**
 * @file
 * $Id: QueueManager.h 579 2011-10-05 07:55:26Z bigmuscle $
 */
