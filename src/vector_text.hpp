#pragma once
#ifndef VECTOR_TEXT_HPP_INCLUDED
#define VECTOR_TEXT_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <vector>

#include "formula_callable.hpp"
#include "geometry.hpp"
#include "graphics.hpp"
#include "texture.hpp"
#include "variant.hpp"

namespace gui {

typedef std::pair<graphics::texture, point> offset_texture;

class vector_text : public virtual game_logic::formula_callable
{
public:
	enum TEXT_ALIGNMENT{ ALIGN_LEFT = -1, ALIGN_CENTER, ALIGN_RIGHT };
	vector_text(const variant& v);

	int x() const { return draw_area_.x(); }
	int y() const { return draw_area_.y(); }
	size_t width() const { return draw_area_.w(); }
	size_t height() const { return draw_area_.h(); }
	rect draw_area() const { return draw_area_; }

	bool visible() { return visible_; }
	void set_visible(bool visible) { visible_ = visible; }

	int size() const { return size_; }

	void set_text(const std::string& txt);
	void set_font(const std::string& fnt);
	void set_size(int size);
	void set_color(const variant& node);
	void set_align(const std::string& align);
	void set_align(TEXT_ALIGNMENT align);

	void draw() const { if(visible_) { handle_draw(); } }
protected:
	virtual ~vector_text()
	{}
	virtual void handle_draw() const;

	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);
private:
	void recalculate_texture();

	bool visible_;
	int size_;
	std::vector<offset_texture> textures_;
	std::string text_;
	std::string font_;
	SDL_Color color_;
	rect draw_area_;
	TEXT_ALIGNMENT align_;
};

typedef boost::intrusive_ptr<vector_text> vector_text_ptr;
typedef boost::intrusive_ptr<const vector_text> const_vector_text_ptr;

}

#endif
