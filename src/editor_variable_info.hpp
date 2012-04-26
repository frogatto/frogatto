#ifndef EDITOR_VARIABLE_INFO_HPP_INCLUDED
#define EDITOR_VARIABLE_INFO_HPP_INCLUDED
#ifndef NO_EDITOR

#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

#include "formula_fwd.hpp"
#include "variant.hpp"

class editor_variable_info {
public:
	enum VARIABLE_TYPE { TYPE_INTEGER, XPOSITION, YPOSITION, TYPE_LEVEL, TYPE_LABEL, TYPE_TEXT, TYPE_BOOLEAN, TYPE_ENUM, TYPE_POINTS };

	explicit editor_variable_info(variant node);

	variant write() const;

	const std::string& variable_name() const { return name_; }
	VARIABLE_TYPE type() const { return type_; }
	const std::vector<std::string>& enum_values() const { return enum_values_; }
	const std::string& info() const { return info_; }
	const std::string& help() const { return help_; }

	const game_logic::const_formula_ptr& formula() const { return formula_; }

	bool numeric_decimal() const { return numeric_decimal_; }
	decimal numeric_min() const { return numeric_min_; }
	decimal numeric_max() const { return numeric_max_; }

private:
	std::string name_;
	VARIABLE_TYPE type_;
	std::vector<std::string> enum_values_;
	std::string info_;
	std::string help_;
	game_logic::const_formula_ptr formula_;

	bool numeric_decimal_;
	decimal numeric_min_, numeric_max_;
};

class editor_entity_info {
public:
	explicit editor_entity_info(variant node);

	variant write() const;

	const std::string& category() const { return category_; }
	const std::vector<editor_variable_info>& vars() const { return vars_; }
	const editor_variable_info* get_var_info(const std::string& var_name) const;
	const std::string& help() const { return help_; }
private:
	std::string category_;
	std::vector<editor_variable_info> vars_;
	std::string help_;
};

typedef boost::shared_ptr<editor_entity_info> editor_entity_info_ptr;
typedef boost::shared_ptr<const editor_entity_info> const_editor_entity_info_ptr;

#endif
#endif // !NO_EDITOR

