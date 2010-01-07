#include <map>
#include <vector>

#include "foreach.hpp"
#include "formula_callable_definition.hpp"

namespace game_logic
{

namespace
{

class simple_definition : public formula_callable_definition
{
public:
	int get_slot(const std::string& key) const {
		int index = 0;
		foreach(const entry& e, entries_) {
			if(e.id == key) {
				return  index;
			}

			++index;
		}

		return -1;
	}

	entry* get_entry(int slot) {
		if(slot < 0 || slot >= entries_.size()) {
			return NULL;
		}

		return &entries_[slot];
	}

	const entry* get_entry(int slot) const {
		if(slot < 0 || slot >= entries_.size()) {
			return NULL;
		}

		return &entries_[slot];
	}

	int num_slots() const { return entries_.size(); }

	void add(const std::string& id) {
		entries_.push_back(entry(id));
	}

private:
	std::vector<entry> entries_;
};

}

formula_callable_definition_ptr create_formula_callable_definition(const std::string* i1, const std::string* i2)
{
	simple_definition* def = new simple_definition;
	while(i1 != i2) {
		def->add(*i1);
		++i1;
	}

	return formula_callable_definition_ptr(def);
}

namespace {
std::map<std::string, const formula_callable_definition*> registry;
int num_definitions = 0;
}

int register_formula_callable_definition(const std::string& id, const formula_callable_definition* def)
{
	registry[id] = def;
	return ++num_definitions;
}

const formula_callable_definition* get_formula_callable_definition(const std::string& id)
{
	return registry[id];
}

}
