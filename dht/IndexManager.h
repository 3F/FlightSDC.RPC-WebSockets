/*
 * Copyright (C) 2008-2011 Big Muscle, http://strongdc.sf.net
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

#ifndef _INDEXMANAGER_H
#define _INDEXMANAGER_H

#include "Constants.h"
#include "KBucket.h"

#include "../client/ShareManager.h"
#include "../client/Singleton.h"

#include <boost/atomic.hpp>

namespace dht
{


	class IndexManager :
	public Singleton<IndexManager>
	{
	public:
		IndexManager();
		~IndexManager();

		
	/** Finds TTH in known indexes and returns it */
	bool findResult(const TTHValue& tth, SourceList& sources) const;
	
	/** Try to publish next file in queue */
	void publishNextFile();
	
	/** Loads existing indexes from disk */
	void loadIndexes(SimpleXML& xml);
	
	/** Save all indexes to disk */
	void saveIndexes(SimpleXML& xml);
		
	/** How many files is currently being published */
		void incPublishing()
		{
			++publishing;
		}
		void decPublishing()
		{
			--publishing;
		}
	
	/** Is publishing allowed? */
		void setPublish(bool _publish)
		{
			publish = _publish;
		}
		bool getPublish() const
		{
			return publish;
		}
	
	/** Processes incoming request to publish file */
		void processPublishSourceRequest(const string& ip, uint16_t port, const UDPKey& udpKey, const AdcCommand& cmd);
	
	/** Removes old sources */
	void checkExpiration(uint64_t aTick);
	
	/** Publishes shared file */
	void publishFile(const TTHValue& tth, int64_t size);
	
	/** Publishes partially downloaded file */
	void publishPartialFile(const TTHValue& tth);
	
	/** Set time when our sharelist should be republished */
	void setNextPublishing()
		{
			nextRepublishTime = GET_TICK() + REPUBLISH_TIME;
		}
	
	/** Is time when we should republish our sharelist? */
	bool isTimeForPublishing() const
		{
			return GET_TICK() >= nextRepublishTime;
		}
	
	private:

	/** Contains known hashes in the network and their sources */
		typedef std::unordered_map<TTHValue, SourceList> TTHMap;
	TTHMap tthList;
	
	/** Queue of files prepared for publishing */
	typedef std::deque<File> FileQueue;
	FileQueue publishQueue;
		
	/** Is publishing allowed? */
	bool publish;
	
	/** How many files is currently being published */
		boost::atomic<long> publishing;
	
	/** Time when our sharelist should be republished */
	uint64_t nextRepublishTime;	
	
	/** Synchronizes access to tthList */
	mutable CriticalSection cs;
	
	/** Add new source to tth list */
		void addSource(const TTHValue& tth, const CID& cid, const string& ip, uint16_t port, uint64_t size, bool partial);

		int m_hour_count;
	};

} // namespace dht

#endif	// _INDEXMANAGER_H