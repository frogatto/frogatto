/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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

enum BASE_PATH_TYPE { BASE_PATH_GAME, BASE_PATH_USER, NUM_PATH_TYPES };

struct modules {
	std::string name_;
	std::string pretty_name_;
	std::string abbreviation_;

	//base_path_ is in the game data directory, user_path_ is in the user's
	//preferences area and is mutable.
	std::string base_path_[NUM_PATH_TYPES];

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
const std::string& get_module_path(const std::string& abbrev="", BASE_PATH_TYPE type=BASE_PATH_GAME);
std::vector<variant> get_all();
variant get(const std::string& name);
void load(const std::string& name, bool initial=true);
void reload(const std::string& name);
void get_module_list(std::vector<std::string>& dirs);
void load_module_from_file(const std::string& modname, modules* mod_);
void write_file(const std::string& mod_path, const std::string& data);

variant build_package(const std::string& id);

bool uninstall_downloaded_module(const std::string& id);

void set_module_args(game_logic::const_formula_callable_ptr callable);
game_logic::const_formula_callable_ptr get_module_args();

class client : public game_logic::formula_callable
{
public:
	client();
	client(const std::string& host, const std::string& port);
	void install_module(const std::string& module_name);
	void rate_module(const std::string& module_id, int rating, const std::string& review);
	void get_status();
	bool process();
	variant get_value(const std::string& key) const;
private:
	enum OPERATION_TYPE { OPERATION_NONE, OPERATION_INSTALL, OPERATION_GET_STATUS, OPERATION_GET_ICONS, OPERATION_RATE };
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
