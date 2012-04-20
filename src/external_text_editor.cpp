#ifndef NO_EDITOR
#include "custom_object_type.hpp"
#include "external_text_editor.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "json_parser.hpp"
#include "thread.hpp"

#include "SDL.h"

#include <stdlib.h>

#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

namespace {
std::string normalize_fname(std::string fname)
{
	while(strstr(fname.c_str(), "//")) {
		fname.erase(fname.begin() + (strstr(fname.c_str(), "//") - fname.c_str()));
	}

	return fname;
}

class vi_editor : public external_text_editor
{
	std::string cmd_;
	int counter_;

	std::map<std::string, std::string> files_;
	std::map<std::string, std::string> file_contents_;
	std::string active_file_;

	//vim servers that we've already inspected in the past, and don't
	//need to do so again.
	std::set<std::string> known_servers_;

	threading::mutex mutex_;
	boost::scoped_ptr<threading::thread> thread_;

	bool shutdown_;

	void refresh_editor_list()
	{
		const int begin = SDL_GetTicks();
		const std::string cmd = "gvim --serverlist";
		FILE* p = popen(cmd.c_str(), "r");
		if(p) {
			std::vector<std::string> servers;

			char buf[1024];
			while(fgets(buf, sizeof(buf), p)) {
				std::string s(buf);
				if(s[s.size()-1] == '\n') {
					s.resize(s.size()-1);
				}

				servers.push_back(s);
			}

			fclose(p);

			{
				threading::lock l(mutex_);
				for(std::map<std::string, std::string>::iterator i = files_.begin(); i != files_.end(); ) {
					if(!std::count(servers.begin(), servers.end(), i->second)) {
						files_.erase(i++);
					} else {
						++i;
					}
				}
			}

			foreach(const std::string& server, servers) {
				{
					threading::lock l(mutex_);
					if(known_servers_.count(server)) {
						continue;
					}
	
					known_servers_.insert(server);
				}

				const std::string cmd = "gvim --servername " + server + " --remote-expr 'simplify(bufname(1))'";
				FILE* p = popen(cmd.c_str(), "r");
				if(p) {
					char buf[1024];
					if(fgets(buf, sizeof(buf), p)) {
						std::string s(buf);
						if(s[s.size()-1] == '\n') {
							s.resize(s.size()-1);
						}

						if(s.size() > 4 && std::string(s.end()-4,s.end()) == ".cfg") {
							threading::lock l(mutex_);
							files_[s] = server;
							std::cerr << "VIM LOADED FILE: " << s << " -> " << server << "\n";
						}
					}

					fclose(p);
				}
			}
		}
	}

	bool get_file_contents(const std::string& server, std::string* data)
	{
		const std::string cmd = "gvim --servername " + server + " --remote-expr 'getbufline(1, 1, 1000000)'";

		FILE* p = popen(cmd.c_str(), "r");
		if(p) {
			std::vector<char> buf;
			buf.resize(10000000);
			size_t nbytes = fread(&buf[0], 1, buf.size(), p);
			fclose(p);
			if(nbytes > 0 && nbytes <= buf.size()) {
				buf.resize(nbytes);
				buf.push_back(0);
				*data = buf.data();
				return true;
			}
		}

		return false;
	}

	void run_thread()
	{
		for(int tick = 0; !shutdown_; ++tick) {
			SDL_Delay(60);

			std::map<std::string, std::string> files;
			{
				threading::lock l(mutex_);
				if(tick%10 == 0) {
					refresh_editor_list();
					files = files_;
				} else if(files_.count(active_file_)) {
					files[active_file_] = files_[active_file_];
				}
			}

			std::map<std::string, std::string> results;
			std::set<std::string> remove_files;

			typedef std::pair<std::string,std::string> str_pair;
			foreach(const str_pair& item, files) {
				std::string contents;
				if(get_file_contents(item.second, &contents)) {
					results[item.first] = contents;
				} else {
					remove_files.insert(item.first);
				}
			}

			{
				threading::lock l(mutex_);
				foreach(const std::string& fname, remove_files) {
					files_.erase(fname);
				}

				foreach(const str_pair& item, results) {
					if(file_contents_[item.first] != item.second) {
						std::cerr << "CONTENTS OF " << item.first << " UPDATED..\n";
						file_contents_[item.first] = item.second;
						active_file_ = item.first;
					}
				}
			}
		}
	}

public:
	explicit vi_editor(variant obj) : cmd_(obj["command"].as_string_default("gvim")), counter_(0), shutdown_(false)
	{
		thread_.reset(new threading::thread(boost::bind(&vi_editor::run_thread, this)));
	}

	void shutdown()
	{
		{
			threading::lock l(mutex_);
			shutdown_ = true;
		}

		thread_.reset();
	}

	void load_file(const std::string& fname_input)
	{
		const std::string fname = normalize_fname(fname_input);

		std::string existing_instance;
		{
			threading::lock l(mutex_);

			if(files_.count(fname)) {
				existing_instance = files_[fname];
			}
		}

		if(existing_instance.empty() == false) {
			const std::string command = "gvim --servername " + existing_instance + " --remote-expr 'foreground()'";
			const int result = system(command.c_str());
			return;
		}

		std::string server_name = formatter() << "S" << counter_;
		{
			threading::lock l(mutex_);
			while(known_servers_.count(server_name)) {
				++counter_;
				server_name = formatter() << "S" << counter_;
			}
		}

		const std::string command = cmd_ + " --servername " + server_name + " " + fname;
		const int result = system(command.c_str());

		threading::lock l(mutex_);
		files_[fname] = server_name;
		++counter_;
	}

	std::string get_file_contents(const std::string& fname_input)
	{
		const std::string fname = normalize_fname(fname_input);

		threading::lock l(mutex_);
		std::map<std::string, std::string>::const_iterator itor = file_contents_.find(fname);
		if(itor != file_contents_.end()) {
			return itor->second;
		} else {
			return "";
		}
	}

	int get_line(const std::string& fname_input) const
	{
		const std::string fname = normalize_fname(fname_input);
		return 0;
	}

	std::vector<std::string> loaded_files() const
	{
		threading::lock l(mutex_);
		std::vector<std::string> result;
		for(std::map<std::string,std::string>::const_iterator i = files_.begin(); i != files_.end(); ++i) {
			result.push_back(i->first);
		}
		return result;
	}
};

std::set<external_text_editor*> all_editors;

}

external_text_editor::manager::manager()
{
}

external_text_editor::manager::~manager()
{
	foreach(external_text_editor* e, all_editors) {
		e->shutdown();
	}
}


external_text_editor_ptr external_text_editor::create(variant key)
{
	const std::string type = key["type"].as_string();
	external_text_editor_ptr result;
	if(type == "vi") {
		static external_text_editor_ptr ptr(new vi_editor(key));
		result = ptr;
	}

	if(result) {
		result->replace_in_game_editor_ = key["replace_in_game_editor"].as_bool(true);
	}

	return result;
}

external_text_editor::external_text_editor()
  : replace_in_game_editor_(true)
{
	all_editors.insert(this);
}

external_text_editor::~external_text_editor()
{
	all_editors.erase(this);
}

void external_text_editor::process()
{
	std::vector<std::string> files = loaded_files();
	if(files.empty() == false) {
		const int begin = SDL_GetTicks();
		foreach(const std::string& fname, files) {
			const std::string contents = get_file_contents(fname);
			if(contents != json::get_file_contents(fname)) {
				custom_object_type::set_file_contents(fname, contents);
			}
		}
	}
}
#endif // NO_EDITOR
