#include "filesystem.hpp"
#include "foreach.hpp"
#include "json_parser.hpp"
#include "module.hpp"

namespace module {

namespace {

std::vector<std::string>& loaded_paths() {
	static std::vector<std::string> result(1, "");
	return result;
}
}

std::string module_loaded_;

const std::string get_module_name(){
	return module_loaded_.empty() ? "frogatto" : module_loaded_;
}

void set_module_name(const std::string& name) {
	module_loaded_ = name;
}

std::string map_file(const std::string& fname)
{
	foreach(const std::string& p, loaded_paths()) {
		std::string path = sys::find_file(p + fname);
		if(sys::file_exists(path)) {
			return path;
		}
	}

	return fname;
}

void get_unique_filenames_under_dir(const std::string& dir,
                                    std::map<std::string, std::string>* file_map)
{
	foreach(const std::string& p, loaded_paths()) {
		const std::string path = p + dir;
		sys::get_unique_filenames_under_dir(path, file_map);
	}
}

void get_files_in_dir(const std::string& dir,
                      std::vector<std::string>* files,
                      std::vector<std::string>* dirs,
                      sys::FILE_NAME_MODE mode)
{
	foreach(const std::string& p, loaded_paths()) {
		const std::string path = p + dir;
		sys::get_files_in_dir(path, files, dirs, mode);
	}
}



std::vector<variant> get_all()
{
	std::vector<variant> result;

	std::vector<std::string> files, dirs;
	sys::get_files_in_dir("modules/", &files, &dirs);
	foreach(const std::string& dir, dirs) {
		std::string fname = "modules/" + dir + "/module.cfg";
		if(sys::file_exists(fname)) {
			variant v = json::parse_from_file(fname);
			v.add_attr(variant("id"), variant(dir));
			result.push_back(v);
		}
	}

	return result;
}

variant get(const std::string& name)
{
	std::string fname = "modules/" + name + "/module.cfg";
	if(sys::file_exists(fname)) {
		variant v = json::parse_from_file(fname);
		v.add_attr(variant("id"), variant(fname));
		return v;
	} else {
		return variant();
	}
}

void load(const std::string& name)
{
	std::string modname = name;
	std::string fname = "modules/" + name + "/module.cfg";
	variant v = json::parse_from_file(fname);

	if(v.is_map()) {
		if(v["name"].is_null() == false) {
			modname = v["name"].as_string();
		} else if(v["id"].is_null() == false) {
			modname = v["id"].as_string();
		}
	}
	set_module_name(modname);
	loaded_paths().insert(loaded_paths().begin(), "./modules/" + name + "/");
}

}
