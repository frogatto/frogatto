#include <iostream>

#include "framed_gui_element.hpp"
#include "gui_section.hpp"
#include "geometry.hpp"
#include "raster.hpp"



framed_gui_element::framed_gui_element()
: area_(rect("4,44,29,69")),corner_height_(8),
texture_(graphics::texture::get("gui/buttons-and-windows.png"))
{

	
	top_left_corner_ = rect(area_.x(),area_.y(),corner_height_,corner_height_);
	top_right_corner_ = rect(area_.x2() - corner_height_,area_.y(),corner_height_,corner_height_);
	bottom_left_corner_ = rect(area_.x(),area_.y2() - corner_height_,corner_height_,corner_height_);
	bottom_right_corner_ = rect(area_.x2() - corner_height_,area_.y2() - corner_height_,corner_height_,corner_height_);
	
	top_border_ = rect(area_.x() + corner_height_, area_.y(),area_.w() - corner_height_ * 2, corner_height_);
	bottom_border_ = rect(area_.x() + corner_height_, area_.y2() - corner_height_,area_.w() - corner_height_ * 2, corner_height_);
	left_border_ = rect(area_.x(), area_.y() + corner_height_,corner_height_, area_.h() - corner_height_ * 2);
	right_border_ = rect(area_.x2() - corner_height_, area_.y() + corner_height_,corner_height_,area_.h() - corner_height_ * 2);
	
	interior_fill_ = rect(area_.x() + corner_height_, area_.y() + corner_height_,area_.w() - corner_height_ * 2,area_.h() - corner_height_ * 2);
}

void framed_gui_element::blit(int x, int y, int w, int h) const
{
	blit_subsection(interior_fill_,x,y,w,h);
	
	blit_subsection(top_border_,x,y,w,top_border_.h());
	blit_subsection(bottom_border_,x,y + h - bottom_border_.h(),w,bottom_border_.h());
	blit_subsection(left_border_,x,y,left_border_.w(),h);
	blit_subsection(right_border_,x + w - right_border_.w(), y,right_border_.w(),h);
	
	blit_subsection(top_left_corner_,x,y,top_left_corner_.w(),top_left_corner_.h());
	blit_subsection(top_right_corner_,x + w - top_right_corner_.w(),y, top_right_corner_.w(), top_right_corner_.h());
	blit_subsection(bottom_left_corner_,x,y + h - bottom_left_corner_.h(),bottom_left_corner_.w(), bottom_left_corner_.h());
	blit_subsection(bottom_right_corner_,x + w - bottom_right_corner_.w(),y + h - bottom_right_corner_.h(),bottom_right_corner_.w(), bottom_right_corner_.h());
	
	
	
	
}

void framed_gui_element::blit_subsection(rect subsection, int x, int y, int w, int h) const
{
	graphics::blit_texture(texture_, x, y, w, h, 0.0,
	                       GLfloat(subsection.x())/GLfloat(texture_.width()),
	                       GLfloat(subsection.y())/GLfloat(texture_.height()),
	                       GLfloat(subsection.x2())/GLfloat(texture_.width()),
	                       GLfloat(subsection.y2())/GLfloat(texture_.height()));

}
