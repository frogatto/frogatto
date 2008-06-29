#ifndef LOAD_LEVEL_HPP_INCLUDED
#define LOAD_LEVEL_HPP_INCLUDED

#include <string>

class level;

struct load_level_manager {
	load_level_manager();
	~load_level_manager();
};

void preload_level(const std::string& lvl);
level* load_level(const std::string& lvl);

#endif
