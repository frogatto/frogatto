#include "graphics.hpp"

#include <iostream>
#include <vector>

#include "foreach.hpp"
#include "joystick.hpp"
#include "preferences.hpp"

#if defined(TARGET_BLACKBERRY)
#include <bps/accelerometer.h>
#include <bps/sensor.h>
#include <bps/bps.h>
#endif

#include "asserts.hpp"

namespace joystick {

namespace {
std::vector<SDL_Joystick*> joysticks;

const int threshold = 32700;
}

manager::manager() {
#if defined(__ANDROID__)
    // We're just going to open 1 joystick on android platform.
	int n = 0; {
#else
	for(int n = 0; n != SDL_NumJoysticks(); ++n) {
#endif
		SDL_Joystick* j = SDL_JoystickOpen(n);
		if(j) {
			joysticks.push_back(j);
		}
	}

	std::cerr << "initialized " << joysticks.size() << " joysticks\n";
}

manager::~manager() {
	foreach(SDL_Joystick* j, joysticks) {
		SDL_JoystickClose(j);
	}
	joysticks.clear();

#if defined(TARGET_BLACKBERRY)
	bps_shutdown();
#endif
}

void update() {
	if(preferences::use_joystick()) {
		SDL_JoystickUpdate();
	}
}

bool up() {
	if(!preferences::use_joystick()) {
		return false;
	}

	foreach(SDL_Joystick* j, joysticks) {
		Sint16  y = SDL_JoystickGetAxis(j, 1);
		if (y != 0 && y < 0) {
			return true;
		}

		const int nhats = SDL_JoystickNumHats(j);
		for(int n = 0; n != nhats; ++n) {
			const Uint8 state = SDL_JoystickGetHat(j, n);
			switch(state) {
			case SDL_HAT_UP:
			case SDL_HAT_RIGHTUP:
			case SDL_HAT_LEFTUP:
					return true;
			}
		}

	}

	return false;
}

bool down() {
	if(!preferences::use_joystick()) {
		return false;
	}

	foreach(SDL_Joystick* j, joysticks) {
		Sint16  y = SDL_JoystickGetAxis(j, 1);
		if (y != 0 && y > 0) {
			return true;
		}

		const int nhats = SDL_JoystickNumHats(j);
		for(int n = 0; n != nhats; ++n) {
			const Uint8 state = SDL_JoystickGetHat(j, n);
			switch(state) {
			case SDL_HAT_DOWN:
			case SDL_HAT_RIGHTDOWN:
			case SDL_HAT_LEFTDOWN:
					return true;
			}
		}

	}

	return false;
}

bool left() {
	if(!preferences::use_joystick()) {
		return false;
	}

	foreach(SDL_Joystick* j, joysticks) {
		Sint16  x = SDL_JoystickGetAxis(j, 0);
		if (x != 0 && x < 0) {
			return true;
		}

		const int nhats = SDL_JoystickNumHats(j);
		for(int n = 0; n != nhats; ++n) {
			const Uint8 state = SDL_JoystickGetHat(j, n);
			switch(state) {
			case SDL_HAT_LEFT:
			case SDL_HAT_LEFTDOWN:
			case SDL_HAT_LEFTUP:
					return true;
			}
		}

	}

	return false;
}

bool right() {
	if(!preferences::use_joystick()) {
		return false;
	}

	foreach(SDL_Joystick* j, joysticks) {
		Sint16  x = SDL_JoystickGetAxis(j, 0);
		if (x != 0 && x > 0) {
			return true;
		}

		const int nhats = SDL_JoystickNumHats(j);
		for(int n = 0; n != nhats; ++n) {
			const Uint8 state = SDL_JoystickGetHat(j, n);
			switch(state) {
			case SDL_HAT_RIGHT:
			case SDL_HAT_RIGHTDOWN:
			case SDL_HAT_RIGHTUP:
					return true;
			}
		}
	}

	return false;
}

bool button(int n) {
	if(!preferences::use_joystick()) {
		return false;
	}

    int cnt = 0;
	foreach(SDL_Joystick* j, joysticks) {
		if(SDL_JoystickGetButton(j, n)) {
			return true;
		}
	}

	return false;
}

int iphone_tilt() {

#if defined(TARGET_BLACKBERRY)
	double x, y, z;
	const int result = accelerometer_read_forces(&x, &y, &z);
	if(result != BPS_SUCCESS) {
		std::cerr << "READ OF ACCELEROMETER FAILED\n";
		return 0;
	} else {
		return x*1000;
	}
#endif

//#if TARGET_OS_IPHONE
//	return SDL_JoystickGetAxis(joysticks.front(), 1);
//#else
	return 0;
//#endif
}

std::vector<int> get_info() {
	std::vector<int> res;
	res.push_back(joysticks.size());
	foreach(SDL_Joystick* j, joysticks) {
		res.push_back(SDL_JoystickGetAxis(j, 0));
		res.push_back(SDL_JoystickGetAxis(j, 1));
	}
	
	return res;
}

}
