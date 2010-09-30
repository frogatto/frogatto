#include <string>
#include <iostream>
#include <boost/unordered_map.hpp>

#include "filesystem.hpp"

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

}

namespace i18n {

const std::string& tr(const std::string& msgid) {
	map::iterator it = hashmap.find (msgid);
	if (it != hashmap.end())
		return it->second;
	//if no translated string was found, return the original
	return msgid;
}

void init() {
	std::string locale;
	char *cstr = getenv("LANG");
	if (cstr != NULL)
		locale = cstr;
	if (locale.size() < 2)
	{
		cstr = getenv("LC_ALL");
		if (cstr != NULL)
			locale = cstr;
	}
	if (locale.size() < 2)
		return;
	
	//only consider the country and language code,
	//e.g. "pt_BR.UTF8" --> "pt_BR" or "de_DE.UTF8" --> "de"
	std::string filename;
	if (locale.size() >= 5) {
		locale = locale.substr(0, 5);
		filename = "./locale/" + locale + "/LC_MESSAGES/frogatto.mo";
		if (!sys::file_exists(filename)) {
			locale = locale.substr(0, 2);
			filename = "./locale/" + locale + "/LC_MESSAGES/frogatto.mo";
			if (!sys::file_exists(filename))
				return;
		}
	} else {
		locale = locale.substr(0, 2);
		filename = "./locale/" + locale + "/LC_MESSAGES/frogatto.mo";
		if (!sys::file_exists(filename))
			return;
	}
	const std::string content = sys::read_file(filename);
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
