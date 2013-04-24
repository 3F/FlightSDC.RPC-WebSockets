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

#include "stdinc.h"
#include "LogManager.h"

namespace dcpp
{

LogManager::LogManager()
#ifdef _DEBUG
	: total(0), missed(0)
#endif
{
	logOptions[UPLOAD][FILE]        = SettingsManager::LOG_FILE_UPLOAD;
	logOptions[UPLOAD][FORMAT]      = SettingsManager::LOG_FORMAT_POST_UPLOAD;
	logOptions[DOWNLOAD][FILE]      = SettingsManager::LOG_FILE_DOWNLOAD;
	logOptions[DOWNLOAD][FORMAT]    = SettingsManager::LOG_FORMAT_POST_DOWNLOAD;
	logOptions[CHAT][FILE]          = SettingsManager::LOG_FILE_MAIN_CHAT;
	logOptions[CHAT][FORMAT]        = SettingsManager::LOG_FORMAT_MAIN_CHAT;
	logOptions[PM][FILE]            = SettingsManager::LOG_FILE_PRIVATE_CHAT;
	logOptions[PM][FORMAT]          = SettingsManager::LOG_FORMAT_PRIVATE_CHAT;
	logOptions[SYSTEM][FILE]        = SettingsManager::LOG_FILE_SYSTEM;
	logOptions[SYSTEM][FORMAT]      = SettingsManager::LOG_FORMAT_SYSTEM;
	logOptions[STATUS][FILE]        = SettingsManager::LOG_FILE_STATUS;
	logOptions[STATUS][FORMAT]      = SettingsManager::LOG_FORMAT_STATUS;
	logOptions[WEBSERVER][FILE]     = SettingsManager::LOG_FILE_WEBSERVER;
	logOptions[WEBSERVER][FORMAT]   = SettingsManager::LOG_FORMAT_WEBSERVER;
#ifdef RIP_USE_LOG_PROTOCOL
	logOptions[PROTOCOL][FILE]      = SettingsManager::LOG_FILE_PROTOCOL;
	logOptions[PROTOCOL][FORMAT]    = SettingsManager::LOG_FORMAT_PROTOCOL;
#endif
}

void LogManager::log(const string& p_area, const string& p_msg) noexcept
{
	Lock l(m_cs);
	try
	{
		string l_area;
		const auto l_fine_it = m_log_path_cache.find(p_area);
		if (l_fine_it != m_log_path_cache.end())
		{
#ifdef _DEBUG
			total++;
//			dcdebug("log path cache: size=%i [%s] %i\n", m_log_path_cache.size(), p_area.c_str(), ++l_fine_it->second.second);
			l_area = l_fine_it->second.first;
#else
			l_area = l_fine_it->second;
#endif
		}
		else
		{
			if (m_log_path_cache.size() > 250)
				m_log_path_cache.clear();
				
			l_area = Util::validateFileName(p_area);
			File::ensureDirectory(l_area);
#ifdef _DEBUG
			missed++;
			const std::pair<string, size_t> l_pair(l_area, 0);
			m_log_path_cache[p_area] = l_pair;
#else
			m_log_path_cache[p_area] = l_area;
#endif
		}
		File f(l_area, File::WRITE, File::OPEN | File::CREATE);
		f.setEndPos(0);
		if (f.getPos() == 0)
		{
			f.write("\xef\xbb\xbf");
		}
		f.write(p_msg + "\r\n");
	}
	catch (const FileException&)
	{
		//-V565
		dcassert(0);
	}
}

void LogManager::log(LogArea area, const StringMap& params, bool p_only_file /* = false */) noexcept
{
#ifndef _CONSOLE
	if (0) //!p_only_file /* && BOOLSETTING(FLY_SQLITE_LOG) */)
	{
		dcassert(CFlylinkDBManager::isValidInstance());
		if (CFlylinkDBManager::isValidInstance())
			CFlylinkDBManager::getInstance()->log(area, params); // [2] https://www.box.net/shared/9e63916273d37e5b2932
	}
	else
#endif
	{
		const string path = SETTING(LOG_DIRECTORY) + Util::formatParams(getSetting(area, FILE), params, false); // [9] https://www.box.net/shared/8d5744849d5f76a255e1
		const string msg = Util::formatParams(getSetting(area, FORMAT), params, false);
		log(path, msg);
	}
}

void LogManager::message(const string& msg, bool p_only_file /*= false */)
{
#if !defined(BETA) && !defined(FLYLINKDC_HE)
	if (BOOLSETTING(LOG_SYSTEM))
#endif
	{
		StringMap params;
		params["message"] = msg;
		log(SYSTEM, params, p_only_file); // [1] https://www.box.net/shared/9e63916273d37e5b2932
	}
	fire(LogManagerListener::Message(), msg);
}

} // namespace dcpp

/**
 * @file
 * $Id: LogManager.cpp 568 2011-07-24 18:28:43Z bigmuscle $
 */
