#include "asserts.hpp"
#include "graphics.hpp"
#include "border_widget.hpp"
#include "raster.hpp"
#include "widget_factory.hpp"

namespace gui {

border_widget::border_widget(widget_ptr child, graphics::color col, int border_size)
  : child_(child), color_(col), border_size_(border_size)
{
	set_dim(child->width() + border_size*2, child->height() + border_size*2);
	child_->set_loc(border_size, border_size);
}

border_widget::border_widget(const variant& v, const game_logic::formula_callable_ptr& e) : widget(v,e)
{
	ASSERT_LOG(v.is_map(), "TYPE ERROR: parameter to border widget must be a map");
	color_ = v.has_key("color") ? graphics::color(0,0,0,255) : graphics::color(v["color"]);
	border_size_ = v.has_key("border_size") ? v["border_size"].as_int() : 2;
	child_ = widget_factory::create(v["child"], e);
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
