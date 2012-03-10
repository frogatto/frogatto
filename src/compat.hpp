#ifndef COMPAT_HPP_INCLUDED
#define COMPAT_HPP_INCLUDED

#ifdef _WINDOWS
#define strtoll _strtoui64
#define round( num )    ( floor( ( num ) + 0.5 ) )

_Check_return_ inline bool __isblank(_In_ int _C) 
    { return (MB_CUR_MAX > 1 ? _isctype(_C,_BLANK) : __chvalidchk(_C, _BLANK)); }

#endif // _WINDOWS


#endif // COMPAT_HPP_INCLUDED
