#ifndef MODULE_HPP_INCLUDED
#define MODULE_HPP_INCLUDED

#include <boost/scoped_ptr.hpp>

#include "filesystem.hpp"
#include "formula_callable.hpp"
#include "variant.hpp"

#include <map>
#include <string>
#include <vector>

namespace module {

struct modules {
	std::string name_;
	std::string pretty_name_;
	std::string abbreviation_;
	std::string base_path_;
	std::vector<int> version_;
	std::vector<std::string> included_modules_;
};

typedef std::map<std::string, std::string> module_file_map;
typedef std::pair<std::string, std::string> module_file_pair;
typedef std::map<std::string, std::string>::const_iterator module_file_map_iterator;

const std::string get_module_name();
const std::string get_module_pretty_name();
std::string get_module_version();
std::string map_file(const std::string& fname);
void get_unique_filenames_under_dir(const std::string& dir,
                                    std::map<std::string, std::string>* file_map);

void get_files_in_dir(const std::string& dir,
                      std::vector<std::string>* files,
                      std::vector<std::string>* dirs=NULL,
                      sys::FILE_NAME_MODE mode=sys::FILE_NAME_ONLY);

std::string get_id(const std::string& id);
std::string get_module_id(const std::string& id);
std::string make_module_id(const std::string& name);
std::map<std::string, std::string>::const_iterator find(const std::map<std::string, std::string>& filemap, const std::string& name);
const std::string& get_module_path(const std::string& abbrev="");
std::vector<variant> get_all();
variant get(const std::string& name);
void load(const std::string& name, bool initial=true);
void reload(const std::string& name);
void get_module_list(std::vector<std::string>& dirs);
void load_module_from_file(const std::string& modname, modules* mod_);
void write_file(const std::string& mod_path, const std::string& data);

variant build_package(const std::string& id);

bool uninstall_downloaded_module(const std::string& id);

class client : public game_logic::formula_callable
{
public:
	client();
	client(const std::string& host, const std::string& port);
	void install_module(const std::string& module_name);
	void get_status();
	bool process();
	variant get_value(const std::string& key) const;
private:
	enum OPERATION_TYPE { OPERATION_NONE, OPERATION_INSTALL, OPERATION_GET_STATUS };
	OPERATION_TYPE operation_;
	std::string module_id_;
	boost::scoped_ptr<class http_client> client_;

	std::map<std::string, variant> data_;
	variant module_info_;

	void on_response(std::string response);
	void on_error(std::string response);
	void on_progress(int sent, int total, bool uploaded);
};

}

#endif
