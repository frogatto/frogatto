#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <deque>
#include <iostream>

#if !defined(_WINDOWS)
#include <sys/time.h>
#endif

#include "asserts.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "json_parser.hpp"
#include "module_web_server.hpp"
#include "string_utils.hpp"
#include "utils.hpp"
#include "unit_test.hpp"
#include "variant.hpp"

using boost::asio::ip::tcp;

module_web_server::module_web_server(const std::string& data_path, boost::asio::io_service& io_service, int port)
	: http::web_server(io_service, port), timer_(io_service), 
	nheartbeat_(0), data_path_(data_path)
{
	if(data_path_.empty() || data_path_[data_path_.size()-1] != '/') {
		data_path_ += "/";
	}

	if(sys::file_exists(data_file_path())) {
		data_ = json::parse_from_file(data_file_path());
	} else {
		std::map<variant, variant> m;
		data_ = variant(&m);
	}

	heartbeat();
}

void module_web_server::heartbeat()
{
	timer_.expires_from_now(boost::posix_time::seconds(1));
	timer_.async_wait(boost::bind(&module_web_server::heartbeat, this));
}

void module_web_server::handle_post(socket_ptr socket, variant doc, const http::environment& env)
{
	std::map<variant,variant> response;
	try {
		const std::string msg_type = doc["type"].as_string();
		if(msg_type == "upload_module") {
			variant module_node = doc["module"];
			const std::string module_id = module_node["id"].as_string();
			ASSERT_LOG(std::count_if(module_id.begin(), module_id.end(), isalnum) + std::count(module_id.begin(), module_id.end(), '_') == module_id.size(), "ILLEGAL MODULE ID");

			std::vector<variant> prev_versions;

			variant current_data = data_[variant(module_id)];
			if(current_data.is_null() == false) {
				const variant new_version = module_node[variant("version")];
				const variant old_version = current_data[variant("version")];
				ASSERT_LOG(new_version > old_version, "VERSION " << new_version.write_json() << " IS NOT NEWER THAN EXISTING VERSION " << old_version.write_json());
				prev_versions = current_data[variant("previous_versions")].as_list();
				current_data.remove_attr_mutation(variant("previous_versions"));
				prev_versions.push_back(current_data);
			}

			const std::string module_path = data_path_ + module_id + ".cfg";
			const std::string module_path_tmp = module_path + ".tmp";
			const std::string contents = module_node.write_json();
			
			sys::write_file(module_path_tmp, contents);
			const int rename_result = rename(module_path_tmp.c_str(), module_path.c_str());
			ASSERT_LOG(rename_result == 0, "FAILED TO RENAME FILE: " << errno);

			response[variant("status")] = variant("ok");

			{
				std::map<variant, variant> summary;
				summary[variant("previous_versions")] = variant(&prev_versions);
				summary[variant("version")] = module_node[variant("version")];
				summary[variant("name")] = module_node[variant("name")];
				summary[variant("description")] = module_node[variant("description")];
				data_.add_attr_mutation(variant(module_id), variant(&summary));
				write_data();
			}

		} else {
			ASSERT_LOG(false, "Unknown message type");
		}
	} catch(validation_failure_exception& e) {
		response[variant("status")] = variant("error");
		response[variant("message")] = variant(e.msg);
	}

	send_msg(socket, "text/json", variant(&response).write_json(), "");
}

void module_web_server::handle_get(socket_ptr socket, const std::string& url, const std::map<std::string, std::string>& args)
{
	std::map<variant,variant> response;
	try {
		std::cerr << "URL: (" << url << ")\n";
		response[variant("status")] = variant("error");
		if(url == "/download_module" && args.count("module_id")) {
			const std::string module_id = args.find("module_id")->second;
			const std::string module_path = data_path_ + module_id + ".cfg";
			if(sys::file_exists(module_path)) {
				std::string response = "{\nstatus: \"ok\",\nmodule: ";
				{
					const std::string contents = sys::read_file(module_path);
					response += contents;
				}

				response += "\n}";
				send_msg(socket, "text/json", response, "");
				return;

			} else {
				response[variant("message")] = variant("No such module");
			}
		} else if(url == "/get_summary") {
			response[variant("status")] = variant("ok");
			response[variant("summary")] = data_;
		} else {
			response[variant("message")] = variant("Unknown path");
		}
	} catch(validation_failure_exception& e) {
		response[variant("status")] = variant("error");
		response[variant("message")] = variant(e.msg);
	}

	send_msg(socket, "text/json", variant(&response).write_json(), "");
}

std::string module_web_server::data_file_path() const
{
	return data_path_ + "/module-data.json";
}

void module_web_server::write_data()
{
	const std::string tmp_path = data_file_path() + ".tmp";
	sys::write_file(tmp_path, data_.write_json());
	const int rename_result = rename(tmp_path.c_str(), data_file_path().c_str());
		ASSERT_LOG(rename_result == 0, "FAILED TO RENAME FILE: " << errno);
}

COMMAND_LINE_UTILITY(module_server)
{
	std::string path = ".";
	int port = 23456;

	std::deque<std::string> arguments(args.begin(), args.end());
	while(!arguments.empty()) {
		const std::string arg = arguments.front();
		arguments.pop_front();
		if(arg == "--path") {
			ASSERT_LOG(arguments.empty() == false, "NEED ARGUMENT AFTER " << arg);
			path = arguments.front();
			arguments.pop_front();
		} else if(arg == "-p" || arg == "--port") {
			ASSERT_LOG(arguments.empty() == false, "NEED ARGUMENT AFTER " << arg);
			port = atoi(arguments.front().c_str());
			arguments.pop_front();
		} else {
			ASSERT_LOG(false, "UNRECOGNIZED ARGUMENT: " << arg);
		}
	}

	const assert_recover_scope recovery;
	boost::asio::io_service io_service;
	module_web_server server(path, io_service, port);
	io_service.run();
}
