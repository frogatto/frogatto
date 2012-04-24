
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
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

#include "graphics.hpp"
#include "surface.hpp"

namespace graphics
{

class texture
{
public:
	enum {NO_STRIP_SPRITESHEET_ANNOTATIONS = 1};
	//error thrown if an operation is done from a worker thread that
	//must be completed by the main graphics thread.
	struct worker_thread_error {};

	//create an instance of this object before using the first texture,
	//and destroy it before the program exits
	struct manager {
		manager();
		~manager();
	};

	static void clear_textures();

	//complete construction of any textures that were accessed in worker threads
	//but which need to be completed in the main thread. May only be called
	//in the main thread.
	static void build_textures_from_worker_threads();

	texture();
	texture(const texture& t);
	~texture();

	typedef std::vector<surface> key;
	static surface build_surface_from_key(const key& k, unsigned int surf_width, unsigned int surf_height);

	unsigned int get_id() const;
	static void set_current_texture(unsigned int id);
	void set_as_current_texture() const;
	bool valid() const { return id_; }

	static texture get(const std::string& str, int options=0);
	static texture get(const std::string& str, const std::string& algorithm);
	static texture get_palette_mapped(const std::string& str, int palette);
	static texture get_no_cache(const surface& surf);
	static GLfloat get_coord_x(GLfloat x);
	static GLfloat get_coord_y(GLfloat y);
	GLfloat translate_coord_x(GLfloat x) const;
	GLfloat translate_coord_y(GLfloat y) const;
	static void clear_cache();

	unsigned int width() const { return width_; }
	unsigned int height() const { return height_; }

	bool is_alpha(int x, int y) const { return (*alpha_map_)[y*width_ + x]; }
	std::vector<bool>::const_iterator get_alpha_row(int x, int y) const { return alpha_map_->begin() + y*width_ + x; }
	std::vector<bool>::const_iterator end_alpha() const { return alpha_map_->end(); }

	const unsigned char* color_at(int x, int y) const;

	friend bool operator==(const texture&, const texture&);
	friend bool operator<(const texture&, const texture&);

	void initialize(const key& k, int options=0);
	explicit texture(const key& surfs, int options=0);

	static void rebuild_all();
	static void unbuild_all();

	struct ID {
		ID();
		~ID();

		void build_id();
		void unbuild_id();
		void destroy();

		bool init() const { return id != static_cast<unsigned int>(-1); }

		unsigned int id;

		//before we've constructed the ID, we can store the
		//surface in here.
		surface s;

		int width, height;
	};

private:
	static texture get_no_cache(const key& k);

	mutable boost::shared_ptr<ID> id_;
	unsigned int width_, height_;
	GLfloat ratio_w_, ratio_h_;

	boost::shared_ptr<std::vector<bool> > alpha_map_;

	//a list of ID objects that we assigned GL ID's to in a worker thread,
	//but which need binding to a texture in the main thread.
	static std::vector<boost::shared_ptr<ID> > id_to_build_;
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

unsigned int map_color_to_16bpp(unsigned int color);

}

#endif
