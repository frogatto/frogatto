#include <deque>

#include <boost/bind.hpp>

#include "asserts.hpp"
#include "base64.hpp"
#include "compress.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "http_client.hpp"
#include "json_parser.hpp"
#include "md5.hpp"
#include "module.hpp"
#include "preferences.hpp"
#include "unit_test.hpp"
#include "uri.hpp"

namespace module {

namespace {

// This will disappear when frogatto is moved to it's on module, then it becomes "core", "core", "core".
module::modules core = {"core", "core", "core", ""};

std::vector<module::modules>& loaded_paths() {
	static std::vector<module::modules> result(1, core);
	return result;
}
}

const std::string get_module_name(){
	return loaded_paths().empty() ? "frogatto" : loaded_paths()[0].name_;
}

const std::string get_module_pretty_name() {
	return loaded_paths().empty() ? "Frogatto" :  loaded_paths()[0].pretty_name_;
}

std::string get_module_version() {
	if(!loaded_paths().empty()) {
		const std::vector<int>& v = loaded_paths()[0].version_;
		if(v.empty()) {
			return "";
		}

		std::ostringstream s;
		s << v[0];
		for(int n = 1; n < v.size(); ++n) {
			s << "." << v[n];
		}

		return s.str();
	} else {
		return "";
	}
}

std::string map_file(const std::string& fname)
{
	foreach(const modules& p, loaded_paths()) {
		std::string path = sys::find_file(p.base_path_ + fname);
		if(sys::file_exists(path)) {
			return path;
		}
	}
	return fname;
}

std::map<std::string, std::string>::const_iterator find(const std::map<std::string, std::string>& filemap, const std::string& name) {
	foreach(const modules& p, loaded_paths()) {
		std::map<std::string, std::string>::const_iterator itor = filemap.find(p.abbreviation_ + ":" + name);
		if(itor != filemap.end()) {
			return itor;
		}
		itor = filemap.find(name);
		if(itor != filemap.end()) {
			return itor;
		}
	}
	return filemap.end();
}

void get_unique_filenames_under_dir(const std::string& dir,
                                    std::map<std::string, std::string>* file_map)
{
	foreach(const modules& p, loaded_paths()) {
		const std::string path = p.base_path_ + dir;
		sys::get_unique_filenames_under_dir(path, file_map, p.abbreviation_ + ":");
	}
}

void get_files_in_dir(const std::string& dir,
                      std::vector<std::string>* files,
                      std::vector<std::string>* dirs,
                      sys::FILE_NAME_MODE mode)
{
	foreach(const modules& p, loaded_paths()) {
		const std::string path = p.base_path_ + dir;
		sys::get_files_in_dir(path, files, dirs, mode);
	}
}

std::string get_id(const std::string& id) {
	size_t cpos = id.find(':');
	if(cpos != std::string::npos) {
		return id.substr(cpos+1);
	}
	return id;
}

std::string get_module_id(const std::string& id) {
	size_t cpos = id.find(':');
	if(cpos != std::string::npos) {
		return id.substr(0, cpos);
	}
	return std::string();
}

std::string make_module_id(const std::string& name) {
	// convert string with path to module:filename syntax
	// e.g. vgi:wip/test1x.cfg -> vgi:test1x.cfg; test1.cfg -> vgi:test1.cfg 
	// (assuming vgi is default module loaded).
	std::string conv_name;
	std::string nn = name;
	size_t cpos = name.find(':');
	std::string modname = loaded_paths().front().abbreviation_;
	if(cpos != std::string::npos) {
		modname = name.substr(0, cpos);
		nn = name.substr(cpos+1);
	}
	int spos = nn.rfind('/');
	if(spos == std::string::npos) {
		spos = nn.rfind('\\');
	}
	if(spos != std::string::npos) {
		conv_name = modname + ":" + nn.substr(spos+1);
	} else {
		conv_name = modname + ":" + nn;
	}
	return conv_name;
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

variant get(const std::string& mod_file_name)
{
	std::string name(mod_file_name);
	if(name.size() > 4 && name.substr(name.size()-4) == ".cfg") {
		name = name.substr(0, name.size()-4);
	}
	std::string fname = "modules/" + name + "/module.cfg";
	if(sys::file_exists(fname)) {
		variant v = json::parse_from_file(fname);
		v.add_attr(variant("id"), variant(fname));
		return v;
	} else {
		return variant();
	}
}

const std::string& get_module_path(const std::string& abbrev) {
	if(abbrev == "") {
		// No abbreviation returns path of first loaded module.
		return loaded_paths().front().base_path_;
	}
	foreach(const modules& m, loaded_paths()) {
		if(m.abbreviation_ == abbrev || m.name_ == abbrev) {
			return m.base_path_;
		}
	}
	// If not found we return the path of the default module.
	// XXX may change this behaviour, depending on how it's seen in practice.
	return loaded_paths().front().base_path_;
}

const std::string make_base_module_path(const std::string& name) {
	return "modules/" + name + "/";
}

void load(const std::string& mod_file_name, bool initial)
{
	std::string name(mod_file_name);
	if(name.size() > 4 && name.substr(name.size()-4) == ".cfg") {
		name = name.substr(0, name.size()-4);
	}
	std::string pretty_name = name;
	std::string abbrev = name;
	std::string fname = make_base_module_path(name) + "module.cfg";
	variant v = json::parse_from_file(fname);

	if(v.is_map()) {
		ASSERT_LOG(v["min_engine_version"].is_null() == false, "A min_engine_version field in the module.cfg file must be specified.");
		ASSERT_LOG(v["min_engine_version"] <= preferences::version_decimal(), "The engine version being used (" << preferences::version_decimal()
			<< ") to run the module is older than required by the module (" << v["min_engine_version"] << ").");

		if(v["name"].is_null() == false) {
			pretty_name = v["name"].as_string();
		} else if(v["id"].is_null() == false) {
			pretty_name = v["id"].as_string();
		}
		if(v["abbreviation"].is_null() == false) {
			abbrev = v["abbreviation"].as_string();
		}
		if(v["dependencies"].is_null() == false) {
			if(v["dependencies"].is_string()) {
				load(v["dependencies"].as_string(), false);
			} else if( v["dependencies"].is_list()) {
				foreach(const std::string& modname, v["dependencies"].as_list_string()) {
					load(modname, false);
				}
			}
		}
	}
	modules m = {name, pretty_name, abbrev, make_base_module_path(name)};
	loaded_paths().insert(loaded_paths().begin(), m);
}

void reload(const std::string& name) {
	loaded_paths().clear();
	loaded_paths().push_back(core);
	load(name, true);
}

void get_module_list(std::vector<std::string>& dirs) {
	// Grab the files/directories under ./module/ for later use.
	std::vector<std::string> files;
	sys::get_files_in_dir("modules/", &files, &dirs);
}

void load_module_from_file(const std::string& modname, modules* mod_) {
	variant v = json::parse_from_file("./modules/" + modname + "/module.cfg");
	ASSERT_LOG(mod_ != NULL, "Invalid module pointer passed.");
	if(v.is_map()) {
		ASSERT_LOG(v["min_engine_version"].is_null() == false, "A min_engine_version field in the module.cfg file must be specified.");
		ASSERT_LOG(v["min_engine_version"] <= preferences::version_decimal(), "The engine version being used (" << preferences::version_decimal()
			<< ") to run the module is older than required by the module (" << v["min_engine_version"] << ").");

		if(v["id"].is_null() == false) {
			mod_->name_= v["id"].as_string();
		}
		if(v["name"].is_null() == false) {
			mod_->pretty_name_= v["name"].as_string();
		}
		if(v["abbreviation"].is_null() == false) {
			mod_->abbreviation_= v["abbreviation"].as_string();
		}
		if(v["dependencies"].is_string()) {
			mod_->included_modules_.push_back(v["dependencies"].as_string());
		} else if(v["dependencies"].is_list()) {
			foreach(const std::string& s, v["dependencies"].as_list_string()) {
				mod_->included_modules_.push_back(s);
			}
		}

		if(v["version"].is_list()) {
			mod_->version_ = v["version"].as_list_int();
		}
	}
}

void write_file(const std::string& mod_path, const std::string& data)
{
	std::string path;
	std::string abbrev = get_module_id(mod_path);
	std::string rel_path = get_id(mod_path);
	// Write a file to a relative path inside a module. rel_path includes the file name.
	// e.g. module::write_file("", "data/object/experimental/bat.cfg", data);
	// If the current module was xxx, then the file would get written to the path
	// ./modules/xxx/data/object/experimental/bat.cfg	
	if(loaded_paths().empty()) {
		path = rel_path;
	} else {
		path = get_module_path(abbrev) + rel_path;
	}
	sys::write_file(path, data);
}

namespace {
void get_files_in_module(const std::string& dir, std::vector<std::string>& res)
{
	std::vector<std::string> files, dirs;
	sys::get_files_in_dir(dir, &files, &dirs);
	foreach(const std::string& d, dirs) {
		get_files_in_module(dir + "/" + d, res);
	}

	foreach(const std::string& fname, files) {
		if(fname.empty() == false && fname[0] == '.') {
			continue;
		}

		res.push_back(dir + "/" + fname);
	}
}
}

variant build_package(const std::string& id)
{
	std::vector<char> data;
	std::vector<std::string> files;
	const std::string path = "modules/" + id;
	ASSERT_LOG(sys::file_exists(path), "COULD NOT FIND PATH: " << path);
	get_files_in_module(path, files);
	std::map<variant, variant> file_attr;
	foreach(const std::string& file, files) {
		std::cerr << "processing " << file << "...\n";
		std::string fname(file.begin() + path.size() + 1, file.end());
		std::map<variant, variant> attr;

		const std::string contents = sys::read_file(file);

		attr[variant("begin")] = variant(data.size());
		attr[variant("size")] = variant(contents.size());
		attr[variant("md5")] = variant(md5::sum(contents));

		data.insert(data.end(), contents.begin(), contents.end());

		file_attr[variant(fname)] = variant(&attr);
	}

	const std::string module_cfg_file = path + "/module.cfg";
	variant module_cfg = json::parse(sys::read_file(module_cfg_file));
	ASSERT_LOG(module_cfg["version"].is_list(), "IN " << module_cfg_file << " THERE MUST BE A VERSION NUMBER GIVEN AS A LIST OF INTEGERS");

	//this verifies that compression/decompression works but is slow.
	//ASSERT_LOG(zip::decompress_known_size(base64::b64decode(base64::b64encode(zip::compress(data))), data.size()) == data, "COMPRESS/DECOMPRESS BROKEN");

	std::cerr << "compressing data: " << data.size() << "...\n";
	data = base64::b64encode(zip::compress(data));

	const std::string data_str(data.begin(), data.end());

	std::map<variant, variant> data_attr;
	data_attr[variant("id")] = variant(id);
	data_attr[variant("version")] = module_cfg["version"];
	data_attr[variant("manifest")] = variant(&file_attr);
	data_attr[variant("data")] = variant(data_str);
	data_attr[variant("data_size")] = variant(data_str.size());
	return variant(&data_attr);
}

namespace {
void finish_upload(std::string response, bool* flag)
{
	std::cerr << "UPLOAD COMPLETE: " << response << "\n";
	*flag = true;
}

void error_upload(std::string response, bool* flag)
{
	std::cerr << "UPLOAD ERROR: " << response << "\n";
	*flag = true;
}

void upload_progress(int sent, int total, bool uploaded)
{
	std::cerr << "SENT " << sent << "/" << total << "\n";
}

}

COMMAND_LINE_UTILITY(publish_module)
{
	std::string module_id;
	std::string server = "localhost";
	std::string port = "23456";

	std::deque<std::string> arguments(args.begin(), args.end());
	while(!arguments.empty()) {
		const std::string arg = arguments.front();
		arguments.pop_front();
		if(arg == "--server") {
			ASSERT_LOG(arguments.empty() == false, "NEED ARGUMENT AFTER " << arg);
			server = arguments.front();
			arguments.pop_front();
		} else if(arg == "-p" || arg == "--port") {
			ASSERT_LOG(arguments.empty() == false, "NEED ARGUMENT AFTER " << arg);
			port = arguments.front();
			arguments.pop_front();
		} else {
			ASSERT_LOG(module_id.empty(), "UNRECOGNIZED ARGUMENT: " << module_id);
			module_id = arg;
			ASSERT_LOG(std::count_if(module_id.begin(), module_id.end(), isalnum) + std::count(module_id.begin(), module_id.end(), '_') == module_id.size(), "ILLEGAL ARGUMENT: " << module_id);
		}
	}

	ASSERT_LOG(module_id.empty() == false, "MUST SPECIFY MODULE ID");

	const variant package = build_package(module_id);
	std::map<variant,variant> attr;
	attr[variant("type")] = variant("upload_module");
	attr[variant("module")] = package;

	const std::string msg = variant(&attr).write_json();

	bool done = false;

	http_client client(server, port);
	client.send_request("POST /module_upload", msg, 
	                    boost::bind(finish_upload, _1, &done),
	                    boost::bind(error_upload, _1, &done),
	                    boost::bind(upload_progress, _1, _2, _3));

	while(!done) {
		client.process();
	}
}

COMMAND_LINE_UTILITY(publish_module_stats)
{
	std::string module_id;
	std::string server = "localhost";
	std::string port = "23456";

	std::deque<std::string> arguments(args.begin(), args.end());
	while(!arguments.empty()) {
		const std::string arg = arguments.front();
		arguments.pop_front();
		if(arg == "--server") {
			ASSERT_LOG(arguments.empty() == false, "NEED ARGUMENT AFTER " << arg);
			uri::uri url = uri::uri::parse(arguments.front());
			arguments.pop_front();
			server = url.host();
			port = url.port();
		} else {
			ASSERT_LOG(module_id.empty(), "UNRECOGNIZED ARGUMENT: " << module_id);
			module_id = arg;
			ASSERT_LOG(std::count_if(module_id.begin(), module_id.end(), isalnum) + std::count(module_id.begin(), module_id.end(), '_') == module_id.size(), "ILLEGAL ARGUMENT: " << module_id);
		}
	}

	ASSERT_LOG(module_id.empty() == false, "MUST SPECIFY MODULE ID");

	const std::string path = "modules/" + module_id + "/stats-server.json";
	ASSERT_LOG(sys::file_exists(path), "DID NOT FIND STATS FILE DEFINITION AT " << path);

	const variant doc = json::parse(sys::read_file(path));

	std::map<variant,variant> attr;
	attr[variant("type")] = variant("upload_table_definitions");
	attr[variant("module")] = variant(module_id);
	attr[variant("definition")] = doc;

	const std::string msg = variant(&attr).write_json();

	bool done = false;

	http_client client(server, port);
	client.send_request("POST /stats", msg, 
	                    boost::bind(finish_upload, _1, &done),
	                    boost::bind(error_upload, _1, &done),
	                    boost::bind(upload_progress, _1, _2, _3));

	while(!done) {
		client.process();
	}
}

}
