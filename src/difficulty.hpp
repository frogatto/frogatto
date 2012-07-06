#pragma once
#ifndef DIFFICULTY_HPP_INCLUDED
#define DIFFICULTY_HPP_INCLUDED

#include "variant.hpp"

namespace difficulty {

struct manager {
	manager();
	~manager();
};

std::string to_string(int diff);
int from_string(const std::string& s);
int from_variant(variant node);

}

#endif // DIFFICULTY_HPP_INCLUDED