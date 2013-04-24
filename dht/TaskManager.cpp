/*
 * Copyright (C) 2009-2011 Big Muscle, http://strongdc.sf.net
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
#include "stdafx.h"

#include "BootstrapManager.h"
#include "DHT.h"
#include "IndexManager.h"
#include "SearchManager.h"
#include "TaskManager.h"
#include "Utils.h"

#include "../client/SettingsManager.h"
#include "../client/ShareManager.h"
#include "../client/TimerManager.h"
#include "../client/LogManager.h"

namespace dht
{

TaskManager::TaskManager(void) : m_timer_execute(0)
{
}

	TaskManager::~TaskManager(void)
	{
		TimerManager::getInstance()->removeListener(this);
	}

	void TaskManager::start()
	{
	const uint64_t tick = GET_TICK();
		nextPublishTime = tick;
		nextSearchTime = tick;
		nextSelfLookup = tick + 3*60*1000,
		nextFirewallCheck = tick + FWCHECK_TIME, 
		lastBootstrap = 0;
	        m_lastDownloadDHTError = 0;
		TimerManager::getInstance()->addListener(this);
	}

	// TimerManagerListener
	void TaskManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept
	{
	CFlySafeGuard b(m_timer_execute);
#ifdef _DEBUG
    // CFlyLog l_TaskManagerLog("TimerManagerListener::Second aTick = " + Util::toString(aTick) + " GetCurrentThreadId() = " + Util::toString(GetCurrentThreadId()));
#endif
	auto l_dht = DHT::getInstance();
	if (l_dht->isConnected() && l_dht->getNodesCount() >= DHT_K)
		{
		auto l_dht_idx = IndexManager::getInstance();
		if (!l_dht->isFirewalled() && l_dht_idx->getPublish() && aTick >= nextPublishTime)
			{
				// publish next file
			l_dht_idx->publishNextFile();
				nextPublishTime = aTick + PUBLISH_TIME;
			}
		}
		else
		{
		bool l_need_dht_download = true;
		if(m_lastDownloadDHTError) // Были ошибки загрузки - ждем полчасика и не ломимся на DHT сервер?
		{
			if(aTick - m_lastDownloadDHTError > 60 * 30 * 1000) // TODO - конфигурируем число ? 
				                                     // TODO-2 отмечаем сервер как глючный и не выбираем рандомно его некоторое время
			{
				l_need_dht_download = true;
				m_lastDownloadDHTError = 0;
			}
			else
				l_need_dht_download = false;
		}
		if(l_need_dht_download)
		{
		string l_sub_agent; // TODO - похоронить
		const bool l_15000 = aTick - lastBootstrap > 15000;
		const bool l_nodes_cnt = aTick - lastBootstrap >= 2000 &&l_dht->getNodesCount() < 2 * DHT_K;
		if (l_15000 || l_nodes_cnt)
			{
				// bootstrap if we doesn't know any remote node
			const bool l_result = BootstrapManager::getInstance()->process(l_sub_agent); 
			if(l_result == false)
			{
				m_lastDownloadDHTError = aTick;
			}
			else
			{
					m_lastDownloadDHTError = 0;
			}
				lastBootstrap = aTick;
			}
			}
	}

		if(aTick >= nextSearchTime)
		{
			SearchManager::getInstance()->processSearches();
			nextSearchTime = aTick + SEARCH_PROCESSTIME;
		}

		if(aTick >= nextSelfLookup)
		{
			// find myself in the network
			SearchManager::getInstance()->findNode(ClientManager::getInstance()->getMe()->getCID());
			nextSelfLookup = aTick + SELF_LOOKUP_TIMER;
	}

		if(aTick >= nextFirewallCheck)
		{
		l_dht->setRequestFWCheck();
			nextFirewallCheck = aTick + FWCHECK_TIME;
		}
	}

	void TaskManager::on(TimerManagerListener::Minute, uint64_t aTick) noexcept
	{
	CFlySafeGuard b(m_timer_execute);
		Utils::cleanFlood();

		std::vector<RoutingTable*> tmp;

		{
			Lock l(cs);
			tmp = zones;
		}

	for (auto i = tmp.cbegin(); i != tmp.cend(); ++i) // remove dead nodes
		{
			// there can be a race condition and zone can already be split now (i.e. zone->bucket == NULL)
		DHT::getInstance()->lock([&]()
		{
			(*i)->checkExpiration(aTick);
		});
		}

		DHT::getInstance()->checkExpiration(aTick);
		IndexManager::getInstance()->checkExpiration(aTick);
	}

}
