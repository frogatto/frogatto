#include "asserts.hpp"
#include "concurrent_cache.hpp"
#include "filesystem.hpp"
#include "level.hpp"
#include "load_level.hpp"
#include "preferences.hpp"
#include "preprocessor.hpp"
#include "wml_parser.hpp"

namespace {
typedef concurrent_cache<std::string, wml::const_node_ptr> level_wml_map;
level_wml_map& wml_cache() {
	static level_wml_map instance;
	return instance;
}

class wml_loader {
	std::string lvl_;
public:
	wml_loader(const std::string& lvl) : lvl_(lvl)
	{}
	void operator()() {
		static const std::string path = preferences::load_compiled() ? "data/compiled/level/" : preferences::level_path();
		const std::string filename = path + lvl_;
		wml::const_node_ptr node(wml::parse_wml_from_file(filename));
		wml_cache().put(lvl_, node);
	}
};
}

void clear_level_wml()
{
	wml_cache().clear();
}

void preload_level_wml(const std::string& lvl)
{
	if(lvl == "save.cfg" || lvl == "autosave.cfg") {
		return;
	}

	if(wml_cache().count(lvl)) {
		return;
	}

	wml_loader loader(lvl);
	loader();
}

wml::const_node_ptr load_level_wml(const std::string& lvl)
{
	if(lvl == "save.cfg" || lvl == "autosave.cfg") {
		std::string filename;
		if(lvl == "save.cfg") {
			filename = preferences::save_file_path();
		} else {
			filename = preferences::auto_save_file_path();
		}

		return wml::parse_wml(preprocess(sys::read_file(filename)));
	}

	if(wml_cache().count(lvl)) {
		return wml_cache().get(lvl);
	}

	wml_loader loader(lvl);
	loader();
	return load_level_wml_nowait(lvl);
}

wml::const_node_ptr load_level_wml_nowait(const std::string& lvl)
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
	sys::get_files_in_dir(preferences::level_path(), &files);
	files.erase(std::remove_if(files.begin(), files.end(), not_cfg_file), files.end());
	return files;
}
