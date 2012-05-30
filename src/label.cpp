
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <iostream>

#include "font.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "i18n.hpp"

namespace gui {

label::label(const std::string& text, int size)
	: text_(i18n::tr(text)), size_(size),
	  fixed_width_(false)
{
	color_.r = color_.g = color_.b = 255;
	recalculate_texture();
}

label::label(const std::string& text, const SDL_Color& color, int size)
	: text_(i18n::tr(text)), color_(color), size_(size),
	  fixed_width_(false)
{
	recalculate_texture();
}

label::label(const variant& v, const game_logic::formula_callable_ptr& e)
	: widget(v,e), fixed_width_(false)
{
	text_ = i18n::tr(v["text"].as_string());
	color_ = v.has_key("color") 
		? graphics::color(v["color"]).as_sdl_color() 
		: graphics::color(255,255,255,255).as_sdl_color();
	size_ = v.has_key("size") ? v["size"].as_int() : 14;
	recalculate_texture();
}

void label::set_color(const SDL_Color& color)
{
	color_ = color;
	recalculate_texture();
}

void label::set_font_size(int size)
{
	size_ = size;
	recalculate_texture();
}

void label::set_text(const std::string& text)
{
	text_ = i18n::tr(text);
	reformat_text();
	recalculate_texture();
}

std::string& label::current_text() {
	if(fixed_width_) {
		return formatted_;
	}
	return text_;
}

void label::set_fixed_width(bool fixed_width)
{
	fixed_width_ = fixed_width;
	reformat_text();
	recalculate_texture();
}

void label::set_dim(int w, int h) {
	if(w != width() || h != height()) {
		inner_set_dim(w, h);
		reformat_text();
		recalculate_texture();
	}
}

void label::inner_set_dim(int w, int h) {
	widget::set_dim(w, h);
}

void label::reformat_text()
{
	if(fixed_width_) {
		formatted_ = text_;
	}
}

void label::recalculate_texture()
{
	texture_ = font::render_text(current_text(), color_, size_);
	inner_set_dim(texture_.width(),texture_.height());
}

void label::handle_draw() const
{
	graphics::blit_texture(texture_, x(), y());
}

void label::set_texture(graphics::texture t) {
	texture_ = t;
}

variant label::get_value(const std::string& key) const
{
	if(key == "text") {
		return variant(text_);
	} else if(key == "color") {
		return graphics::color(color_.r, color_.g, color_.b, color_.unused).write();
	} else if(key == "size") {
		return variant(size_);
	}
	return widget::get_value(key);
}

void label::set_value(const std::string& key, const variant& v)
{
	if(key == "text") {
		set_text(v.as_string());
	} else if(key == "color") {
		set_color(graphics::color(v).as_sdl_color());
	} else if(key == "size") {
		set_font_size(v.as_int());
	}
	widget::set_value(key, v);
}

dialog_label::dialog_label(const std::string& text, const SDL_Color& color, int size)
	: label(text, color, size), progress_(0) {

	recalculate_texture();
}

dialog_label::dialog_label(const variant& v, const game_logic::formula_callable_ptr& e)
	: label(v, e), progress_(0)
{
	recalculate_texture();
}

void dialog_label::set_progress(int progress)
{
	progress_ = progress;
	recalculate_texture();
}

void dialog_label::recalculate_texture()
{
	label::recalculate_texture();
	stages_ = current_text().size();
	int prog = progress_;
	if(prog < 0) prog = 0;
	if(prog > stages_) prog = stages_;
	std::string txt = current_text().substr(0, prog);

	if(prog > 0) {
		set_texture(font::render_text(txt, color(), size()));
	} else {
		set_texture(graphics::texture());
	}
}

void dialog_label::set_value(const std::string& key, const variant& v)
{
	if(key == "progress") {
		set_progress(v.as_int());
	}
	label::set_value(key, v);
}

variant dialog_label::get_value(const std::string& key) const
{
	if(key == "progress") {
		return variant(progress_);
	}
	return label::get_value(key);
}

label_factory::label_factory(const SDL_Color& color, int size)
   : color_(color), size_(size)
{}

label_ptr label_factory::create(const std::string& text) const
{
	return label_ptr(new label(text,color_,size_));
}

label_ptr label_factory::create(const std::string& text,
                                const std::string& tip) const
{
	const label_ptr res(create(text));
	res->set_tooltip(tip);
	return res;
}

}
