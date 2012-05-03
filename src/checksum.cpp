#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "asserts.hpp"
#include "checksum.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "md5.hpp"
#include "json_parser.hpp"
#include "unit_test.hpp"
#include "variant.hpp"

namespace checksum {

namespace {
bool verified = false;
std::map<std::string, std::string> hashes;
std::string whole_game_signature;
}

manager::manager() {
	try {
		whole_game_signature = md5::sum(sys::read_file("./signature.cfg"));
		variant v = json::parse_from_file("./signature.cfg");

		if(!v.is_map()) {
			verified = false;
			return;
		}

		std::vector<std::string> keys = v.get_keys().as_list_string();
		std::vector<std::string> values = v.get_values().as_list_string();
		ASSERT_EQ(keys.size(), values.size());
		for(int n = 0; n != keys.size(); ++n) {
			hashes[keys[n]] = values[n];
		}

		verified = true;
	} catch(...) {
		verified = false;
	}
}

manager::~manager() {
	std::cerr << "EXITING WITH " << (verified ? "VERIFIED" : "UNVERIFIED") << " SESSION\n";
}

const std::string& game_signature()
{
	return whole_game_signature;
}

bool is_verified()
{
	return verified;
}

namespace {
bool both_slashes(char a, char b) {
	return a == '/' && b == '/';
}
}

void verify_file(const std::string& fname_input, const std::string& contents)
{
	if(!verified) {
		return;
	}

	std::string fname = fname_input;
	fname.erase(std::unique(fname.begin(), fname.end(), both_slashes), fname.end());

	if(fname.size() < 5 || std::string(fname.begin(), fname.begin()+5) != "data/") {
		return;
	}

	const std::map<std::string,std::string>::const_iterator itor = hashes.find(fname);
	if(itor == hashes.end()) {
		if(!contents.empty()) {
			std::cerr << "UNVERIFIED NEW FILE: " << fname << "\n";
			verified = false;
		}
		return;
	}

	verified = md5::sum(contents) == itor->second;
	if(!verified) {
		std::cerr << "UNVERIFIED FILE: " << fname << "\n";
	}
}

}

namespace {
void get_signatures(const std::string& dir, std::map<std::string, std::string>* results)
{
	std::vector<std::string> files, dirs;
	sys::get_files_in_dir(dir, &files, &dirs);
	foreach(const std::string& d, dirs) {
		get_signatures(dir + "/" + d, results);
	}

	foreach(const std::string& fname, files) {
		const std::string path = dir + "/" + fname;
		const std::string md5sum = md5::sum(sys::read_file(path));
		(*results)[path] = md5sum;
	}
}
}

COMMAND_LINE_UTILITY(sign_game_data)
{
	std::map<std::string,std::string> signatures;
	get_signatures("data", &signatures);

	std::map<variant,variant> output;
	for(std::map<std::string,std::string>::const_iterator i = signatures.begin(); i != signatures.end(); ++i) {
		output[variant(i->first)] = variant(i->second);
	}

	sys::write_file("signature.cfg", variant(&output).write_json());
}
