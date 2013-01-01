#ifndef LOAD_LEVEL_HPP_INCLUDED
#define LOAD_LEVEL_HPP_INCLUDED

#include <string>
#include <vector>

#include "variant.hpp"

class level;

struct load_level_manager {
	load_level_manager();
	~load_level_manager();
};

namespace loadlevel {
void load_level_paths();
void reload_level_paths();
const std::string& get_level_path(const std::string& name);
}

void clear_level_wml();
void preload_level_wml(const std::string& lvl);
variant load_level_wml(const std::string& lvl);
variant load_level_wml_nowait(const std::string& lvl);

void preload_level(const std::string& lvl);
level* load_level(const std::string& lvl);

std::vector<std::string> get_known_levels();

#endif
