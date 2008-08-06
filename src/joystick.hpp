#ifndef JOYSTICK_HPP_INCLUDED
#define JOYSTICK_HPP_INCLUDED

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
}

#endif
