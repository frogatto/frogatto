#include "asserts.hpp"
#include "graphical_font_label.hpp"

namespace gui {

graphical_font_label::graphical_font_label(
       const std::string& text, const std::string& font, int size)
  : text_(text), font_(graphical_font::get(font)), size_(size)
{
	set_environment();
	ASSERT_LOG(font_.get(), "UNKNOWN FONT: " << font);
	reset_text_dimensions();
}

graphical_font_label::graphical_font_label(const variant& v, game_logic::formula_callable* e)
	: widget(v,e)
{
	text_ = v["text"].as_string();
	font_ = graphical_font::get(v["font"].as_string());
	ASSERT_LOG(font_.get(), "UNKNOWN FONT: " << v["font"].as_string());
	size_ = v["size"].as_int();
	reset_text_dimensions();
}

void graphical_font_label::handle_draw() const
{
	font_->draw(x(), y(), text_, size_);
}

void graphical_font_label::reset_text_dimensions()
{
	rect dim = font_->dimensions(text_, size_);
	widget::set_dim(dim.w(), dim.h());
}

void graphical_font_label::set_text(const std::string& text)
{
	text_ = text;
	reset_text_dimensions();
}

void graphical_font_label::set_value(const std::string& key, const variant& v)
{
	if(key == "text") {
		set_text(v.as_string());
	} else if(key == "font") {
		font_ = graphical_font::get(v.as_string());
		ASSERT_LOG(font_.get(), "UNKNOWN FONT: " << v.as_string());
		reset_text_dimensions();
	} else if(key == "size") {
		size_ = v.as_int();
		reset_text_dimensions();
	}
	//widget::set_value(key);
}

variant graphical_font_label::get_value(const std::string& key) const
{
	if(key == "text") {
		return variant(text_);
	} else if(key == "font") {
		return variant(font_->id());
	} else if(key == "size") {
		return variant(size_);
	}
	return widget::get_value(key);
}

}
