#ifndef GRAPHICAL_FONT_HPP_INCLUDED
#define GRAPHICAL_FONT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

#include "geometry.hpp"
#include "texture.hpp"
#include "wml_node_fwd.hpp"

class graphical_font;
typedef boost::shared_ptr<graphical_font> graphical_font_ptr;
typedef boost::shared_ptr<const graphical_font> const_graphical_font_ptr;

class graphical_font
{
public:
	static void init(wml::const_node_ptr node);
	static const_graphical_font_ptr get(const std::string& id);
	explicit graphical_font(wml::const_node_ptr node);
	const std::string& id() const { return id_; }
	rect draw(int x, int y, const std::string& text) const;
	rect dimensions(const std::string& text) const;

private:
	rect do_draw(int x, int y, const std::string& text, bool draw_text=true) const;

	std::string id_;

	graphics::texture texture_;
	std::vector<rect> char_rect_map_;
	int kerning_;
};

#endif
