#include "SDL.h"

#include <iostream>
#include <vector>

#include "foreach.hpp"
#include "joystick.hpp"

namespace joystick {

namespace {
std::vector<SDL_Joystick*> joysticks;

const int threshold = 20000;
}

manager::manager() {
	for(int n = 0; n != SDL_NumJoysticks(); ++n) {
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
}

void update() {
	SDL_JoystickUpdate();
}

bool up() {
	foreach(SDL_Joystick* j, joysticks) {
		if(SDL_JoystickGetAxis(j, 1) < -threshold ||
		   SDL_JoystickGetAxis(j, 3) < -threshold ||
		   SDL_JoystickGetAxis(j, 5) < -threshold) {
			return true;
		}
	}

	return false;
}

bool down() {
	foreach(SDL_Joystick* j, joysticks) {
		if(SDL_JoystickGetAxis(j, 1) > threshold ||
		   SDL_JoystickGetAxis(j, 3) > threshold ||
		   SDL_JoystickGetAxis(j, 5) > threshold) {
			return true;
		}
	}

	return false;
}

bool left() {
	foreach(SDL_Joystick* j, joysticks) {
		if(SDL_JoystickGetAxis(j, 0) < -threshold ||
		   SDL_JoystickGetAxis(j, 2) < -threshold ||
		   SDL_JoystickGetAxis(j, 4) < -threshold) {
			return true;
		}
	}

	return false;
}

bool right() {
	foreach(SDL_Joystick* j, joysticks) {
		if(SDL_JoystickGetAxis(j, 0) > threshold ||
		   SDL_JoystickGetAxis(j, 2) > threshold ||
		   SDL_JoystickGetAxis(j, 4) > threshold) {
			return true;
		}
	}

	return false;
}

bool button(int n) {
	foreach(SDL_Joystick* j, joysticks) {
		if(SDL_JoystickGetButton(j, n)) {
			return true;
		}
	}

	return false;
}

}
