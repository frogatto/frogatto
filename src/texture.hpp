
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef TEXTURE_HPP_INCLUDED
#define TEXTURE_HPP_INCLUDED

#include <bitset>
#include <string>
#include <boost/shared_ptr.hpp>
#include <vector>

#include <GL/gl.h>

#include "surface.hpp"

namespace graphics
{

class texture
{
public:
	//create an instance of this object before using the first texture,
	//and destroy it before the program exits
	struct manager {
		manager();
		~manager();
	};

	enum OPTION { NO_MIPMAP, PRETTY_SCALING, NUM_OPTIONS };
	typedef std::bitset<NUM_OPTIONS> options_type;
	static void clear_textures();

	texture() : width_(0), height_(0) {}

	typedef std::vector<surface> key;

	void set_as_current_texture() const;
	bool valid() const { return id_; }

	static texture get_frame_buffer();
	static texture get(const std::string& str, options_type options=options_type());
	static texture get(const std::string& str, const std::string& algorithm, options_type options=options_type());
	static texture get(const key& k, options_type options=options_type());
	static texture get(const surface& surf, options_type options=options_type());
	static texture get_no_cache(const key& k, options_type options=options_type());
	static texture get_no_cache(const surface& surf, options_type options=options_type());
	static void set_current_texture(const key& k);
	static void set_coord(GLfloat x, GLfloat y);
	static void clear_cache();

	unsigned int width() const { return width_; }
	unsigned int height() const { return height_; }

	friend bool operator==(const texture&, const texture&);
	friend bool operator<(const texture&, const texture&);

private:
	explicit texture(const key& surfs, options_type options=options_type());

	struct ID {
		explicit ID(GLuint id) : id(id) {
		}

		~ID();

		GLuint id;
	};
	boost::shared_ptr<ID> id_;
	unsigned int width_, height_;
	GLfloat ratio_w_, ratio_h_;
};

inline bool operator==(const texture& a, const texture& b)
{
	return a.id_ == b.id_;
}

inline bool operator!=(const texture& a, const texture& b)
{
	return !operator==(a, b);
}

inline bool operator<(const texture& a, const texture& b)
{
	if(!a.id_) {
		return false;
	} else if(!b.id_) {
		return true;
	}

	return a.id_->id < b.id_->id;
}

}

#endif
