/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#if defined(__ANDROID__)

#include "filesystem.hpp"
#include "string_utils.hpp"
#include "asserts.hpp"

#include <fstream>
#include <sstream>

// Include files for opendir(3), readdir(3), etc.
// These files may vary from platform to platform,
// since these functions are NOT ANSI-conforming functions.
// They may have to be altered to port to new platforms
#include <sys/types.h>

//for mkdir
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

// for getenv
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>

#include <jni.h>
#include "SDL.h"
#include "SDL_screenkeyboard.h"
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include "SDL_rwops.h"
#include "preferences.hpp"

#include <boost/algorithm/string.hpp>

namespace sys
{

namespace {
#ifdef HAVE_CONFIG_H
  const std::string data_dir=DATADIR ;
  const bool have_datadir = true;
#else
  const std::string data_dir="";
  const bool have_datadir = false;
#endif

  const mode_t AccessMode = 00770;
}


void print_assets()
{
    AAssetManager* assetManager = SDL_ANDROID_JavaAssetManager();
    AAssetDir* assetDir = AAssetManager_openDir(assetManager, "images");
    const char* filename;
	__android_log_print(ANDROID_LOG_INFO,"Frogatto","print_assets()");
    while((filename = AAssetDir_getNextFileName(assetDir)) != 0) {
		__android_log_print(ANDROID_LOG_INFO,"Frogatto","File: %s",filename);
	}
    AAssetDir_close(assetDir);
}


void get_files_in_dir(const std::string& sdirectory,
					  std::vector<std::string>* files,
					  std::vector<std::string>* dirs,
					  FILE_NAME_MODE mode)
{
    AAssetManager* assetManager = SDL_ANDROID_JavaAssetManager();
    AAssetDir* assetDir; 
	std::string directory(sdirectory);
	int len = directory.length()-1;
	if(directory[len] == '/') {
		directory = directory.substr(0,len);
	}
	if(directory[0] == '.' && directory[1] == '/') {
		directory = directory.substr(2);
	}
	//LOG("get_files_in_dir() : " << directory << " : " << sdirectory);
	assetDir = AAssetManager_openDir(assetManager, directory.c_str());
    const char* filename;
	bool read_dirs_txt = false;
    while((filename = AAssetDir_getNextFileName(assetDir)) != 0) {
	//	__android_log_print(ANDROID_LOG_INFO,"Frogatto:get_files_in_dir","File: %s",filename);
		//LOG("get_files_in_dir() : " << filename);
        if(filename[0] != '.') {
			std::string s(filename);
			if(dirs != 0 && s.compare("dirs.txt") == 0) {
				read_dirs_txt = true;
			} else {
            	if(files) {
                	files->push_back(s);
				}
            }
        }
    }
    AAssetDir_close(assetDir);
	//LOG("get_files_in_dir() : after close");
	if(read_dirs_txt) {
		//LOG("get_files_in_dir() : read_files : " << directory << "/dirs.txt");
		std::string dir_list = read_file(directory + "/dirs.txt");
		while(dir_list[dir_list.length()-1] == '\n') {
			dir_list = dir_list.substr(0,dir_list.length()-1);
		}
		boost::split(*dirs, dir_list, boost::is_any_of("\n"));
		//LOG("get_files_in_dir() : after split");
	}

	if(files != NULL)
		std::sort(files->begin(),files->end());
	if (dirs != NULL)
		std::sort(dirs->begin(),dirs->end());
	//LOG("get_files_in_dir : after sorts");
}

void get_unique_filenames_under_dir(const std::string& sdir,
                                    std::map<std::string, std::string>* file_map)
{
	if(sdir.size() > 1024) {
		return;
	}
	std::string dir(sdir);
	if(dir[dir.length()-1] == '/') {
		dir = dir.substr(0,dir.length()-1);
	}

	std::vector<std::string> files;
	std::vector<std::string> dirs;
	get_files_in_dir(dir, &files, &dirs);
	//LOG("get_unique_filenames_under_dir(): " << dir);
	for(std::vector<std::string>::const_iterator i = files.begin();
	    i != files.end(); ++i) {
		(*file_map)[*i] = dir + "/" + *i;
		//LOG("unique: " << *i << " : " << (*file_map)[*i]);
	}

	for(std::vector<std::string>::const_iterator i = dirs.begin();
	    i != dirs.end(); ++i) {
		//LOG("get_unique_filenames_under_dir(): " << (dir + "/" + *i) << " : " << *i);
		get_unique_filenames_under_dir(dir + "/" + *i, file_map);
	}
}

std::string get_dir(const std::string& dir_path)
{
	DIR* dir = opendir(dir_path.c_str());
	if(dir == NULL) {
		const int res = mkdir(dir_path.c_str(),AccessMode);
		if(res == 0) {
			dir = opendir(dir_path.c_str());
		} else {
			std::cerr << "could not open or create directory: " << dir_path << '\n';
		}
	}

	if(dir == NULL)
		return "";

	closedir(dir);

	return dir_path;
}

std::string get_user_data_dir()
{
    std::string dir_path = preferences::user_data_path();
	DIR* dir = opendir(dir_path.c_str());
	if(dir == NULL) {
		const int res = mkdir(dir_path.c_str(),AccessMode);

		// Also create the maps directory
		mkdir((dir_path + "/editor").c_str(),AccessMode);
		mkdir((dir_path + "/saves").c_str(),AccessMode);
		if(res == 0) {
			dir = opendir(dir_path.c_str());
		} else {
			std::cerr << "could not open or create directory: " << dir_path << '\n';
		}
	}

	if(dir == NULL)
		return "";

	closedir(dir);

	return dir_path;
}

std::string get_saves_dir()
{
	const std::string dir_path = get_user_data_dir() + "/saves";
	return get_dir(dir_path);
}

bool do_file_exists(const std::string& fname)
{
	std::ifstream file(fname.c_str(), std::ios_base::binary);
	if(file.rdstate() == 0) {
        file.close();
        return true;
	}
	AAssetManager* assetManager = SDL_ANDROID_JavaAssetManager();
	AAsset* asset;
	if(fname[0] == '.' && fname[1] == '/') {
		asset = AAssetManager_open(assetManager, fname.substr(2).c_str(), AASSET_MODE_UNKNOWN);
	} else {
		asset = AAssetManager_open(assetManager, fname.c_str(), AASSET_MODE_UNKNOWN);
	}
    if(asset) {
        AAsset_close(asset);
        return true;
    }
    return false;
}

std::string find_file(const std::string& fname)
{
	if(do_file_exists(fname)) {
		return fname;
	}
	if(have_datadir) {
		std::string data_fname = data_dir + "/" + fname;
		if(do_file_exists(data_fname)) {
			return data_fname;
		}
	}
	return fname;
}

int64_t file_mod_time(const std::string& fname)
{
	/*struct stat buf;
	if(stat(fname.c_str(), &buf)) {
		std::cerr << "file_mod_time FAILED for '" << fname << "': ";
		switch(errno) {
		case EACCES: std::cerr << "EACCES\n"; break;
		case EBADF: std::cerr << "EBADF\n"; break;
		case EFAULT: std::cerr << "EFAULT\n"; break;
		case ENOENT: std::cerr << "ENOENT\n"; break;
		case ENOTDIR: std::cerr << "ENOTDIR\n"; break;
		default: std::cerr << "UNKNOWN ERROR " << errno << "\n"; break;
		}

		return 0;
	}
	return static_cast<int64_t>(buf.st_mtime);
	*/
	return 0;
}

bool file_exists(const std::string& name)
{
	return do_file_exists(find_file(name));
}


std::string read_file(const std::string& fname)
{
	AAssetManager* assetManager = SDL_ANDROID_JavaAssetManager();
	AAsset* asset;
	std::string name(fname);
	if(name[0] == '.' && name[1] == '/') {
		name = name.substr(2);
	}
	asset = AAssetManager_open(assetManager, name.c_str(), AASSET_MODE_RANDOM);
	if(asset != 0) {
		int len = AAsset_getLength(asset);
		std::vector<char> v;
		v.resize(len);
		if(AAsset_read(asset,&v[0],len)>0) {
			std::string s(v.begin(),v.end());
			AAsset_close(asset);
			return s;
		}
    	AAsset_close(asset);
		return 0;
	}

	// Couldn't find the file as an asset, try the standard filesystem
	std::string filename = find_file(fname);
	std::ifstream file(filename.c_str(),std::ios_base::binary);
	std::stringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

void write_file(const std::string& fname, const std::string& data)
{
	//Try to ensure that the dir the file is in exists.
	std::vector<std::string> components = util::split(fname, '/');
	if(!components.empty()) {
		components.pop_back();

		std::vector<std::string> tmp;
		while(components.empty() == false && get_dir(util::join(components, '/')).empty()) {
			tmp.push_back(components.back());
			components.pop_back();
		}

		while(!components.empty() && !tmp.empty()) {
			components.push_back(tmp.back());
			tmp.pop_back();

			get_dir(util::join(components, '/'));
		}
	}

	//Write the file.
	std::ofstream file(fname.c_str(),std::ios_base::binary);
	file << data;
}

static SDLCALL int aa_rw_seek(struct SDL_RWops* ops, int offset, int whence)
{
	return AAsset_seek((AAsset*)ops->hidden.unknown.data1, offset, whence);
}

static SDLCALL int aa_rw_read(struct SDL_RWops* ops, void *ptr, int size, int maxnum)
{
	return AAsset_read((AAsset*)ops->hidden.unknown.data1, ptr, maxnum * size) / size;
}

static SDLCALL int aa_rw_close(struct SDL_RWops* ops)
{
	AAsset_close((AAsset*)ops->hidden.unknown.data1);
	SDL_FreeRW(ops);

	return 0;
}

SDL_RWops* read_sdl_rw_from_asset(const std::string& name)
{
	AAssetManager* assetManager = SDL_ANDROID_JavaAssetManager();
	AAsset* asset;
	if(name[0] == '.' && name[1] == '/') {
		asset = AAssetManager_open(assetManager, name.substr(2).c_str(), AASSET_MODE_RANDOM);
	} else {
		asset = AAssetManager_open(assetManager, name.c_str(), AASSET_MODE_RANDOM);
	}
    if(!asset) {
        return 0;
    }
    SDL_RWops* ops = SDL_AllocRW();
    if(!ops) {
        AAsset_close(asset);
        return 0;
    }
	ops->hidden.unknown.data1 = asset;
	ops->read = aa_rw_read;
	ops->write = NULL;
	ops->seek = aa_rw_seek;
	ops->close = aa_rw_close;
	return ops;
}

}

#endif // ANDROID

