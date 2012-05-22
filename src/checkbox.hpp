#ifndef CHECKBOX_HPP_INCLUDED
#define CHECKBOX_HPP_INCLUDED

#include "button.hpp"

#include <string>

namespace gui {

class checkbox : public button
{
public:
	checkbox(const std::string& label, bool checked, boost::function<void(bool)> onclick, BUTTON_RESOLUTION button_resolution=BUTTON_SIZE_NORMAL_RESOLUTION);
private:
	void on_click();

	std::string label_;
	boost::function<void(bool)> onclick_;
	bool checked_;
};

typedef boost::intrusive_ptr<checkbox> checkbox_ptr;

}

#endif
