#include "asserts.hpp"
#include "concurrent_cache.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "json_parser.hpp"
#include "level.hpp"
#include "load_level.hpp"
#include "module.hpp"
#include "preferences.hpp"
#include "preprocessor.hpp"
#include "string_utils.hpp"
#include "variant.hpp"

namespace {
typedef concurrent_cache<std::string, variant> level_wml_map;
level_wml_map& wml_cache() {
	static level_wml_map instance;
	return instance;
}

std::map<std::string,std::string>& get_level_paths() {
	static std::map<std::string,std::string> res;
	return res;
}
}

namespace loadlevel {
void reload_level_paths() {
	wml_cache().clear();
	get_level_paths().clear();
	load_level_paths();
}
void load_level_paths() {
	module::get_unique_filenames_under_dir("data/level/", &get_level_paths());
}

const std::string& get_level_path(const std::string& name) {
	if(get_level_paths().empty()) {
		loadlevel::load_level_paths();
	}
	std::map<std::string, std::string>::const_iterator itor = module::find(get_level_paths(), name);
	if(itor == get_level_paths().end()) {
		std::cerr << "FILE NOT FOUND: " << name << std::endl;
		ASSERT_LOG(false, "FILE NOT FOUND: " << name);
	}
	return itor->second;
}
}

namespace {
class wml_loader {
	std::string lvl_;
public:
	wml_loader(const std::string& lvl) : lvl_(lvl)
	{}
	void operator()() {
		const std::string& filename = loadlevel::get_level_path(lvl_);
		try {
			variant node(json::parse_from_file(filename));
			wml_cache().put(lvl_, node);
		} catch(...) {
			std::cerr << "FAILED TO LOAD " << lvl_ << " -> " << filename << "\n";
			ASSERT_LOG(false, "FAILED TO LOAD");
		}
	}
};
}

void clear_level_wml()
{
	wml_cache().clear();
}

void preload_level_wml(const std::string& lvl)
{
	if(lvl == "autosave.cfg" || (lvl.substr(0,4) == "save" && lvl.substr(lvl.length()-4) == ".cfg")) {
		return;
	}

	if(wml_cache().count(lvl)) {
		return;
	}

	wml_loader loader(lvl);
	loader();
}

variant load_level_wml(const std::string& lvl)
{
	if(lvl == "tmp_state.cfg") {
		//special state for debugging.
		return json::parse_from_file("./tmp_state.cfg");
	}

	if(lvl == "autosave.cfg" || (lvl.substr(0,4) == "save" && lvl.substr(lvl.length()-4) == ".cfg")) {
		std::string filename;
		if(lvl == "autosave.cfg") {
			filename = preferences::auto_save_file_path();
		} else {
			filename = preferences::save_file_path();
		}

		return json::parse_from_file(filename);
	}

	if(wml_cache().count(lvl)) {
		return wml_cache().get(lvl);
	}

	wml_loader loader(lvl);
	loader();
	return load_level_wml_nowait(lvl);
}

variant load_level_wml_nowait(const std::string& lvl)
{
	return wml_cache().get(lvl);
}

load_level_manager::load_level_manager()
{
}

load_level_manager::~load_level_manager()
{
}

void preload_level(const std::string& lvl)
{
}

level* load_level(const std::string& lvl)
{
	level* res = new level(lvl);
	res->finish_loading();
	return res;
}

namespace {
bool not_cfg_file(const std::string& filename) {
	return filename.size() < 4 || !std::equal(filename.end() - 4, filename.end(), ".cfg");
}
}

std::vector<std::string> get_known_levels()
{
	std::vector<std::string> files;
	std::map<std::string, std::string> file_map;
	std::map<std::string, std::string>::iterator it;
	if(preferences::is_level_path_set()) {
		sys::get_unique_filenames_under_dir(preferences::level_path(), &file_map, "");
	} else {
		module::get_unique_filenames_under_dir(preferences::level_path(), &file_map);
	}
	for(it = file_map.begin(); it != file_map.end(); ) {
		if(not_cfg_file(it->first)) {
			file_map.erase(it++);
		} else { 
			++it; 
		}
	}

	std::pair<std::string, std::string> file;
	foreach(file, file_map) {
		files.push_back(file.first);
	}
	std::sort(files.begin(), files.end());
	return files;
}
