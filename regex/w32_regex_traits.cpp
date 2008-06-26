/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         w32_regex_traits.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Implements w32_regex_traits<char> (and associated helper classes).
  */

#define BOOST_REGEX_SOURCE
#include <boost/regex/config.hpp>

#if defined(_WIN32) && !defined(BOOST_REGEX_NO_W32)
#include <boost/regex/regex_traits.hpp>
#include <boost/regex/pattern_except.hpp>

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#define NOGDI
#include <windows.h>

#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#endif

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{
   using ::memset;
}
#endif

namespace boost{ namespace re_detail{

void w32_regex_traits_char_layer<char>::init() 
{
   // we need to start by initialising our syntax map so we know which
   // character is used for which purpose:
   std::memset(m_char_map, 0, sizeof(m_char_map));
   cat_type cat;
   std::string cat_name(w32_regex_traits<char>::get_catalog_name());
   if(cat_name.size())
   {
      cat = ::boost::re_detail::w32_cat_open(cat_name);
      if(!cat)
      {
         std::string m("Unable to open message catalog: ");
         std::runtime_error err(m + cat_name);
         ::boost::re_detail::raise_runtime_error(err);
      }
   }
   //
   // if we have a valid catalog then load our messages:
   //
   if(cat)
   {
      for(regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
      {
         string_type mss = ::boost::re_detail::w32_cat_get(cat, this->m_locale, i, get_default_syntax(i));
         for(string_type::size_type j = 0; j < mss.size(); ++j)
         {
            m_char_map[static_cast<unsigned char>(mss[j])] = i;
         }
      }
   }
   else
   {
      for(regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
      {
         const char* ptr = get_default_syntax(i);
         while(ptr && *ptr)
         {
            m_char_map[static_cast<unsigned char>(*ptr)] = i;
            ++ptr;
         }
      }
   }
   //
   // finish off by calculating our escape types:
   //
   unsigned char i = 'A';
   do
   {
      if(m_char_map[i] == 0)
      {
         if(::boost::re_detail::w32_is(this->m_locale, 0x0002u, (char)i)) 
            m_char_map[i] = regex_constants::escape_type_class;
         else if(::boost::re_detail::w32_is(this->m_locale, 0x0001u, (char)i)) 
            m_char_map[i] = regex_constants::escape_type_not_class;
      }
   }while(0xFF != i++);

   //
   // fill in lower case map:
   //
   char char_map[1 << CHAR_BIT];
   for(int ii = 0; ii < (1 << CHAR_BIT); ++ii)
      char_map[ii] = static_cast<char>(ii);
   int r = ::LCMapStringA(this->m_locale, LCMAP_LOWERCASE, char_map, 1 << CHAR_BIT, this->m_lower_map, 1 << CHAR_BIT);
   BOOST_ASSERT(r != 0);
   if(r < (1 << CHAR_BIT))
   {
      // if we have multibyte characters then not all may have been given
      // a lower case mapping:
      for(int jj = r; jj < (1 << CHAR_BIT); ++jj)
         this->m_lower_map[jj] = static_cast<char>(jj);
   }
   r = ::GetStringTypeExA(this->m_locale, CT_CTYPE1, char_map, 1 << CHAR_BIT, this->m_type_map);
   BOOST_ASSERT(0 != r);
}

BOOST_REGEX_DECL lcid_type BOOST_REGEX_CALL w32_get_default_locale()
{
   return ::GetUserDefaultLCID();
}

BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_lower(char c, lcid_type id)
{
   WORD mask;
   if(::GetStringTypeExA(id, CT_CTYPE1, &c, 1, &mask) && (mask & C1_LOWER))
      return true;
   return false;
}

BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_lower(wchar_t c, lcid_type id)
{
   WORD mask;
   if(::GetStringTypeExW(id, CT_CTYPE1, &c, 1, &mask) && (mask & C1_LOWER))
      return true;
   return false;
}
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_lower(unsigned short ca, lcid_type id)
{
   WORD mask;
   wchar_t c = ca;
   if(::GetStringTypeExW(id, CT_CTYPE1, &c, 1, &mask) && (mask & C1_LOWER))
      return true;
   return false;
}
#endif

BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_upper(char c, lcid_type id)
{
   WORD mask;
   if(::GetStringTypeExA(id, CT_CTYPE1, &c, 1, &mask) && (mask & C1_UPPER))
      return true;
   return false;
}

BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_upper(wchar_t c, lcid_type id)
{
   WORD mask;
   if(::GetStringTypeExW(id, CT_CTYPE1, &c, 1, &mask) && (mask & C1_UPPER))
      return true;
   return false;
}
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_upper(unsigned short ca, lcid_type id)
{
   WORD mask;
   wchar_t c = ca;
   if(::GetStringTypeExW(id, CT_CTYPE1, &c, 1, &mask) && (mask & C1_UPPER))
      return true;
   return false;
}
#endif

void free_module(void* mod)
{
   ::FreeLibrary(static_cast<HMODULE>(mod));
}

BOOST_REGEX_DECL cat_type BOOST_REGEX_CALL w32_cat_open(const std::string& name)
{
   cat_type result(::LoadLibraryA(name.c_str()), &free_module);
   return result;
}

BOOST_REGEX_DECL std::string BOOST_REGEX_CALL w32_cat_get(const cat_type& cat, lcid_type, int i, const std::string& def)
{
   char buf[256];
   if(0 == ::LoadStringA(
      static_cast<HMODULE>(cat.get()),
      i,
      buf,
      256
   ))
   {
      return def;
   }
   return std::string(buf);
}

#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL std::wstring BOOST_REGEX_CALL w32_cat_get(const cat_type& cat, lcid_type, int i, const std::wstring& def)
{
   wchar_t buf[256];
   if(0 == ::LoadStringW(
      static_cast<HMODULE>(cat.get()),
      i,
      buf,
      256
   ))
   {
      return def;
   }
   return std::wstring(buf);
}
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL std::basic_string<unsigned short> BOOST_REGEX_CALL w32_cat_get(const cat_type& cat, lcid_type, int i, const std::basic_string<unsigned short>& def)
{
   unsigned short buf[256];
   if(0 == ::LoadStringW(
      static_cast<HMODULE>(cat.get()),
      i,
      (LPWSTR)buf,
      256
   ))
   {
      return def;
   }
   return std::basic_string<unsigned short>(buf);
}
#endif
#endif
BOOST_REGEX_DECL std::string BOOST_REGEX_CALL w32_transform(lcid_type id, const char* p1, const char* p2)
{
   int bytes = ::LCMapStringA(
      id,       // locale identifier
      LCMAP_SORTKEY,  // mapping transformation type
      p1,  // source string
      static_cast<int>(p2 - p1),        // number of characters in source string
      0,  // destination buffer
      0        // size of destination buffer
      );
   if(!bytes)
      return std::string(p1, p2);
   std::string result(++bytes, '\0');
   bytes = ::LCMapStringA(
      id,       // locale identifier
      LCMAP_SORTKEY,  // mapping transformation type
      p1,  // source string
      static_cast<int>(p2 - p1),        // number of characters in source string
      &*result.begin(),  // destination buffer
      bytes        // size of destination buffer
      );
   if(bytes > static_cast<int>(result.size()))
      return std::string(p1, p2);
   while(result.size() && result[result.size()-1] == '\0')
   {
      result.erase(result.size()-1);
   }
   return result;
}

#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL std::wstring BOOST_REGEX_CALL w32_transform(lcid_type id, const wchar_t* p1, const wchar_t* p2)
{
   int bytes = ::LCMapStringW(
      id,       // locale identifier
      LCMAP_SORTKEY,  // mapping transformation type
      p1,  // source string
      static_cast<int>(p2 - p1),        // number of characters in source string
      0,  // destination buffer
      0        // size of destination buffer
      );
   if(!bytes)
      return std::wstring(p1, p2);
   std::string result(++bytes, '\0');
   bytes = ::LCMapStringW(
      id,       // locale identifier
      LCMAP_SORTKEY,  // mapping transformation type
      p1,  // source string
      static_cast<int>(p2 - p1),        // number of characters in source string
      reinterpret_cast<wchar_t*>(&*result.begin()),  // destination buffer *of bytes*
      bytes        // size of destination buffer
      );
   if(bytes > static_cast<int>(result.size()))
      return std::wstring(p1, p2);
   while(result.size() && result[result.size()-1] == L'\0')
   {
      result.erase(result.size()-1);
   }
   std::wstring r2;
   for(std::string::size_type i = 0; i < result.size(); ++i)
      r2.append(1, static_cast<wchar_t>(static_cast<unsigned char>(result[i])));
   return r2;
}
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL std::basic_string<unsigned short> BOOST_REGEX_CALL w32_transform(lcid_type id, const unsigned short* p1, const unsigned short* p2)
{
   int bytes = ::LCMapStringW(
      id,       // locale identifier
      LCMAP_SORTKEY,  // mapping transformation type
      (LPCWSTR)p1,  // source string
      static_cast<int>(p2 - p1),        // number of characters in source string
      0,  // destination buffer
      0        // size of destination buffer
      );
   if(!bytes)
      return std::basic_string<unsigned short>(p1, p2);
   std::string result(++bytes, '\0');
   bytes = ::LCMapStringW(
      id,       // locale identifier
      LCMAP_SORTKEY,  // mapping transformation type
      (LPCWSTR)p1,  // source string
      static_cast<int>(p2 - p1),        // number of characters in source string
      reinterpret_cast<wchar_t*>(&*result.begin()),  // destination buffer *of bytes*
      bytes        // size of destination buffer
      );
   if(bytes > static_cast<int>(result.size()))
      return std::basic_string<unsigned short>(p1, p2);
   while(result.size() && result[result.size()-1] == L'\0')
   {
      result.erase(result.size()-1);
   }
   std::basic_string<unsigned short> r2;
   for(std::string::size_type i = 0; i < result.size(); ++i)
      r2.append(1, static_cast<unsigned short>(static_cast<unsigned char>(result[i])));
   return r2;
}
#endif
#endif
BOOST_REGEX_DECL char BOOST_REGEX_CALL w32_tolower(char c, lcid_type id)
{
   char result[2];
   int b = ::LCMapStringA(
      id,       // locale identifier
      LCMAP_LOWERCASE,  // mapping transformation type
      &c,  // source string
      1,        // number of characters in source string
      result,  // destination buffer
      1);        // size of destination buffer
   if(b == 0)
      return c;
   return result[0];
}

#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL wchar_t BOOST_REGEX_CALL w32_tolower(wchar_t c, lcid_type id)
{
   wchar_t result[2];
   int b = ::LCMapStringW(
      id,       // locale identifier
      LCMAP_LOWERCASE,  // mapping transformation type
      &c,  // source string
      1,        // number of characters in source string
      result,  // destination buffer
      1);        // size of destination buffer
   if(b == 0)
      return c;
   return result[0];
}
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL unsigned short BOOST_REGEX_CALL w32_tolower(unsigned short c, lcid_type id)
{
   wchar_t result[2];
   int b = ::LCMapStringW(
      id,       // locale identifier
      LCMAP_LOWERCASE,  // mapping transformation type
      (wchar_t const*)&c,  // source string
      1,        // number of characters in source string
      result,  // destination buffer
      1);        // size of destination buffer
   if(b == 0)
      return c;
   return result[0];
}
#endif
#endif
BOOST_REGEX_DECL char BOOST_REGEX_CALL w32_toupper(char c, lcid_type id)
{
   char result[2];
   int b = ::LCMapStringA(
      id,       // locale identifier
      LCMAP_UPPERCASE,  // mapping transformation type
      &c,  // source string
      1,        // number of characters in source string
      result,  // destination buffer
      1);        // size of destination buffer
   if(b == 0)
      return c;
   return result[0];
}

#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL wchar_t BOOST_REGEX_CALL w32_toupper(wchar_t c, lcid_type id)
{
   wchar_t result[2];
   int b = ::LCMapStringW(
      id,       // locale identifier
      LCMAP_UPPERCASE,  // mapping transformation type
      &c,  // source string
      1,        // number of characters in source string
      result,  // destination buffer
      1);        // size of destination buffer
   if(b == 0)
      return c;
   return result[0];
}
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL unsigned short BOOST_REGEX_CALL w32_toupper(unsigned short c, lcid_type id)
{
   wchar_t result[2];
   int b = ::LCMapStringW(
      id,       // locale identifier
      LCMAP_UPPERCASE,  // mapping transformation type
      (wchar_t const*)&c,  // source string
      1,        // number of characters in source string
      result,  // destination buffer
      1);        // size of destination buffer
   if(b == 0)
      return c;
   return result[0];
}
#endif
#endif
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is(lcid_type id, boost::uint32_t m, char c)
{
   WORD mask;
   if(::GetStringTypeExA(id, CT_CTYPE1, &c, 1, &mask) && (mask & m & w32_regex_traits_implementation<char>::mask_base))
      return true;
   if((m & w32_regex_traits_implementation<char>::mask_word) && (c == '_'))
      return true;
   return false;
}

#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is(lcid_type id, boost::uint32_t m, wchar_t c)
{
   WORD mask;
   if(::GetStringTypeExW(id, CT_CTYPE1, &c, 1, &mask) && (mask & m & w32_regex_traits_implementation<wchar_t>::mask_base))
      return true;
   if((m & w32_regex_traits_implementation<wchar_t>::mask_word) && (c == '_'))
      return true;
   if((m & w32_regex_traits_implementation<wchar_t>::mask_unicode) && (c > 0xff))
      return true;
   return false;
}
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is(lcid_type id, boost::uint32_t m, unsigned short c)
{
   WORD mask;
   if(::GetStringTypeExW(id, CT_CTYPE1, (wchar_t const*)&c, 1, &mask) && (mask & m & w32_regex_traits_implementation<wchar_t>::mask_base))
      return true;
   if((m & w32_regex_traits_implementation<wchar_t>::mask_word) && (c == '_'))
      return true;
   if((m & w32_regex_traits_implementation<wchar_t>::mask_unicode) && (c > 0xff))
      return true;
   return false;
}
#endif
#endif

} // re_detail
} // boost

#endif

