#include "foreach.hpp"
#include "formula_function_registry.hpp"
#include "unit_test.hpp"

namespace {
std::map<std::string, std::map<std::string, function_creator*> >& function_creators()
{
	static std::map<std::string, std::map<std::string, function_creator*> > instance;
	return instance;
}

std::map<std::string, std::vector<std::string> >& helpstrings()
{
	static std::map<std::string, std::vector<std::string> > instance;
	return instance;
}
}

const std::map<std::string, function_creator*>& get_function_creators(const std::string& module)
{
	return function_creators()[module];
}

int register_function_creator(const std::string& module, const std::string& id, function_creator* creator)
{
	function_creators()[module][id] = creator;
	return function_creators()[module].size();
}

const std::vector<std::string>& function_helpstrings(const std::string& module)
{
	return helpstrings()[module];
}

int register_function_helpstring(const std::string& module, const std::string& str)
{
	helpstrings()[module].push_back(str);
	return helpstrings()[module].size();
}

COMMAND_LINE_UTILITY(document_ffl_functions)
{
	for(std::map<std::string, std::vector<std::string> >::const_iterator i = helpstrings().begin(); i != helpstrings().end(); ++i) {
		if(i->second.empty()) {
			continue;
		}

		std::cout << "-- MODULE: " << i->first << " --\n";
		std::vector<std::string> helpstrings = function_helpstrings(i->first);
		std::sort(helpstrings.begin(), helpstrings.end());
		foreach(std::string s, helpstrings) {
			std::string::iterator i = std::find(s.begin(), s.end(), ':');
			if(i != s.end()) {
				s = "{{{ " + std::string(s.begin(), i) + " }}}" + std::string(i, s.end());
			}
			s = "  * " + s;
			std::cout << s << "\n";
		}

		std::cout << "\n";
	}
}
