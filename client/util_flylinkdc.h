/*
 * Copyright (C) 2011-2012 FlylinkDC++ Team
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

#ifndef DCPLUSPLUS_DCPP_UTIL_FLYLINKDC_H
#define DCPLUSPLUS_DCPP_UTIL_FLYLINKDC_H


template <class T> inline void safe_delete(T* & p)
{
	if (p != nullptr)
	{
		delete p;
		p = 0;
	}
}
template <class T> inline void safe_delete_array(T* & p)
{
	if (p != nullptr)
	{
		delete [] p;
		p = 0;
	}
}
template <class T> inline void safe_release(T* & p)
{
	if (p != nullptr)
	{
		p->Release();
		p = 0;
	}
}
template <class T> inline void DestroyAndDetachWindow(T & p)
{
	if (p != nullptr)
	{
		p.DestroyWindow();
		p.Detach();
	}
}

#endif // DCPLUSPLUS_DCPP_COMPILER_FLYLINKDC_H
