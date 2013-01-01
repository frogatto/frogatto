#pragma once
#ifndef BASE64_HPP_INCLUDED
#define BASE64_HPP_INCLUDED

#include <string>
#include <vector>

namespace base64 {

std::string b64encode(const std::string& data, int output_line_length=64);
std::vector<char> b64encode(const std::vector<char>& data, int output_line_length=64);
std::string b64decode(const std::string& data);
std::vector<char> b64decode(const std::vector<char>& data);

}

#endif // BASE64_HPP_INCLUDED
