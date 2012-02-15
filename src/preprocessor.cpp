#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "preprocessor.hpp"
#include "filesystem.hpp"
#include "string_utils.hpp"

std::string preprocess(const std::string& input){
	std::string output_string;
	bool in_comment = false;
	
	std::string::const_iterator i = input.begin();
	
	while(i != input.end()){
		if(*i == '#'){//enter a single-line comment
			in_comment = true;}
		if(*i == '\n' and in_comment){
			in_comment = false;}
		if(*i == '@' and not in_comment){
			// process pre-processing directive here. See what comes after the '@' and do something appropriate
			static const std::string IncludeString = "@include";
			if(input.end() - i > IncludeString.size() && std::equal(IncludeString.begin(), IncludeString.end(), i)) {
					std::string filename_string;

					i += IncludeString.size(); //skip past the directive - we've tested that it exists
					
					//test for an argument to @include - e.g. "filename.cfg".  First the open quote:
					std::string::const_iterator quote = std::find(i, input.end(), '"');
					if(quote == input.end()) {
						std::cerr << "We didn't find a opening quote. Syntax error." << std::endl;
					}
					if(std::count_if(i, quote, util::isspace) != quote - i) {
					// # of whitespaces != number of intervening chars => something else was present.  Syntax Error. 
						std::cerr << "# of whitespaces != number of intervening chars." << std::endl;
					}
					i = quote + 1; //we've found a quote, advance past it
					//now the closing quote, and use it to find what's inbetween:
					std::string::const_iterator endQuote = std::find(i, input.end(), '"');
					if(endQuote == input.end()) {
						std::cerr << "We didn't find a closing quote. Syntax error." << std::endl;
					}
					
					filename_string = std::string(i, endQuote);
					
					i = endQuote + 1;
					
															
					output_string += preprocess(sys::read_file(filename_string));
			}
		} else {
			//nothing special to process, just copy the chars across
			output_string.push_back(*i);
		}
		++i;
	}

	return output_string;
}




#ifdef BUILD_PREPROCESSOR_TOOL

extern "C" int main(int argc, char** argv)
{


	for(int i = 1; i < argc; ++i) {
		std::ifstream file(argv[i], std::ios_base::binary);
		std::stringstream ss;
		ss << file.rdbuf();
		std::cout << preprocess(ss.str());
	}

}
#endif
