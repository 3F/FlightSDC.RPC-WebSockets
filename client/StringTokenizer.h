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

#ifndef DCPLUSPLUS_CLIENT_STRING_TOKENIZER_H
#define DCPLUSPLUS_CLIENT_STRING_TOKENIZER_H

namespace dcpp
{

template<class T>
class StringTokenizer
{
	private:
		vector<T> tokens;
	public:
		explicit StringTokenizer(const T& aString, const typename T::value_type& aToken)
		{
			T::size_type i = 0;
			T::size_type j = 0;
			while ((i = aString.find(aToken, j)) != T::npos)
			{
				tokens.push_back(aString.substr(j, i - j));
				j = i + 1;
			}
			if (j < aString.size())
				tokens.push_back(aString.substr(j, aString.size() - j)); // 2012-05-03_22-05-14_YNJS7AEGAWCUMRBY2HTUTLYENU4OS2PKNJXT6ZY_F4B220A1_crash-stack-r502-beta24-x64-build-9900.dmp
		}
		
		explicit StringTokenizer(const T& aString, const typename T::value_type* aToken)
		{
			T::size_type i = 0;
			T::size_type j = 0;
			size_t l = strlen(aToken);
			while ((i = aString.find(aToken, j)) != T::npos)
			{
				tokens.push_back(aString.substr(j, i - j));
				j = i + l;
			}
			if (j < aString.size())
				tokens.push_back(aString.substr(j, aString.size() - j));
		}
		
		const vector<T>& getTokens() const
		{
			return tokens;
		}
};

} // namespace dcpp

#endif // !DCPLUSPLUS_CLIENT_STRING_TOKENIZER_H

/**
 * @file
 * $Id: StringTokenizer.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
