#ifndef FORMULA_CALLABLE_DEFINITION_HPP_INCLUDED
#define FORMULA_CALLABLE_DEFINITION_HPP_INCLUDED

#include <string>

#include "formula_callable_definition_fwd.hpp"

namespace game_logic
{

class formula_callable_definition
{
public:
	struct entry {
		explicit entry(const std::string& id_) : id(id_), type_definition(0) {}
		std::string id;
		const formula_callable_definition* type_definition;

		//optionally store the reference to the type definition.
		const_formula_callable_definition_ptr type_definition_holder;
	};

	virtual ~formula_callable_definition() {}

	virtual int get_slot(const std::string& key) const = 0;
	virtual entry* get_entry(int slot) { return NULL; }
	virtual const entry* get_entry(int slot) const = 0;
	virtual int num_slots() const = 0;
};

formula_callable_definition_ptr create_formula_callable_definition(const std::string* beg, const std::string* end, const formula_callable_definition* base=NULL);

int register_formula_callable_definition(const std::string& id, const formula_callable_definition* def);
const formula_callable_definition* get_formula_callable_definition(const std::string& id);

}

#endif
