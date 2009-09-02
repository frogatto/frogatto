
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "concurrent_cache.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "surface_formula.hpp"
#include "texture.hpp"
#include "thread.hpp"
#include <GL/gl.h>
#include <GL/glu.h>
#include <map>
#include <set>
#include <iostream>
#include <cstring>

namespace graphics
{

surface scale_surface(surface input);

namespace {
	std::set<texture*>& texture_registry() {
		static std::set<texture*>* reg = new std::set<texture*>;
		return *reg;
	}

	threading::mutex& texture_registry_mutex() {
		static threading::mutex* m = new threading::mutex;
		return *m;
	}

	void add_texture_to_registry(texture* t) {
		threading::lock lk(texture_registry_mutex());
		texture_registry().insert(t);
	}

	void remove_texture_from_registry(texture* t) {
		threading::lock lk(texture_registry_mutex());
		texture_registry().erase(t);
	}

	typedef concurrent_cache<texture::key,graphics::texture> texture_map;
	texture_map texture_cache;
	const size_t TextureBufSize = 128;
	GLuint texture_buf[TextureBufSize];
	size_t texture_buf_pos = TextureBufSize;
	std::vector<GLuint> avail_textures;
	bool graphics_initialized = false;

	GLuint current_texture = 0;

	GLuint get_texture_id() {
		if(!avail_textures.empty()) {
			const GLuint res = avail_textures.back();
			avail_textures.pop_back();
			return res;
		}

		if(texture_buf_pos == TextureBufSize) {
			glGenTextures(TextureBufSize, texture_buf);
			texture_buf_pos = 0;
		}

		return texture_buf[texture_buf_pos++];
	}

	bool npot_allowed = true;
	GLfloat width_multiplier = -1.0;
	GLfloat height_multiplier = -1.0;

	unsigned int next_power_of_2(unsigned int n)
	{
		--n;
		n = n|(n >> 1);
		n = n|(n >> 2);
		n = n|(n >> 4);
		n = n|(n >> 8);
		n = n|(n >> 16);
		++n;
		return n;
	}

	bool is_npot_allowed()
    {
	static bool once = false;
	static bool npot = true;
	if (once) return npot;
	once = true;

	const char *supported = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
	const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	const char *vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));

	std::cerr << "OpenGL version: " << version << "\n";
	std::cerr << "OpenGL vendor: " << vendor << "\n";
	std::cerr << "OpenGL extensions: " << supported << "\n";

	// OpenGL >= 2.0 drivers must support NPOT textures
	bool version_2 = (version[0] >= '2');
	npot = version_2;
	// directly test for NPOT extension
	if (std::strstr(supported, "GL_ARB_texture_non_power_of_two")) npot = true;

	if (npot) {
		// Use some heuristic to make sure it is HW accelerated. Might need some
		// more work.
		if (std::strstr(vendor, "NVIDIA Corporation")) {
			if (!std::strstr(supported, "NV_fragment_program2") ||
				!std::strstr(supported, "NV_vertex_program3")) {
					npot = false;
				}
			} else if (std::strstr(vendor, "ATI Technologies")) {
					// TODO: Investigation note: my ATI card works fine for npot textures as long
					// as mipmapping is enabled. otherwise it runs slowly. Work out why. --David
				//if (!std::strstr(supported, "GL_ARB_texture_non_power_of_two"))
					npot = false;
		}
	}
	if(!npot) {
		std::cerr << "Using only pot textures\n";
	}
	return npot;
    }

	std::string mipmap_type_to_string(GLenum type) {
		switch(type) {
		case GL_NEAREST:
			return "N";
		case GL_LINEAR:
			return "L";
		case GL_NEAREST_MIPMAP_NEAREST:
			return "NN";
		case GL_NEAREST_MIPMAP_LINEAR:
			return "NL";
		case GL_LINEAR_MIPMAP_NEAREST:
			return "LN";
		case GL_LINEAR_MIPMAP_LINEAR:
			return "LL";
		default:
			return "??";
		}
	}
}

texture::manager::manager() {
	assert(!graphics_initialized);

	width_multiplier = 1.0;
	height_multiplier = 1.0;

	graphics_initialized = true;
}

texture::manager::~manager() {
	graphics_initialized = false;
}

void texture::clear_textures()
{
	//go through all the textures and clear out the ID's. We only want to
	//re-initialize each shared ID once.
	threading::lock lk(texture_registry_mutex());
	for(std::set<texture*>::iterator i = texture_registry().begin(); i != texture_registry().end(); ++i) {
		texture& t = **i;
		if(t.id_) {
			t.id_->destroy();
		}
	}

	//go through and initialize anyone's ID which hasn't been initialized
	//already.
	for(std::set<texture*>::iterator i = texture_registry().begin(); i != texture_registry().end(); ++i) {
		texture& t = **i;
		if(t.id_ && t.id_->s.get() == NULL) {
			t.initialize();
		}
	}
}

texture::texture() : width_(0), height_(0)
{
	add_texture_to_registry(this);
}

texture::texture(const key& surfs)
   : width_(0), height_(0), ratio_w_(1.0), ratio_h_(1.0), key_(surfs)
{
	add_texture_to_registry(this);
	initialize();
}

texture::texture(const texture& t)
  : id_(t.id_), width_(t.width_), height_(t.height_),
   ratio_w_(t.ratio_w_), ratio_h_(t.ratio_h_), key_(t.key_),
   alpha_map_(t.alpha_map_)
{
	add_texture_to_registry(this);
}

