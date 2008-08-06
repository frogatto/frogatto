#ifndef WML_PREPROCESSOR_HPP_INCLUDED
#define WML_PREPROCESSOR_HPP_INCLUDED

#include <map>
#include <string>

namespace wml {
typedef std::map<std::string, std::string> symbols_map;
struct preprocessor_error {};
void preprocess(const std::string& filename, std::string* output,
				symbols_map* symbols=NULL);
}

#endif
