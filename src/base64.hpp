#pragma once
#ifndef BASE64_HPP_INCLUDED
#define BASE64_HPP_INCLUDED

#include <vector>

namespace base64 {

std::vector<char> b64encode(const std::vector<char>& data, int output_line_length=64);
std::vector<char> b64decode(const std::vector<char>& data);

}

#endif // BASE64_HPP_INCLUDED
