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

#ifndef DCPLUSPLUS_DCPP_STDINC_H
#define DCPLUSPLUS_DCPP_STDINC_H


#include "compiler.h"

#ifndef _DEBUG
# define BOOST_DISABLE_ASSERTS 1
#endif

#define PPA_INCLUDE_VERSION_CHECK

#ifdef USE_RIP_MINIDUMP
#define NIGHT_ORION_SEND_STACK_DUMP_TO_SERVER
#endif


#ifndef BZ_NO_STDIO
#define BZ_NO_STDIO 1
#endif

#ifdef _WIN32
#include "w.h"
#else
#include <unistd.h>
#define BOOST_PTHREAD_HAS_MUTEXATTR_SETTYPE
#endif

#include <wchar.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <sys/types.h>
#include <time.h>
#include <locale.h>
#ifndef _MSC_VER
#include <stdint.h>
#endif

#include <algorithm>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <numeric>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifndef _CONSOLE // MakeDefs

#ifdef _DEBUG
#include <boost/noncopyable.hpp>
#endif

#define BOOST_ALL_NO_LIB 1

#endif // MakeDefs


#include <regex>

namespace dcpp
{
using namespace std;

inline int stricmp(const string& a, const string& b)
{
	return _stricmp(a.c_str(), b.c_str());
}
inline int strnicmp(const string& a, const string& b, size_t n)
{
	return _strnicmp(a.c_str(), b.c_str(), n);
}
inline int stricmp(const wstring& a, const wstring& b)
{
	return _wcsicmp(a.c_str(), b.c_str());
}
inline int strnicmp(const wstring& a, const wstring& b, size_t n)
{
	return _wcsnicmp(a.c_str(), b.c_str(), n);
}
}


#endif // !defined(STDINC_H)

/**
 * @file
 * $Id: stdinc.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