texture::~texture()
{
	remove_texture_from_registry(this);
}

void texture::initialize()
{
	assert(graphics_initialized);
	if(key_.empty() ||
	   std::find(key_.begin(),key_.end(),surface()) != key_.end()) {
		return;
	}

	npot_allowed = is_npot_allowed();

	width_ = key_.front()->w;
	height_ = key_.front()->h;
	alpha_map_.resize(width_*height_);

	unsigned int surf_width = width_;
	unsigned int surf_height = height_;
	if(!npot_allowed) {
		surf_width = next_power_of_2(surf_width);
		surf_height = next_power_of_2(surf_height);
//		surf_width = surf_height =
//		   std::max(next_power_of_2(surf_width),
//		            next_power_of_2(surf_height));
		ratio_w_ = GLfloat(width_)/GLfloat(surf_width);
		ratio_h_ = GLfloat(height_)/GLfloat(surf_height);
	}


	surface s(SDL_CreateRGBSurface(SDL_SWSURFACE,surf_width,surf_height,32,SURFACE_MASK));

	for(key::const_iterator i = key_.begin(); i != key_.end(); ++i) {
		if(i == key_.begin()) {
			SDL_SetAlpha(i->get(), 0, SDL_ALPHA_OPAQUE);
		} else {
			SDL_SetAlpha(i->get(), SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
		}

		SDL_BlitSurface(i->get(),NULL,s.get(),NULL);
	}

	const int npixels = surf_width*surf_height;
	for(int n = 0; n != npixels; ++n) {
		//we use a color in our sprite sheets to indicate transparency, rather than an alpha channel
		static const unsigned char AlphaPixel[] = {0x6f, 0x6d, 0x51}; //the background color, brown
		static const unsigned char AlphaPixel2[] = {0xf9, 0x30, 0x3d}; //the border color, red
		unsigned char* pixel = reinterpret_cast<unsigned char*>(s->pixels) + n*4;

		if(pixel[0] == AlphaPixel[0] && pixel[1] == AlphaPixel[1] && pixel[2] == AlphaPixel[2] ||
		   pixel[0] == AlphaPixel2[0] && pixel[1] == AlphaPixel2[1] && pixel[2] == AlphaPixel2[2]) {
			pixel[3] = 0;

			const int x = n%surf_width;
			const int y = n/surf_width;
			if(x < width_ && y < height_) {
				alpha_map_[y*width_ + x] = true;
			}
		}
	}

	if(!id_) {
		id_.reset(new ID);
	}

	id_->s = s;

	current_texture = 0;
}

void texture::set_as_current_texture() const
{
	if(!valid()) {
		glBindTexture(GL_TEXTURE_2D,0);
		current_texture = 0;
		return;
	}

	if(id_->init() == false) {
		if(preferences::use_pretty_scaling()) {
			id_->s = scale_surface(id_->s);
		}

		id_->id = get_texture_id();
		glBindTexture(GL_TEXTURE_2D,id_->id);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D,0,4,id_->s->w,id_->s->h,0,GL_RGBA,
				     GL_UNSIGNED_BYTE,id_->s->pixels);

		//free the surface.
		id_->s = surface();
	}

	if(current_texture == id_->id) {
		return;
	}

	current_texture = id_->id;

	glBindTexture(GL_TEXTURE_2D,id_->id);
	width_multiplier = ratio_w_;
	height_multiplier = ratio_h_;
}

texture texture::get_frame_buffer()
{
	texture t;
	t.id_.reset(new ID(get_texture_id()));
	int width = screen_width();
	int height = screen_height();

	int actual_width = width;
	int actual_height = height;
	t.ratio_w_ = 1.0;
	t.ratio_h_ = 1.0;

	if(!npot_allowed) {
		actual_width = actual_height =
		   std::max(next_power_of_2(actual_width),
		            next_power_of_2(actual_height));
		t.ratio_w_ = GLfloat(width)/GLfloat(actual_width);
		t.ratio_h_ = GLfloat(height)/GLfloat(actual_height);
	}

	t.width_ = actual_width;
	t.height_ = actual_height;

	t.set_as_current_texture();
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, actual_width, actual_height, 0);

	return t;
}

texture texture::get(const std::string& str)
{
	texture result(get(surface_cache::get(str)));

	return result;
}

texture texture::get(const std::string& str, const std::string& algorithm)
{
	texture result(get(get_surface_formula(surface_cache::get(str), algorithm)));
	return result;
}

texture texture::get(const key& surfs)
{
	texture result = texture_cache.get(surfs);
	if(!result.valid()) {
		result = texture(surfs);

		texture_cache.put(surfs, result);
	}

	return result;
}

texture texture::get(const surface& surf)
{
	return get(key(1,surf));
}

texture texture::get_no_cache(const key& surfs)
{
	return texture(surfs);
}

texture texture::get_no_cache(const surface& surf)
{
	return texture(key(1,surf));
}

void texture::set_current_texture(const key& k)
{
	texture t(get(k));
	t.set_as_current_texture();
}

void texture::set_coord(GLfloat x, GLfloat y)
{
	if(npot_allowed) {
		glTexCoord2f(x,y);
	} else {
		glTexCoord2f(x*width_multiplier,y*height_multiplier);
	}
}

void texture::clear_cache()
{
	texture_cache.clear();
}

texture::ID::~ID()
{
	destroy();
}

void texture::ID::destroy()
{
	if(graphics_initialized && init()) {
		avail_textures.push_back(id);
	}

	id = GLuint(-1);
	s = surface();
}

}
