//-----------------------------------------------------------------------------
//(c) 2013 pavel.pimenov@gmail.com
//-----------------------------------------------------------------------------

#include "stdinc.h"

#ifdef _DEBUG
#include <fstream>
#include "CFlyProfiler.h"
//========================================================
std::map<std::string, unsigned __int64> CFlyProfiler::g_entry_counter;
FastCriticalSection CFlyProfiler::g_cs;
//========================================================
CFlyProfiler g_fly_profiler;
//========================================================
CFlyProfiler::CFlyProfiler()
{
}
CFlyProfiler::~CFlyProfiler()
{
	FastLock l(g_cs);
	std::map<unsigned __int64, std::string> l_stat;
	for (auto i = g_entry_counter.begin(); i != g_entry_counter.end(); ++i)
	{
		std::string & l_item = l_stat[i->second];
		l_item += "\t\t[ " + i->first + "]\r\n";
	}
	std::fstream l_log_out("flylinkdc-profiler-report.txt", std::ios_base::out | std::ios_base::trunc);
	for (auto j = l_stat.begin(); j != l_stat.end(); ++j)
	{
		l_log_out << "count = " << j->first << " file = " << j->second << std::endl;
	}
}
void CFlyProfiler::add(const char* p_file, long p_line,  const char* p_func)
{
	FastLock l(g_cs);
	string l_key =  p_file;
	l_key += "  ";
//	l_key += p_line;
//	l_key += "  ";
	l_key += p_func;
	++g_entry_counter[l_key];
}
#endif

