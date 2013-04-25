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

#ifndef DCPLUSPLUS_DCPP_W_H_
#define DCPLUSPLUS_DCPP_W_H_

#ifdef _WIN32

#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0600
#endif

#ifndef _WIN32_IE
# define _WIN32_IE  0x0501
#endif

#ifndef WINVER
# define WINVER 0x501
#endif

#ifndef STRICT
#define STRICT 1
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX 1
#endif

#include <windows.h>
#include <mmsystem.h>
#include <tchar.h>
#include "CFlyProfiler.h"

class CFlySafeGuard
{
		volatile LONG& m_flag;
	public:
		CFlySafeGuard(volatile LONG& p_flag) : m_flag(p_flag)
		{
			InterlockedIncrement(&m_flag);
		}
		~CFlySafeGuard()
		{
			InterlockedDecrement(&m_flag);
		}
};

#endif

#endif /* W_H_ */
