/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef COMPAT_HPP_INCLUDED
#define COMPAT_HPP_INCLUDED

#ifdef _WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#define strtoll _strtoui64
#define round( num )    ( floor( ( num ) + 0.5 ) )

_Check_return_ inline bool __isblank(_In_ int _C) 
    { return (MB_CUR_MAX > 1 ? _isctype(_C,_BLANK) : __chvalidchk(_C, _BLANK)); }

#endif // _WINDOWS


#endif // COMPAT_HPP_INCLUDED
