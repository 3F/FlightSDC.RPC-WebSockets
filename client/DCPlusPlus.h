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

#ifndef DCPLUSPLUS_DCPP_DCPLUSPLUS_H
#define DCPLUSPLUS_DCPP_DCPLUSPLUS_H

#include "compiler.h"
#include "typedefs.h"
// [+] SSA - uncomment to use mediainfo.dll (not static)
// #define USE_MEDIAINFO_DLL
// [+] SSA - Remove needless words from MediaInfo
#define SSA_REMOVE_NEEDLESS_WORDS_FROM_VIDEO_AUDIO_INFO
// [+] SSA - Use OverallBitrate in audio info
// #define SSA_USE_OVERALBITRATE_IN_AUDIO_INFO


// Make sure we're using the templates from algorithm...
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace dcpp
{

extern void startup(void (*f)(void*, const tstring&), void* p);
extern void shutdown();

} // namespace dcpp

//[+]PPA
extern bool g_isWineCheck;

#endif // !defined(DC_PLUS_PLUS_H)

/**
 * @file
 * $Id: DCPlusPlus.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
