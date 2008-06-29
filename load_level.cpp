#include <assert.h>

#include "level.hpp"
#include "load_level.hpp"

#include <boost/thread.hpp>

namespace {
typedef std::map<std::string, std::pair<boost::thread*, level*> > level_map;
level_map levels_loading;
boost::mutex levels_loading_mutex;

class level_loader {
	std::string lvl_;
public:
	level_loader(const std::string& lvl) : lvl_(lvl)
	{}
	void operator()() {
		level* lvl = new level(lvl_);
		boost::mutex::scoped_lock lck(levels_loading_mutex);
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
	boost::mutex::scoped_lock lck(levels_loading_mutex);
	if(levels_loading.count(lvl) == 0) {
		levels_loading[lvl].first = new boost::thread(level_loader(lvl));
	}
}

level* load_level(const std::string& lvl)
{
	level_map::iterator itor;
	{
		boost::mutex::scoped_lock lck(levels_loading_mutex);
		itor = levels_loading.find(lvl);
		if(itor == levels_loading.end()) {
			return new level(lvl);
		}
	}

	itor->second.first->join();
	delete itor->second.first;
	level* res = itor->second.second;
	levels_loading.erase(itor);
	return res;
}
