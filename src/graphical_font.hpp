#ifndef GRAPHICAL_FONT_HPP_INCLUDED
#define GRAPHICAL_FONT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <string>
#include <vector>

#include "geometry.hpp"
#include "texture.hpp"
#include "variant.hpp"

class graphical_font;
typedef boost::shared_ptr<graphical_font> graphical_font_ptr;
typedef boost::shared_ptr<const graphical_font> const_graphical_font_ptr;

class graphical_font
{
public:
	static void init(variant node);
	static const_graphical_font_ptr get(const std::string& id);
	explicit graphical_font(variant node);
	const std::string& id() const { return id_; }
	rect draw(int x, int y, const std::string& text, int size=2) const;
	rect dimensions(const std::string& text, int size=2) const;

private:
	rect do_draw(int x, int y, const std::string& text, bool draw_text, int size) const;

	std::string id_;

	graphics::texture texture_;
	//hashmap to map characters to rectangles in the texture
	typedef boost::unordered_map<unsigned int, rect> char_rect_map;
	char_rect_map char_rect_map_;
	int kerning_;
};

#endif
