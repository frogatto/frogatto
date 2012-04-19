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

class vi_editor : public external_text_editor
{
	std::string cmd_;
	int counter_;

	std::map<std::string, std::string> files_;
	std::map<std::string, std::string> file_contents_;
	std::string active_file_;

	threading::mutex mutex_;
	boost::scoped_ptr<threading::thread> thread_;

	bool shutdown_;

	bool get_file_contents(const std::string& server, std::string* data)
	{
		const std::string cmd = "gvim --servername " + server + " --remote-expr 'getbufline(1, 1, 1000000)'";

		FILE* p = popen(cmd.c_str(), "r");
		if(p) {
			std::vector<char> buf;
			buf.resize(10000000);
			size_t nbytes = fread(&buf[0], 1, buf.size(), p);
			fclose(p);
			std::cerr << "READ FILE: " << nbytes << "\n";
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
					file_contents_.erase(fname);
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
	{}

	void shutdown()
	{
		{
			threading::lock l(mutex_);
			shutdown_ = true;
		}

		thread_.reset();
	}

	void load_file(const std::string& fname)
	{
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

		const std::string server_name = formatter() << "S" << counter_;
		const std::string command = cmd_ + " --servername " + server_name + " " + fname;
		const int result = system(command.c_str());

		if(!thread_) {
			thread_.reset(new threading::thread(boost::bind(&vi_editor::run_thread, this)));
		}

		threading::lock l(mutex_);
		files_[fname] = server_name;
		++counter_;
	}

	std::string get_file_contents(const std::string& fname)
	{
		threading::lock l(mutex_);
		std::map<std::string, std::string>::const_iterator itor = file_contents_.find(fname);
		if(itor != file_contents_.end()) {
			return itor->second;
		} else {
			return "";
		}
	}

	int get_line(const std::string& fname) const
	{
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
	if(type == "vi") {
		return external_text_editor_ptr(new vi_editor(key));
	}

	return external_text_editor_ptr();
}

external_text_editor::external_text_editor()
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
