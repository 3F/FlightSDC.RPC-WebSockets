/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_LOG_MANAGER_H
#define DCPLUSPLUS_DCPP_LOG_MANAGER_H

#include "File.h"
#include "CFlylinkDBManager.h"

namespace dcpp
{

class LogManagerListener
{
	public:
		virtual ~LogManagerListener() { }
		template<int I> struct X
		{
			enum { TYPE = I };
		};
		
		typedef X<0> Message;
		virtual void on(Message, const string&) noexcept { }
};

class LogManager : public Singleton<LogManager>, public Speaker<LogManagerListener>
{
	public:
		enum LogArea { CHAT, PM, DOWNLOAD, UPLOAD, SYSTEM, STATUS,
		               WEBSERVER,
#ifdef RIP_USE_LOG_PROTOCOL
		               PROTOCOL,
#endif
		               LAST
		             };
		enum {FILE, FORMAT};
		
		void log(LogArea area, const StringMap& params, bool p_only_file = false) noexcept;
		void message(const string& msg, bool p_only_file = false);
		
		const string& getSetting(int area, int sel) const
		{
			return SettingsManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(logOptions[area][sel]), true);
		}
		
		void saveSetting(int area, int sel, const string& setting)
		{
			SettingsManager::getInstance()->set(static_cast<SettingsManager::StrSetting>(logOptions[area][sel]), setting);
		}
		
	private:
		void log(const string& p_area, const string& p_msg) noexcept;
		
		friend class Singleton<LogManager>;
		CriticalSection m_cs; // [!] IRainman opt: use spin lock here.
		
		int logOptions[LAST][2];
#ifdef _DEBUG
		unordered_map<string, pair<string, size_t>> m_log_path_cache;
		size_t total, missed;
#else
		unordered_map<string, string> m_log_path_cache;
#endif
		LogManager();
		~LogManager()
		{
#ifdef _DEBUG
			dcdebug("log path cache: total found=%i, missed=%i size=%i\n", total, missed, m_log_path_cache.size());
#endif
		}
		
};

#define LOG(area, msg)  LogManager::getInstance()->log(LogManager::area, msg)

class CFlyLog
{
	public:
		const string m_message;
		const uint64_t m_start;
		uint64_t m_tc;
		const bool m_use_cmd_debug;
		void log(const string& p_msg)
		{
			if (m_use_cmd_debug)
			{
				COMMAND_DEBUG(p_msg, DebugManager::CLIENT_OUT, "127.0.0.1");
			}
			LogManager::getInstance()->message(p_msg);
		}
	public:
		CFlyLog(const string& p_message, bool p_use_cmd_debug = false) : m_message(p_message), m_start(GET_TICK()), m_tc(m_start), m_use_cmd_debug(p_use_cmd_debug)
		{
			log("[Start] " + m_message);
		}
		~CFlyLog()
		{
			const uint64_t l_current = GET_TICK();
			log("[Stop ] " + m_message + " [" + Util::toString(l_current - m_tc) + " ms, Total: " + Util::toString(l_current - m_start) + " ms]");
		}
		void step(const string& p_message_step, const bool p_reset_count = true)
		{
			const uint64_t l_current = GET_TICK();
			log("[Step ] " + m_message + p_message_step + " [" + Util::toString(l_current - m_tc) + " ms]");
			if (p_reset_count)
				m_tc = l_current;
		}
};

} // namespace dcpp

#endif // !defined(LOG_MANAGER_H)

/**
 * @file
 * $Id: LogManager.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
