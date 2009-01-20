#ifndef GUI_SECTION_HPP_INCLUDED
#define GUI_SECTION_HPP_INCLUDED

#include <string>

#include <boost/shared_ptr.hpp>

#include "geometry.hpp"
#include "texture.hpp"
#include "wml_node_fwd.hpp"

class gui_section;
typedef boost::shared_ptr<const gui_section> const_gui_section_ptr;

class gui_section
{
public:
	static void init(wml::const_node_ptr node);
	static const_gui_section_ptr get(const std::string& key);

	explicit gui_section(wml::const_node_ptr node);

	void blit(int x, int y) const { blit(x, y, width(), height()); }
	void blit(int x, int y, int w, int h) const;
	int width() const { return area_.w()*2; }
	int height() const { return area_.h()*2; }
private:
	graphics::texture texture_;
	rect area_;
};

#endif
