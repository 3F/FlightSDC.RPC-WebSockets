//-----------------------------------------------------------------------------
//(c) 2013 pavel.pimenov@gmail.com
//-----------------------------------------------------------------------------
#ifndef CFlyProfiler_H
#define CFlyProfiler_H

#pragma once

#ifdef _DEBUG

#include <string>
#include <map>
#include "Thread.h"

class CFlyProfiler
{
		static std::map<std::string, unsigned __int64> g_entry_counter;
		static FastCriticalSection g_cs;
	public:
		CFlyProfiler();
		~CFlyProfiler();
		void add(const char* p_file, long p_line, const char* p_func);
};
#define FLYLINKDC_PROFILER \
	extern CFlyProfiler g_fly_profiler; \
	g_fly_profiler.add(__FILE__,__LINE__, __FUNCTION__);
#else
#define FLYLINKDC_PROFILER
#endif

#endif
