#ifndef PREPROCESSOR_HPP_INCLUDED
#define PREPROCESSOR_HPP_INCLUDED

#include <string>

#include "variant.hpp"

namespace game_logic {
	class formula_callable;
}

std::string preprocess(const std::string& input);

struct preprocessor_error {};
variant preprocess_string_value(const std::string& input, const game_logic::formula_callable* callable=NULL);


#endif
