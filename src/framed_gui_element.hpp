#ifndef FRAMED_GUI_ELEMENT_HPP_INCLUDED
#define FRAMED_GUI_ELEMENT_HPP_INCLUDED

#include "gui_section.hpp"


#include <boost/shared_ptr.hpp>



class framed_gui_element;
typedef boost::shared_ptr<const framed_gui_element> const_framed_gui_element_ptr;


class framed_gui_element
{
public:
	void blit(int x, int y, int w, int h) const;
	framed_gui_element();

	
private:
	const_gui_section_ptr  top_border_;
	const_gui_section_ptr  bottom_border_;
	const_gui_section_ptr  left_border_;
	const_gui_section_ptr  right_border_;
	
	const_gui_section_ptr  top_right_corner_;
	const_gui_section_ptr  top_left_corner_;
	const_gui_section_ptr  bottom_right_corner_;
	const_gui_section_ptr  bottom_left_corner_;
	
	const_gui_section_ptr interior_fill_;  //later on, we might want to do this as a proper pattern.  For now, we're gonna imp this as a stretch-to-fill because it doesn't matter with our current graphics (since they're just a solid color).

};

#endif