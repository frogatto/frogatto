#include "asserts.hpp"
#include "graphical_font_label.hpp"

namespace gui {

graphical_font_label::graphical_font_label(
       const std::string& text, const std::string& font, int size)
  : text_(text), font_(graphical_font::get(font)), size_(size)
{
	ASSERT_LOG(font_.get(), "UNKNOWN FONT: " << font);
	rect dim = font_->dimensions(text_, size_);
	widget::set_dim(dim.w(), dim.h());
}

void graphical_font_label::handle_draw() const
{
	font_->draw(x(), y(), text_, size_);
}

}
