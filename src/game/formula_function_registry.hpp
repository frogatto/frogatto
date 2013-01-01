#ifndef FORMULA_FUNCTION_REGISTRY_HPP_INCLUDED
#define FORMULA_FUNCTION_REGISTRY_HPP_INCLUDED

#include "formula_function.hpp"

#include <string>

class function_creator {
public:
	virtual ~function_creator() {}
	virtual game_logic::function_expression* create(const game_logic::function_expression::args_list& args) const = 0;
};

template<typename T>
class specific_function_creator : public function_creator {
public:
	virtual ~specific_function_creator() {}
	virtual game_logic::function_expression* create(const game_logic::function_expression::args_list& args) const {
		return new T(args);
	}
};

const std::map<std::string, function_creator*>& get_function_creators(const std::string& module);

int register_function_creator(const std::string& module, const std::string& id, function_creator* creator);

const std::vector<std::string>& function_helpstrings(const std::string& module);

int register_function_helpstring(const std::string& module, const std::string& str);

#define FUNCTION_DEF(name, min_args, max_args, helpstring) \
const int name##_dummy_help_var = register_function_helpstring(FunctionModule, helpstring); \
class name##_function : public function_expression { \
public: \
	explicit name##_function(const args_list& args) \
	  : function_expression(#name, args, min_args, max_args) {} \
private: \
	variant execute(const formula_callable& variables) const {

#define END_FUNCTION_DEF(name) } }; const int name##_dummy_var = register_function_creator(FunctionModule, #name, new specific_function_creator<name##_function>());

#define EVAL_ARG(n) (args()[n]->evaluate(variables))
#define NUM_ARGS (args().size())


#endif
