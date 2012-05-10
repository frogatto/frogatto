
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef FILESYSTEM_HPP_INCLUDED
#define FILESYSTEM_HPP_INCLUDED

#include <boost/cstdint.hpp>

#include <map>
#include <string>
#include <vector>

#if defined(__ANDROID__)
#include "SDL.h"
#include "SDL_rwops.h"
#endif

namespace sys
{

enum FILE_NAME_MODE { ENTIRE_FILE_PATH, FILE_NAME_ONLY };

//! Populates 'files' with all the files and
//! 'dirs' with all the directories in dir.
//! If files or dirs are NULL they will not be used.
//!
//! Mode determines whether the entire path or just the filename is retrieved.
void get_files_in_dir(const std::string& dir,
                      std::vector<std::string>* files,
                      std::vector<std::string>* dirs=NULL,
                      FILE_NAME_MODE mode=FILE_NAME_ONLY);

//Function which given a directory, will recurse through all sub-directories,
//and find each distinct filename. It will fill the files map such that the
//keys are filenames and the values are the full path to the file.
void get_unique_filenames_under_dir(const std::string& dir,
                                    std::map<std::string, std::string>* file_map);

//creates a dir if it doesn't exist and returns the path
std::string get_dir(const std::string& dir);
std::string get_user_data_dir();
std::string get_saves_dir();

std::string read_file(const std::string& fname);
void write_file(const std::string& fname, const std::string& data);

bool file_exists(const std::string& fname);
std::string find_file(const std::string& name);

int64_t file_mod_time(const std::string& fname);

#if defined(__ANDROID__)
SDL_RWops* read_sdl_rw_from_asset(const std::string& name);
void print_assets();
#endif // ANDROID

void move_file(const std::string& from, const std::string& to);
void remove_file(const std::string& fname);


}

#endif

