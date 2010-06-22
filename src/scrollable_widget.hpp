#ifndef SCROLLABLE_WIDGET_HPP_INCLUDED
#define SCROLLABLE_WIDGET_HPP_INCLUDED

#include "widget.hpp"

namespace gui {

class scrollable_widget : public widget
{
public:
	virtual int scroll_max() const;
	virtual int scroll_window_size() const;
};

}

#endif
