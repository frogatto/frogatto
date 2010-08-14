#include <algorithm>
#include <iostream>

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "package.hpp"
#include "preferences.hpp"
#include "string_utils.hpp"

namespace package {

namespace {
std::string packages_dir() {
	return std::string(preferences::user_data_path()) + "/packages";
}
}

std::vector<std::string> all_packages()
{
	std::vector<std::string> files, dirs;
	sys::get_files_in_dir(packages_dir(), &files, &dirs);
	std::cerr << "PACKAGES: " << dirs.size() << "\n";
	return dirs;
}

void create_package(const std::string& name)
{
	sys::get_dir(packages_dir());
	sys::get_dir(packages_dir() + "/" + name);
	sys::get_dir(packages_dir() + "/" + name + "/level");
	sys::get_dir(packages_dir() + "/" + name + "/objects");
}

namespace {
bool hidden_file(const std::string& filename) {
	return !filename.empty() && filename[0] == '.';
}
}

std::vector<std::string> package_levels(const std::string& name)
{
	std::vector<std::string> files;
	sys::get_files_in_dir(packages_dir() + "/" + name + "/level", &files);
	files.erase(std::remove_if(files.begin(), files.end(), hidden_file), files.end());

	std::cerr << "LOAD PACKAGE: " << name << "\n";
	foreach(std::string& f, files) {
		f = name + "/" + f;
	std::cerr << "PACKAGE: " << name << " -> " << f << "\n";
	}
	

	return files;
}

std::string get_level_filename(const std::string& level_id)
{
	std::vector<std::string> v = util::split(level_id, '/');
	if(v.size() == 1) {
		return (preferences::load_compiled() ? "data/compiled/level/" : preferences::level_path()) + "/" + level_id;
	}

	ASSERT_LOG(v.size() == 2, "Illegal level ID: " << level_id);
	return packages_dir() + "/" + v.front() + "/level/" + v.back();
}

}
