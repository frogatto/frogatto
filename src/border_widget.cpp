#include "graphics.hpp"
#include "border_widget.hpp"
#include "raster.hpp"

namespace gui {

border_widget::border_widget(widget_ptr child, graphics::color col, int border_size)
  : child_(child), color_(col), border_size_(border_size)
{
	set_dim(child->width() + border_size*2, child->height() + border_size*2);
	child_->set_loc(border_size, border_size);
}

void border_widget::set_color(const graphics::color& col)
{
	color_ = col;
}

void border_widget::handle_draw() const
{
	glPushMatrix();
	graphics::draw_rect(rect(x(), y(), width(), height()), color_);
	glTranslatef(x(), y(), 0.0);
	child_->draw();
	glPopMatrix();
}

bool border_widget::handle_event(const SDL_Event& event, bool claimed)
{
	SDL_Event ev = event;
	normalize_event(&ev);
	return child_->process_event(ev, claimed);
}

}
