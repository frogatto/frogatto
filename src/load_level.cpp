#include <assert.h>

#include "filesystem.hpp"
#include "level.hpp"
#include "load_level.hpp"
#include "preprocessor.hpp"
#include "thread.hpp"

namespace {
typedef std::map<std::string, std::pair<threading::thread*, level*> > level_map;
level_map levels_loading;
threading::mutex levels_loading_mutex;

class level_loader {
	std::string lvl_;
public:
	level_loader(const std::string& lvl) : lvl_(lvl)
	{}
	void operator()() {
		level* lvl = new level(lvl_);
		threading::lock lck(levels_loading_mutex);
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
		/*
	assert(!lvl.empty());
	threading::lock lck(levels_loading_mutex);
	if(levels_loading.count(lvl) == 0) {
		levels_loading[lvl].first = new threading::thread(level_loader(lvl));
	}*/
}

level* load_level(const std::string& lvl)
{
	std::cerr << "START LOAD LEVEL\n";
	level_map::iterator itor;
	{
		threading::lock lck(levels_loading_mutex);
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
	sys::get_files_in_dir("data/level/", &files);
	files.erase(std::remove_if(files.begin(), files.end(), hidden_file), files.end());
	return files;
}
