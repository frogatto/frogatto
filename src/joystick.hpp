#ifndef JOYSTICK_HPP_INCLUDED
#define JOYSTICK_HPP_INCLUDED

#include <vector>

namespace joystick {

struct manager {
	manager();
	~manager();
};

void update();

bool up();
bool down();
bool left();
bool right();

bool button(int n);

int iphone_tilt();

std::vector<int> get_info();
}

#endif
