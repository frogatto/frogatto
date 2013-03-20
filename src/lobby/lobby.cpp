#ifdef _MSC_VER
#include "targetver.h"
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <json_spirit.h>

#include "asserts.hpp"
#include "server.hpp"
#include "shared_data.hpp"
#include "game_server_worker.hpp"
#include "sqlite_wrapper.hpp"

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace 
{
	const int64_t default_polling_interval = 5000;
	const std::string default_lobby_config_file = "lobby-config.cfg";
}

int main(int argc, char* argv[])
{
	std::vector<std::string> args;
	std::string lobby_config_file = default_lobby_config_file;

	// Push the command-line arguments into an array
	for(int i = 1; i < argc; ++i) {
		args.push_back(argv[i]);
	}

#ifdef BOOST_NO_CXX11_RANGE_BASED_FOR
	BOOST_FOREACH(auto a, args) {
#else
	for(auto a : args) {
#endif
		std::vector<std::string> seperated_args;
		boost::split(seperated_args, a, boost::lambda::_1 == '=');
		if(seperated_args[0] == "--config-file" || seperated_args[0] == "-n") {
			lobby_config_file = seperated_args[1];
		}
	}

	std::ifstream is(lobby_config_file);
	json_spirit::mValue value;
	json_spirit::read(is, value);
	ASSERT_LOG(value.type() == json_spirit::obj_type, "lobby-config.cfg should be an object.");
	auto cfg_obj = value.get_obj();
	if(cfg_obj.find("arguments") != cfg_obj.end()) {
#ifdef BOOST_NO_CXX11_RANGE_BASED_FOR
		BOOST_FOREACH(const auto&v, cfg_obj["arguments"].get_array()) {
#else
		for(const auto& v : cfg_obj["arguments"].get_array()) {
#endif
			args.push_back(v.get_str());
		}
	}

	sqlite::sqlite_wrapper_ptr db_ptr = sqlite::sqlite_wrapper_ptr(new sqlite::wrapper("lobby-data.db"));
	sqlite::bindings_type bindings;
	sqlite::rows_type result;
	bool res = db_ptr->exec("SELECT COUNT(*) FROM [sqlite_master] where tbl_name = 'users_table'", bindings, &result);
	if(res && result[0].get_int() == 0) {
		// No database found, let's make one.
		db_ptr->exec("CREATE TABLE users_table(username_clean varchar(255), username varchar(255), password varchar(255), user_avatar varchar(255), user_email varchar(255), user_data blob)", bindings, &result);
	}

	ASSERT_LOG(cfg_obj.find("database") != cfg_obj.end(), "No database record found in config file!");
	auto db_obj = cfg_obj["database"].get_obj();
	ASSERT_LOG(db_obj.find("address") != db_obj.end(), "No 'address' record found in database section");
	ASSERT_LOG(db_obj.find("port") != db_obj.end(), "No 'port' record found in database section");
	ASSERT_LOG(db_obj.find("name") != db_obj.end(), "No 'name' record found in database section");
	ASSERT_LOG(db_obj.find("username") != db_obj.end(), "No 'username' record found in database section");
	ASSERT_LOG(db_obj.find("password") != db_obj.end(), "No 'password' record found in database section");

	sql::Driver* driver;
	boost::shared_ptr<sql::Connection> conn;
	driver = get_driver_instance();
	ASSERT_LOG(driver != NULL, "Couldn't create database driver");
	std::stringstream str;
	str << "tcp://" << db_obj["address"].get_str() << ":" << db_obj["port"].get_int();
	conn = boost::shared_ptr<sql::Connection>(driver->connect(str.str(), db_obj["username"].get_str(), db_obj["password"].get_str()));
	ASSERT_LOG(conn != NULL, "Couldn't connect to the mysql database");
	conn->setSchema(db_obj["name"].get_str());
	
	int64_t polling_interval = default_polling_interval;
	auto it = cfg_obj.find("server_polling_interval");
	if(it != cfg_obj.end() && (it->second.type() == json_spirit::real_type 
		|| it->second.type() == json_spirit::int_type)) {
		polling_interval = int64_t(it->second.get_real() * 1000.0);
	}

	std::vector<std::pair<game_server::worker*, boost::shared_ptr<boost::thread> > > server_thread_list;

	try {
		auto listen_obj = cfg_obj["listen"].get_obj();
		std::size_t num_threads = cfg_obj["threads"].get_int();
		std::string file_path = cfg_obj["file_path"].get_str();

		game_server::shared_data shared_data(db_ptr, conn);

		auto gs_ary = cfg_obj["game_server"].get_array();
			// Create a tasks to poll the game servers
#ifdef BOOST_NO_CXX11_RANGE_BASED_FOR
		BOOST_FOREACH(auto game_servers, gs_ary) {
#else
		for(auto game_servers : gs_ary) {
#endif
			auto gs_obj = game_servers.get_obj();
			std::string gs_addr = gs_obj["address"].get_str();
			std::string gs_port = gs_obj["port"].get_str();

			// n.b. that boost thread takes it's callable by value, we use boost::ref to make it use a reference
			// to our created object. boost::thread manages the lifetime of the worker, hence it doesn't leak.
			game_server::worker* worker = new game_server::worker(polling_interval, shared_data, gs_addr, gs_port);
			boost::shared_ptr<boost::thread> game_server_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::ref(*worker)));
			server_thread_list.push_back(std::make_pair(worker, game_server_thread));
		}
	
		// Initialise the server.
		http::server::server s(listen_obj["address"].get_str(), 
			listen_obj["port"].get_str(), 
			file_path, 
			num_threads, 
			shared_data);

		// Run the server until stopped.
		s.run();

		// Abort thread and wait till it finishes
#ifdef BOOST_NO_CXX11_RANGE_BASED_FOR
		BOOST_FOREACH(auto p, server_thread_list) {
#else
		for(auto p : server_thread_list) {
#endif
			p.first->abort();
			p.second->join();
		}
	} catch(std::exception& e) {
		ASSERT_LOG(false, "exception: " << e.what());
	}

	return 0;
}
