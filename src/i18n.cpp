#include <string>
#include <iostream>
#include <boost/unordered_map.hpp>
#include "stdint.h"

#include "filesystem.hpp"
#include "module.hpp"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef TARGET_OS_HARMATTAN
#include <gconf/gconf-client.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef TARGET_BLACKBERRY
#include "bps/bps.h"
#include "bps/locale.h"
#include <sstream>
#endif

namespace {

//header structure of the MO file format, as described on
//http://www.gnu.org/software/hello/manual/gettext/MO-Files.html
struct mo_header {
	uint32_t magic;
	uint32_t version;
	uint32_t number;   // number of strings
	uint32_t o_offset; // offset of original string table
	uint32_t t_offset; // offset of translated string table
};

//the original and translated string table consists of
//a number of string length / file offset pairs:
struct mo_entry {
	uint32_t length;
	uint32_t offset;
};

//hashmap to map original to translated strings
typedef boost::unordered_map<std::string, std::string> map;
map hashmap;

std::string locale;

}

namespace i18n {

const std::string& tr(const std::string& msgid) {
	//do not attempt to translate empty strings ("") since that returns metadata
	if (msgid.empty())
		return msgid;
	map::iterator it = hashmap.find (msgid);
	if (it != hashmap.end())
		return it->second;
	//if no translated string was found, return the original
	return msgid;
}

const std::string& get_locale() {
	return locale;
}

	void init() {
#ifdef _WIN32
	{
		char c[1024];
		GetLocaleInfoA(LOCALE_USER_DEFAULT,LOCALE_SISO639LANGNAME,c,1024);
		if(c[0]!='\0'){
			locale=c;
			GetLocaleInfoA(LOCALE_USER_DEFAULT,LOCALE_SISO3166CTRYNAME,c,1024);
			if(c[0]!='\0') locale+=std::string("_")+c;
		}
	}
#endif
		
#ifdef __APPLE__
		CFArrayRef localeIDs = CFLocaleCopyPreferredLanguages();
		if (localeIDs)
		{
			CFStringRef localeID = (CFStringRef)CFArrayGetValueAtIndex(localeIDs, 0);
			char tmp[16];
			if (CFStringGetCString(localeID, tmp, 16, kCFStringEncodingUTF8))
				locale = std::string(tmp);
			CFRelease(localeIDs);
		}
#endif

#if defined(TARGET_OS_HARMATTAN)
	std::cerr << "Get GConf default client\n";
	GConfClient *gconf = gconf_client_get_default();
	locale = std::string(gconf_client_get_string(gconf, "/meegotouch/i18n/region", NULL));
#elif defined(TARGET_BLACKBERRY)
	char *language = 0;
	char *country = 0;
	if (BPS_SUCCESS == locale_get(&language, &country) && language!= NULL && country != NULL) {
		std::stringstream ss;
		ss << language << "_" << country;
		locale = ss.str();

		bps_free(language);
		bps_free(country);
	}
#else
	char *cstr = getenv("LANG");
	if (cstr != NULL)
		locale = cstr;
	if (locale.size() < 2)
	{
		cstr = getenv("LC_ALL");
		if (cstr != NULL)
			locale = cstr;
	}
	
	if (locale == "zh-Hans") locale = "zh_CN"; //hack to make it work on iOS
	if (locale == "zh-Hant") locale = "zh_TW";
#endif
	//strip the charset part of the country and language code,
	//e.g. "pt_BR.UTF8" --> "pt_BR"
	size_t found = locale.find(".");
	if (found != std::string::npos) {
		locale = locale.substr(0, found);
	}
	if (locale.size() < 2)
		return;
	
	std::string filename = "./locale/" + locale + "/LC_MESSAGES/frogatto.mo";
	found = locale.find("@");
	if (!sys::file_exists(filename) && found != std::string::npos) {
		locale = locale.substr(0, found);
		filename = "./locale/" + locale + "/LC_MESSAGES/frogatto.mo";
	}
	//strip the country code, e.g. "de_DE" --> "de"
	found = locale.find("_");
	if (!sys::file_exists(filename) && found != std::string::npos) {
		locale = locale.substr(0, found);
		filename = "./locale/" + locale + "/LC_MESSAGES/frogatto.mo";
	}
	if (!sys::file_exists(filename))
		return;
	const std::string content = sys::read_file(module::map_file(filename));
	size_t size = content.size();
	if (size < sizeof(mo_header))
		return;
	mo_header* header = (mo_header*) content.c_str();
	if (header->magic != 0x950412de ||
	    header->version != 0 ||
	    header->o_offset + 8*header->number > size ||
	    header->t_offset + 8*header->number > size)
		return;
	mo_entry* original = (mo_entry*) (content.c_str() + header->o_offset);
	mo_entry* translated = (mo_entry*) (content.c_str() + header->t_offset);
	for (int i = 0; i < header->number; ++i) {
		if (original[i].offset + original[i].length > size ||
		    translated[i].offset + translated[i].length > size)
			return;
		const std::string msgid = content.substr(original[i].offset, original[i].length);
		const std::string msgstr = content.substr(translated[i].offset, translated[i].length);
		hashmap[msgid] = msgstr;
	}
}

}
