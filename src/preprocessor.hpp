#ifndef PREPROCESSOR_HPP_INCLUDED
#define PREPROCESSOR_HPP_INCLUDED

#include <string>

#include "variant.hpp"

std::string preprocess(const std::string& input);

struct preprocessor_error {};
variant preprocess_string_value(const std::string& input);


#endif
