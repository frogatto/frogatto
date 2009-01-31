#include <iostream>

#include "framed_gui_element.hpp"
#include "gui_section.hpp"



framed_gui_element::framed_gui_element(){
	//change to actually load from a proper wml node later
	top_left_corner_ = gui_section::get("button_top_left");
	top_right_corner_ = gui_section::get("button_top_right");
	bottom_left_corner_ = gui_section::get("button_bottom_left");
	bottom_right_corner_ = gui_section::get("button_bottom_right");

	top_border_ = gui_section::get("button_top_border");
	bottom_border_ = gui_section::get("button_bottom_border");
	left_border_ = gui_section::get("button_left_border");
	right_border_ = gui_section::get("button_right_border");
	
	interior_fill_ = gui_section::get("button_interior_fill");
	
}

void framed_gui_element::blit(int x, int y, int w, int h) const
{
	//TODO:  add a branch and a parameter to allow us to specify the state the button should be in (hover, pressed, normal)
	interior_fill_->blit(x,y,w,h);
	
	top_border_->blit(x,y,w,top_border_->height());
	bottom_border_->blit(x,y + h - bottom_border_->height(),w,bottom_border_->height());
	left_border_->blit(x,y,left_border_->width(),h);
	right_border_->blit(x + w - right_border_->width(), y,right_border_->width(),h);
	
	top_left_corner_->blit(x,y,top_left_corner_->width(),top_left_corner_->height());
	top_right_corner_->blit(x + w - top_right_corner_->width(),y, top_right_corner_->width(), top_right_corner_->height());
	bottom_left_corner_->blit(x,y + h - bottom_left_corner_->height(),bottom_left_corner_->width(), bottom_left_corner_->height());
	bottom_right_corner_->blit(x + w - bottom_right_corner_->width(),y + h - bottom_right_corner_->height(),bottom_right_corner_->width(), bottom_right_corner_->height());
	
}