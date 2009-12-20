
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

	static void clear_textures();

	texture();
	texture(const texture& t);
	~texture();

	typedef std::vector<surface> key;

	void set_as_current_texture() const;
	bool valid() const { return id_; }

	static texture get(const std::string& str);
	static texture get(const std::string& str, const std::string& algorithm);
	static texture get(const key& k);
	static texture get_no_cache(const key& k);
	static texture get_no_cache(const surface& surf);
	static void set_current_texture(const key& k);
	static GLfloat get_coord_x(GLfloat x);
	static GLfloat get_coord_y(GLfloat y);
	GLfloat translate_coord_x(GLfloat x) const;
	GLfloat translate_coord_y(GLfloat y) const;
	static void clear_cache();

	unsigned int width() const { return width_; }
	unsigned int height() const { return height_; }

	bool is_alpha(int x, int y) const { return (*alpha_map_)[y*width_ + x]; }

	friend bool operator==(const texture&, const texture&);
	friend bool operator<(const texture&, const texture&);

private:
	void initialize();

	static texture get(const surface& surf);
	explicit texture(const key& surfs);

	struct ID {
		ID() : id(GLuint(-1)) {
		}

		explicit ID(GLuint id) : id(id) {
		}

		~ID();

		void destroy();

		bool init() const { return id != GLuint(-1); }

		GLuint id;

		//before we've constructed the ID, we can store the
		//surface in here.
		surface s;
	};
	mutable boost::shared_ptr<ID> id_;
	unsigned int width_, height_;
	GLfloat ratio_w_, ratio_h_;

	key key_;
	boost::shared_ptr<std::vector<bool> > alpha_map_;
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
