#include <assert.h>

#include "asserts.hpp"
#include "concurrent_cache.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "json_parser.hpp"
#include "level.hpp"
#include "load_level.hpp"
#include "package.hpp"
#include "preferences.hpp"
#include "preprocessor.hpp"
#include "string_utils.hpp"
#include "texture.hpp"
#include "thread.hpp"
#include "variant.hpp"

namespace {
typedef concurrent_cache<std::string, variant> level_wml_map;
level_wml_map& wml_cache() {
	static level_wml_map instance;
	return instance;
}

std::map<std::string, threading::thread*>& wml_threads()
{
	static std::map<std::string, threading::thread*> instance;
	return instance;
}

class wml_loader {
	std::string lvl_;
public:
	wml_loader(const std::string& lvl) : lvl_(lvl)
	{}
	void operator()() {
		static const std::string global_path = preferences::load_compiled() ? "data/compiled/level/" : preferences::level_path();

		std::string filename;

		std::vector<std::string> components = util::split(lvl_, '/');
		if(components.size() == 1) {
			filename = global_path + lvl_;
		} else if(components.size() == 2) {
			filename = std::string(preferences::user_data_path()) + "/packages/" + components.front() + "/" + components.back();
		} else {
			ASSERT_LOG(false, "UNRECOGNIZED LEVEL PATH FORMAT: " << lvl_);
		}

		try {
			variant node(json::parse_from_file(filename));
			wml_cache().put(lvl_, node);
		} catch(json::parse_error& e) {
			ASSERT_LOG(false, "ERROR PARSING LEVEL WML FOR '" << filename << "': " << e.error_message());
		}catch(...) {
			std::cerr << "FAILED TO LOAD " << filename << "\n";
			ASSERT_LOG(false, "FAILED TO LOAD");
		}
	}
};
}

void clear_level_wml()
{
	wml_cache().clear();
}

namespace {
bool is_save_file(const std::string& fname)
{
	static const std::string SaveFiles[] = {"save.cfg", "save2.cfg", "save3.cfg", "save4.cfg"};
	return std::count(SaveFiles, SaveFiles + sizeof(SaveFiles)/sizeof(SaveFiles[0]), fname) != 0;
}
}

void preload_level_wml(const std::string& lvl)
{
	if(is_save_file(lvl) || lvl == "autosave.cfg") {
		return;
	}

	if(wml_cache().count(lvl)) {
		return;
	}

	wml_cache().put(lvl, variant());
	wml_threads()[lvl] = new threading::thread(wml_loader(lvl));
}

variant load_level_wml(const std::string& lvl)
{
	if(lvl == "tmp_state.cfg") {
		//special state for debugging.
		return json::parse_from_file("./tmp_state.cfg");
	}

	if(is_save_file(lvl) || lvl == "autosave.cfg") {
		std::string filename;
		if(is_save_file(lvl)) {
			filename = std::string(preferences::user_data_path()) + "/" + lvl;
		} else {
			filename = preferences::auto_save_file_path();
		}

		return json::parse_from_file(filename);
	}

	if(wml_cache().count(lvl)) {
		std::map<std::string, threading::thread*>::iterator t = wml_threads().find(lvl);
		if(t != wml_threads().end()) {
			delete t->second;
			wml_threads().erase(t);
		}

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

namespace {
typedef std::map<std::string, std::pair<threading::thread*, level*> > level_map;
level_map levels_loading;

threading::mutex& levels_loading_mutex() {
	static threading::mutex m;
	return m; 
}

class level_loader {
	std::string lvl_;
public:
	level_loader(const std::string& lvl) : lvl_(lvl)
	{}
	void operator()() {
		level* lvl = NULL;
		try {
			lvl = new level(lvl_);
		} catch(const graphics::texture::worker_thread_error&) {
			//we can't load the level in here, we must do it in the main thread.
			std::cerr << "LOAD LEVEL FAILURE: " << lvl << " MUST LOAD IN "
			             "MAIN THREAD\n";
		}
		threading::lock lck(levels_loading_mutex());
		levels_loading[lvl_].second = lvl;
	}
};

}

load_level_manager::load_level_manager()
{}

load_level_manager::~load_level_manager()
{
	for(level_map::iterator i = levels_loading.begin(); i != levels_loading.end(); ++i) {
		i->second.first->join();
		delete i->second.first;
		delete i->second.second;
	}

	levels_loading.clear();
}

void preload_level(const std::string& lvl)
{
	//--TODO: Currently multi-threaded pre-loading causes weird crashes.
	//--      need to fix this!!
	assert(!lvl.empty());
	threading::lock lck(levels_loading_mutex());
	if(levels_loading.count(lvl) == 0) {
		levels_loading[lvl].first = new threading::thread(level_loader(lvl));
	}
}

level* load_level(const std::string& lvl)
{
	std::cerr << "START LOAD LEVEL\n";
	level_map::iterator itor;
	{
		threading::lock lck(levels_loading_mutex());
		itor = levels_loading.find(lvl);
		if(itor == levels_loading.end()) {
			level* res = new level(lvl);
			res->finish_loading();
			fprintf(stderr, "LOADED LEVEL: %p\n", res);
			return res;
		}
	}

	itor->second.first->join();
	delete itor->second.first;
	level* res = itor->second.second;
	if(res == NULL) {
		res = new level(lvl);
	}
	res->finish_loading();
	levels_loading.erase(itor);
	std::cerr << "FINISH LOAD LEVEL\n";
	return res;
}

namespace {
bool hidden_file(const std::string& filename) {
	return !filename.empty() && filename[0] == '.';
}
}

std::vector<std::string> get_known_levels()
{
	std::vector<std::string> files;
	sys::get_files_in_dir(preferences::level_path(), &files);
	files.erase(std::remove_if(files.begin(), files.end(), hidden_file), files.end());

	foreach(const std::string& pkg, package::all_packages()) {
		std::vector<std::string> v = package::package_levels(pkg);
		files.insert(files.end(), v.begin(), v.end());
	}

	std::sort(files.begin(), files.end());
	
	return files;
}
