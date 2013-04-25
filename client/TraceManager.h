/*
 * Copyright (C) 2001-2007 Jacek Sieka, j_s@telia.com
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

#if !defined(DCPLUSPLUS_DCPP_TRACE_MANAGER_H)
#define DCPLUSPLUS_DCPP_TRACE_MANAGER_H

#pragma once

namespace dcpp
{
#ifdef FLYLINKDC_TRACE_ENABLE

#include "Thread.h"
#include "File.h"


class TraceManager : public Singleton<TraceManager>
{
	public:
		void CDECL trace_print(const char* format, ...) noexcept;
		void CDECL trace_start(const char* format, ...) noexcept;
		void CDECL trace_end(const char* format, ...) noexcept;
		
	private:
	
		void  print(const string& msg);
		
		friend class Singleton<TraceManager>;
		CriticalSection cs;
		
		File* f;
		unordered_map<DWORD, int> indents;
		
		TraceManager()
		{
			f = new File("trace.log", File::WRITE, File::OPEN | File::CREATE);
			f->setEndPos(0);
		}
		~TraceManager()
		{
			delete f;
		}
		
};

#define TracePrint TraceManager::getInstance()->trace_print
#define TraceStart TraceManager::getInstance()->trace_start
#define TraceEnd TraceManager::getInstance()->trace_end
#else
void TracePrint(const char* format, ...)  {}
void TraceStart(const char* format, ...)  {}
void TraceEnd(const char* format, ...)  {}

#endif // FLYLINKDC_TRACE_ENABLE

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_TRACE_MANAGER_H)

/**
 * @file
 * $Id: TraceManager.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
