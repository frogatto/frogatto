#ifndef PACKAGE_HPP_INCLUDED
#define PACKAGE_HPP_INCLUDED

#include <string>
#include <vector>

namespace package {

std::vector<std::string> all_packages();
void create_package(const std::string& name);
std::vector<std::string> package_levels(const std::string& name);

std::string get_level_filename(const std::string& level_id);
}

#endif
