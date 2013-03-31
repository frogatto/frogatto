/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <boost/bind.hpp>
#include <map>
#include <string>
#include <stdio.h>

#include "filesystem.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_callable_definition.hpp"
#include "formula_object.hpp"
#include "json_parser.hpp"
#include "module.hpp"
#include "preferences.hpp"
#include "string_utils.hpp"
#include "variant_type.hpp"
#include "variant_utils.hpp"


namespace game_logic
{

namespace {
void invalidate_class_definition(const std::string& class_name);

boost::intrusive_ptr<const formula_class> get_class(const std::string& type);

struct property_entry {
	property_entry() {
	}
	property_entry(const std::string& class_name, const std::string& prop_name, variant node) {
		name = prop_name;

		formula_callable_definition* class_def = get_class_definition(class_name);

		const formula::strict_check_scope strict_checking;
		if(node.is_string()) {
			getter = game_logic::formula::create_optional_formula(node, NULL, get_class_definition(class_name));
			ASSERT_LOG(getter, "COULD NOT PARSE CLASS FORMULA " << class_name << "." << prop_name);

			ASSERT_LOG(getter->query_variant_type()->is_any() == false, "COULD NOT INFER TYPE FOR CLASS PROPERTY " << class_name << "." << prop_name << ". SET THIS PROPERTY EXPLICITLY");

			formula_callable_definition::entry* entry = class_def->get_entry_by_id(prop_name);
			ASSERT_LOG(entry != NULL, "COULD NOT FIND CLASS PROPERTY ENTRY " << class_name << "." << prop_name);

			entry->set_variant_type(getter->query_variant_type());
			return;
		}

		variant variable_setting = node["variable"];
		if(variable_setting.is_bool() && variable_setting.as_bool()) {
			variable = variant(name);
		} else if(variable_setting.is_string()) {
			variable = variable_setting;
		}

		if(node["get"].is_string()) {
			getter = game_logic::formula::create_optional_formula(node["get"], NULL, get_class_definition(class_name));
		}

		if(node["set"].is_string()) {
			setter = game_logic::formula::create_optional_formula(node["set"], NULL, get_class_definition(class_name));
		}

#ifndef NO_FFL_TYPE_SAFETY_CHECKS
		if(preferences::type_safety_checks()) {
			variant valid_types = node["type"];
			if(valid_types.is_null() && variable_setting.is_bool() && variable_setting.as_bool()) {
				variant default_value = node["default"];
				if(default_value.is_null() == false) {
					valid_types = variant(variant::variant_type_to_string(default_value.type()));
				}
			}

			if(valid_types.is_null() == false) {
				get_type = parse_variant_type(valid_types);
				set_type = get_type;
			}
			valid_types = node["set_type"];
			if(valid_types.is_null() == false) {
				set_type = parse_variant_type(valid_types);
			}
		}
#endif
	}

	std::string name;
	variant variable;
	game_logic::const_formula_ptr getter, setter;

	variant_type_ptr get_type, set_type;
};

std::map<std::string, variant> class_node_map;

void load_class_node(const std::string& type, const variant& node)
{
	class_node_map[type] = node;
	
	const variant classes = node["classes"];
	if(classes.is_map()) {
		foreach(variant key, classes.get_keys().as_list()) {
			load_class_node(type + "." + key.as_string(), classes[key]);
		}
	}
}

void load_class_nodes(const std::string& type)
{
	const std::string path = "data/classes/" + type + ".cfg";
	const std::string real_path = module::map_file(path);

	sys::notify_on_file_modification(real_path, boost::bind(invalidate_class_definition, type));

	const variant v = json::parse_from_file(path);
	ASSERT_LOG(v.is_map(), "COULD NOT FIND FFL CLASS: " << type);

	load_class_node(type, v);
}

variant get_class_node(const std::string& type)
{
	std::map<std::string, variant>::const_iterator i = class_node_map.find(type);
	if(i != class_node_map.end()) {
		return i->second;
	}

	if(std::find(type.begin(), type.end(), '.') != type.end()) {
		std::vector<std::string> v = util::split(type, '.');
		load_class_nodes(v.front());
	} else {
		load_class_nodes(type);
	}

	i = class_node_map.find(type);
	ASSERT_LOG(i != class_node_map.end(), "COULD NOT FIND CLASS: " << type);
	return i->second;
}

enum CLASS_BASE_FIELDS { FIELD_PRIVATE, FIELD_VALUE, FIELD_SELF, FIELD_ME, FIELD_CLASS, NUM_BASE_FIELDS };
static const std::string BaseFields[] = {"private", "value", "self", "me", "_class"};

class formula_class_definition : public formula_callable_definition
{
public:
	formula_class_definition(const std::string& class_name, const variant& var)
	  : type_name_("class " + class_name)
	{
		for(int n = 0; n != NUM_BASE_FIELDS; ++n) {
			properties_[BaseFields[n]] = n;
			slots_.push_back(entry(BaseFields[n]));
			switch(n) {
			case FIELD_PRIVATE:
			slots_.back().variant_type = variant_type::get_type(variant::VARIANT_TYPE_MAP);
			break;
			case FIELD_VALUE:
			slots_.back().variant_type = variant_type::get_any();
			break;
			case FIELD_SELF:
			case FIELD_ME:
			slots_.back().variant_type = variant_type::get_class(class_name);
			break;
			case FIELD_CLASS:
			slots_.back().variant_type = variant_type::get_type(variant::VARIANT_TYPE_STRING);
			break;
			}
		}

		std::vector<variant> nodes;
		nodes.push_back(var);
		while(nodes.back()["bases"].is_list() && nodes.back()["bases"].num_elements() > 0) {
			variant nodes_v = nodes.back()["bases"];
			ASSERT_LOG(nodes_v.num_elements() == 1, "MULTIPLE INHERITANCE NOT YET SUPPORTED");

			variant new_node = get_class_node(nodes_v[0].as_string());
			ASSERT_LOG(std::count(nodes.begin(), nodes.end(), new_node) == 0, "RECURSIVE INHERITANCE DETECTED");

			nodes.push_back(new_node);
		}

		std::reverse(nodes.begin(), nodes.end());

		foreach(const variant& node, nodes) {
			const variant properties = node["properties"];
			if(!properties.is_map()) {
				continue;
			}

			foreach(variant key, properties.get_keys().as_list()) {
				if(properties_.count(key.as_string()) == 0) {
					properties_[key.as_string()] = slots_.size();
					slots_.push_back(entry(key.as_string()));
				}

				const int slot = properties_[key.as_string()];

				variant prop_node = properties[key];
				if(prop_node.is_map()) {
					variant valid_types = prop_node["type"];
					if(valid_types.is_null() && prop_node["variable"].is_bool() && prop_node["variable"].as_bool()) {
						variant default_value = prop_node["default"];
						if(default_value.is_null() == false) {
							valid_types = variant(variant::variant_type_to_string(default_value.type()));
						}
					}

					if(valid_types.is_null() == false) {
						slots_[slot].variant_type = parse_variant_type(valid_types);
					}
				} else if(prop_node.is_string()) {
					variant_type_ptr fn_type = parse_optional_function_type(prop_node);
					if(fn_type) {
						slots_[slot].variant_type = fn_type;
					}
				}
			}
		}
	}

	virtual ~formula_class_definition() {}

	void init() {
		foreach(entry& e, slots_) {
			std::string class_name;
			if(e.variant_type && e.variant_type->is_class(&class_name)) {
				e.type_definition = get_class_definition(class_name);
			}
		}
	}

	virtual int get_slot(const std::string& key) const {
		std::map<std::string, int>::const_iterator itor = properties_.find(key);
		if(itor != properties_.end()) {
			return itor->second;
		}
		
		return -1;
	}

	virtual entry* get_entry(int slot) {
		if(slot < 0 || slot >= slots_.size()) {
			return NULL;
		}

		return &slots_[slot];
	}

	virtual const entry* get_entry(int slot) const {
		if(slot < 0 || slot >= slots_.size()) {
			return NULL;
		}

		return &slots_[slot];
	}
	virtual int num_slots() const {
		return slots_.size();
	}

	const std::string* type_name() const {
		return &type_name_;
	}

private:
	std::map<std::string, int> properties_;
	std::vector<entry> slots_;
	std::string type_name_;
};

typedef std::map<std::string, formula_class_definition*> class_definition_map;
class_definition_map class_definitions;

typedef std::map<std::string, boost::intrusive_ptr<formula_class> > classes_map;

bool in_unit_test = false;
std::vector<formula_class*> unit_test_queue;

}

formula_callable_definition* get_class_definition(const std::string& name)
{
	class_definition_map::iterator itor = class_definitions.find(name);
	if(itor != class_definitions.end()) {
		return itor->second;
	}

	formula_class_definition* def = new formula_class_definition(name, get_class_node(name));
	class_definitions[name] = def;
	def->init();

	return def;
}

class formula_class : public reference_counted_object
{
public:
	formula_class(const std::string& class_name, const variant& node);
	void set_name(const std::string& name);
	const std::string& name() const { return name_; }
	const variant& name_variant() const { return name_variant_; }
	const variant& private_data() const { return private_data_; }
	const std::vector<game_logic::const_formula_ptr>& constructor() const { return constructor_; }
	const std::map<std::string, int>& properties() const { return properties_; }
	const std::vector<property_entry>& slots() const { return slots_; }
	const classes_map& sub_classes() const { return sub_classes_; }

	bool is_a(const std::string& name) const;

	void build_nested_classes();
	void run_unit_tests();

private:
	std::string name_;
	variant name_variant_;
	variant private_data_;
	std::vector<game_logic::const_formula_ptr> constructor_;
	std::map<std::string, int> properties_;

	std::vector<property_entry> slots_;
	
	classes_map sub_classes_;

	variant unit_test_;

	std::vector<boost::intrusive_ptr<const formula_class> > bases_;

	variant nested_classes_;
};

bool is_class_derived_from(const std::string& derived, const std::string& base)
{
	return get_class(derived)->is_a(base);
}

formula_class::formula_class(const std::string& class_name, const variant& node)
  : name_(class_name)
{
	variant bases_v = node["bases"];
	if(bases_v.is_null() == false) {
		for(int n = 0; n != bases_v.num_elements(); ++n) {
			bases_.push_back(get_class(bases_v[n].as_string()));
		}
	}

	std::map<variant, variant> m;
	private_data_ = variant(&m);

	foreach(boost::intrusive_ptr<const formula_class> base, bases_) {
		merge_variant_over(&private_data_, base->private_data_);
	}

	if(node["private"].is_map()) {
		merge_variant_over(&private_data_, node["private"]);
	}

	ASSERT_LOG(bases_.size() <= 1, "Multiple inheritance of classes not currently supported");

	foreach(boost::intrusive_ptr<const formula_class> base, bases_) {
		slots_ = base->slots();
		properties_ = base->properties();
	}

	const variant properties = node["properties"];
	if(properties.is_map()) {
		foreach(variant key, properties.get_keys().as_list()) {
			const variant prop_node = properties[key];
			property_entry entry(class_name, key.as_string(), prop_node);

			if(properties_.count(key.as_string()) == 0) {
				properties_[key.as_string()] = slots_.size();
				slots_.push_back(property_entry());
			}

			slots_[properties_[key.as_string()]] = entry;

			if(prop_node.has_key("default") && entry.variable.is_string()) {
				private_data_.add_attr(entry.variable, prop_node["default"]);
			}
		}
	}

	nested_classes_ = node["classes"];

	if(node["constructor"].is_string()) {
		const formula::strict_check_scope strict_checking;

		formula_callable_definition* class_def = get_class_definition(class_name);
		constructor_.push_back(game_logic::formula::create_optional_formula(node["constructor"], NULL, class_def));
	}

	unit_test_ = node["test"];
}

void formula_class::build_nested_classes()
{
	if(nested_classes_.is_map()) {
		foreach(variant key, nested_classes_.get_keys().as_list()) {
			const variant class_node = nested_classes_[key];
			sub_classes_[key.as_string()].reset(new formula_class(name_ + "." + key.as_string(), class_node));
		}

		nested_classes_ = variant();
	}
}

void formula_class::set_name(const std::string& name)
{
	name_ = name;
	name_variant_ = variant(name);
	for(classes_map::iterator i = sub_classes_.begin(); i != sub_classes_.end(); ++i) {
		i->second->set_name(name + "." + i->first);
	}
}
bool formula_class::is_a(const std::string& name) const
{
	if(name == name_) {
		return true;
	}

	typedef boost::intrusive_ptr<const formula_class> Ptr;
	foreach(const Ptr& base, bases_) {
		if(base->is_a(name)) {
			return true;
		}
	}

	return false;
}

void formula_class::run_unit_tests()
{
	if(unit_test_.is_null()) {
		return;
	}

	if(in_unit_test) {
		unit_test_queue.push_back(this);
		return;
	}

	variant unit_test = unit_test_;
	unit_test_ = variant();

	in_unit_test = true;

	boost::intrusive_ptr<game_logic::map_formula_callable> callable(new game_logic::map_formula_callable);
	std::map<variant,variant> attr;
	callable->add("vars", variant(&attr));

	for(int n = 0; n != unit_test.num_elements(); ++n) {
		variant test = unit_test[n];
		game_logic::formula_ptr cmd = game_logic::formula::create_optional_formula(test["command"]);
		if(cmd) {
			variant v = cmd->execute(*callable);
			callable->execute_command(v);
		}

		game_logic::formula_ptr predicate = game_logic::formula::create_optional_formula(test["assert"]);
		if(predicate) {
			game_logic::formula_ptr message = game_logic::formula::create_optional_formula(test["message"]);

			std::string msg;
			if(message) {
				msg += ": " + message->execute(*callable).write_json();
			}

			ASSERT_LOG(predicate->execute(*callable).as_bool(), "UNIT TEST FAILURE FOR CLASS " << name_ << " TEST " << n << " FAILED: " << test["assert"].write_json() << msg << "\n");
		}

	}

	in_unit_test = false;

	for(classes_map::iterator i = sub_classes_.begin(); i != sub_classes_.end(); ++i) {
		i->second->run_unit_tests();
	}

	if(unit_test_queue.empty() == false) {
		formula_class* c = unit_test_queue.back();
		unit_test_queue.pop_back();
		c->run_unit_tests();
	}
}

namespace
{
struct private_data_scope {
	explicit private_data_scope(int& r, variant* tmp_value=NULL, const variant* value=NULL) : r_(r), tmp_value_(tmp_value) {
		++r_;

		if(tmp_value) {
			*tmp_value = *value;
		}
	}

	~private_data_scope() {
		--r_;

		if(tmp_value_) {
			*tmp_value_ = variant();
		}
	}
private:
	int& r_;
	variant* tmp_value_;
};

classes_map classes_, backup_classes_;
std::set<std::string> known_classes;

void record_classes(const std::string& name, const variant& node)
{
	known_classes.insert(name);

	const variant classes = node["classes"];
	if(classes.is_map()) {
		foreach(variant key, classes.get_keys().as_list()) {
			const variant class_node = classes[key];
			record_classes(name + "." + key.as_string(), class_node);
		}
	}
}

boost::intrusive_ptr<formula_class> build_class(const std::string& type)
{
	const variant v = get_class_node(type);

	record_classes(type, v);

	boost::intrusive_ptr<formula_class> result(new formula_class(type, v));
	result->set_name(type);
	return result;
}

boost::intrusive_ptr<const formula_class> get_class(const std::string& type)
{
	if(std::find(type.begin(), type.end(), '.') != type.end()) {
		std::vector<std::string> v = util::split(type, '.');
		boost::intrusive_ptr<const formula_class> c = get_class(v.front());
		for(int n = 1; n < v.size(); ++n) {
			classes_map::const_iterator itor = c->sub_classes().find(v[n]);
			ASSERT_LOG(itor != c->sub_classes().end(), "COULD NOT FIND FFL CLASS: " << type);
			c = itor->second.get();
		}

		return c;
	}

	classes_map::const_iterator itor = classes_.find(type);
	if(itor != classes_.end()) {
		return itor->second;
	}

	boost::intrusive_ptr<formula_class> result;
	
	if(!backup_classes_.empty() && backup_classes_.count(type)) {
		try {
			result = build_class(type);
		} catch(...) {
			result = backup_classes_[type];
			std::cerr << "ERROR LOADING NEW CLASS\n";
		}
	} else {
		result = build_class(type);
	}

	classes_[type] = result;
	result->build_nested_classes();
	result->run_unit_tests();
	return boost::intrusive_ptr<const formula_class>(result.get());
}

}

void formula_object::visit_variants(variant node, boost::function<void (variant)> fn, std::vector<formula_object*>* seen)
{
	std::vector<formula_object*> seen_buf;
	if(!seen) {
		seen = &seen_buf;
	}

	if(node.try_convert<formula_object>()) {
		formula_object* obj = node.try_convert<formula_object>();
		if(std::count(seen->begin(), seen->end(), obj)) {
			return;
		}

		fn(node);

		const_wml_serializable_formula_callable_ptr ptr(obj);
		wml_formula_callable_serialization_scope::register_serialized_object(ptr);
		seen->push_back(obj);

		visit_variants(obj->private_data_, fn, seen);
		seen->pop_back();
		return;
	}
	
	fn(node);

	if(node.is_list()) {
		foreach(const variant& item, node.as_list()) {
			formula_object::visit_variants(item, fn, seen);
		}
	} else if(node.is_map()) {
		foreach(const variant_pair& item, node.as_map()) {
			formula_object::visit_variants(item.second, fn, seen);
		}
	}
}

void formula_object::reload_classes()
{
	classes_.clear();
}

boost::intrusive_ptr<formula_object> formula_object::create(const std::string& type, variant args)
{
	const formula::strict_check_scope strict_checking;
	boost::intrusive_ptr<formula_object> res(new formula_object(type, args));
	res->call_constructors(args);
	res->validate();
	return res;
}

formula_object::formula_object(const std::string& type, variant args)
  : class_(get_class(type)), expose_private_data_(false)
{
	private_data_ = deep_copy_variant(class_->private_data());
}

bool formula_object::is_a(const std::string& class_name) const
{
	return class_->is_a(class_name);
}

const std::string& formula_object::get_class_name() const
{
	return class_->name();
}

void formula_object::call_constructors(variant args)
{
	if(args.is_map()) {
		const formula_callable_definition* def = get_class_definition(class_->name());
		foreach(const variant& key, args.get_keys().as_list()) {
			std::map<std::string, int>::const_iterator itor = class_->properties().find(key.as_string());
			if(itor != class_->properties().end() && class_->slots()[itor->second].setter.get() == NULL && class_->slots()[itor->second].variable.is_null()) {
				if(property_overrides_.size() <= itor->second) {
					property_overrides_.resize(itor->second+1);
				}

				//A read-only property. Set the formula to what is passed in.
				formula_ptr f(new formula(args[key], NULL, def));
				const formula_callable_definition::entry* entry = def->get_entry_by_id(key.as_string());
				ASSERT_LOG(entry, "COULD NOT FIND ENTRY IN CLASS DEFINITION: " << key.as_string());
				if(entry->variant_type) {
					ASSERT_LOG(variant_types_compatible(entry->variant_type, f->query_variant_type()), "ERROR: property override in instance of class " << class_->name() << " has mis-matched type for property " << key.as_string() << ": " << entry->variant_type->to_string() << " doesn't match " << f->query_variant_type()->to_string() << " at " << args[key].debug_location());
				}
				property_overrides_[itor->second] = f;
			} else {
				set_value(key.as_string(), args[key]);
			}
		}
	}

	foreach(const game_logic::const_formula_ptr f, class_->constructor()) {
		private_data_scope scope(expose_private_data_, &tmp_value_, &args);
		execute_command(f->execute(*this));
	}
}

formula_object::formula_object(variant data)
  : class_(get_class(data["@class"].as_string())), expose_private_data_(false)
{
	if(data.is_map() && data["private"].is_map()) {
		private_data_ = deep_copy_variant(data["private"]);
	} else {
		private_data_ = deep_copy_variant(class_->private_data());
	}

	if(data.is_map() && data["property_overrides"].is_list()) {
		const variant overrides = data["property_overrides"];
		property_overrides_.reserve(overrides.num_elements());
		for(int n = 0; n != overrides.num_elements(); ++n) {
			if(overrides[n].is_null()) {
				property_overrides_.push_back(formula_ptr());
			} else {
				property_overrides_.push_back(formula_ptr(new formula(overrides[n])));
			}
		}
	}

	set_addr(data["_addr"].as_string());
}

formula_object::~formula_object()
{}

boost::intrusive_ptr<formula_object> formula_object::clone() const
{
	return boost::intrusive_ptr<formula_object>(new formula_object(*this));
}

variant formula_object::serialize_to_wml() const
{
	std::map<variant, variant> result;
	result[variant("@class")] = variant(class_->name());
	result[variant("private")] = deep_copy_variant(private_data_);

	if(property_overrides_.empty() == false) {
		std::vector<variant> properties;
		foreach(const formula_ptr& f, property_overrides_) {
			if(f) {
				properties.push_back(variant(f->str()));
			} else {
				properties.push_back(variant());
			}
		}
		result[variant("property_overrides")] = variant(&properties);
	}

	char addr_buf[256];
	sprintf(addr_buf, "%p", this);
	result[variant("_addr")] = variant(addr_buf);

	return variant(&result);
}

variant formula_object::get_value(const std::string& key) const
{
	/*TODO: MAKE DATA HIDING WORK
	if(expose_private_data_) */ {
		if(key == "private") {
			return private_data_;
		} else if(key == "value") {
			return tmp_value_;
		}
	}

	if(key == "self" || key == "me") {
		return variant(this);
	}

	if(key == "_class") {
		return class_->name_variant();
	}

	std::map<std::string, int>::const_iterator itor = class_->properties().find(key);
	ASSERT_LOG(itor != class_->properties().end(), "UNKNOWN PROPERTY ACCESS " << key << " IN CLASS " << class_->name() << "\nFORMULA LOCATION: " << get_call_stack());

	if(itor->second < property_overrides_.size() && property_overrides_[itor->second]) {
		private_data_scope scope(expose_private_data_);
		return property_overrides_[itor->second]->execute(*this);
	}

	const property_entry& entry = class_->slots()[itor->second];

	if(entry.getter) {
		private_data_scope scope(expose_private_data_);
		return entry.getter->execute(*this);
	} else if(entry.variable.is_null() == false) {
		return private_data_[entry.variable];
	} else {
		ASSERT_LOG(false, "ILLEGAL READ PROPERTY ACCESS OF NON-READABLE VARIABLE " << key << " IN CLASS " << class_->name());
	}
}

variant formula_object::get_value_by_slot(int slot) const
{
	switch(slot) {
		case FIELD_PRIVATE: return private_data_;
		case FIELD_VALUE: return tmp_value_;
		case FIELD_SELF:
		case FIELD_ME: return variant(this);
		case FIELD_CLASS: return class_->name_variant();
		default: break;
	}

	slot -= NUM_BASE_FIELDS;

	ASSERT_LOG(slot >= 0 && slot < class_->slots().size(), "ILLEGAL VALUE QUERY TO FORMULA OBJECT: " << slot << " IN " << class_->name());


	if(slot < property_overrides_.size() && property_overrides_[slot]) {
		private_data_scope scope(expose_private_data_);
		return property_overrides_[slot]->execute(*this);
	}
	
	const property_entry& entry = class_->slots()[slot];

	if(entry.getter) {
		private_data_scope scope(expose_private_data_);
		return entry.getter->execute(*this);
	} else if(entry.variable.is_null() == false) {
		return private_data_[entry.variable];
	} else {
		ASSERT_LOG(false, "ILLEGAL READ PROPERTY ACCESS OF NON-READABLE VARIABLE IN CLASS " << class_->name());
	}
}

void formula_object::set_value(const std::string& key, const variant& value)
{
	if(expose_private_data_ && key == "private") {
		if(value.is_map() == false) {
			ASSERT_LOG(false, "TRIED TO SET CLASS PRIVATE DATA TO A VALUE WHICH IS NOT A MAP: " << value);
		}
		private_data_ = value;
		return;
	}

	std::map<std::string, int>::const_iterator itor = class_->properties().find(key);
	ASSERT_LOG(itor != class_->properties().end(), "UNKNOWN PROPERTY ACCESS " << key << " IN CLASS " << class_->name());

	set_value_by_slot(itor->second+NUM_BASE_FIELDS, value);
	return;
}

void formula_object::set_value_by_slot(int slot, const variant& value)
{
	if(slot < NUM_BASE_FIELDS) {
		switch(slot) {
		case FIELD_PRIVATE:
			if(value.is_map() == false) {
				ASSERT_LOG(false, "TRIED TO SET CLASS PRIVATE DATA TO A VALUE WHICH IS NOT A MAP: " << value);
			}
			private_data_ = value;
			return;
		default:
			ASSERT_LOG(false, "TRIED TO SET ILLEGAL KEY IN CLASS: " << BaseFields[slot]);
		}
	}

	slot -= NUM_BASE_FIELDS;
	ASSERT_LOG(slot >= 0 && slot < class_->slots().size(), "ILLEGAL VALUE SET TO FORMULA OBJECT: " << slot << " IN " << class_->name());

	const property_entry& entry = class_->slots()[slot];

	if(entry.set_type) {
		if(!entry.set_type->match(value)) {
			ASSERT_LOG(false, "ILLEGAL WRITE PROPERTY ACCESS: SETTING VARIABLE " << entry.name << " IN CLASS " << class_->name() << " TO INVALID TYPE " << variant::variant_type_to_string(value.type()) << ": " << value.write_json());
		}
	}

	if(entry.setter) {
		private_data_scope scope(expose_private_data_, &tmp_value_, &value);
		execute_command(entry.setter->execute(*this));
	} else if(entry.variable.is_null() == false) {
		private_data_.add_attr_mutation(entry.variable, value);
	} else {
		ASSERT_LOG(false, "ILLEGAL WRITE PROPERTY ACCESS OF NON-WRITABLE VARIABLE " << entry.name << " IN CLASS " << class_->name());
	}

	if(entry.get_type && (entry.getter || entry.setter)) {
		//now that we've set the value, retrieve it and ensure it matches
		//the type we expect.
		variant var;

		formula_ptr override;
		if(slot < property_overrides_.size()) {
			override = property_overrides_[slot];
		}
		if(override) {
			private_data_scope scope(expose_private_data_);
			var = override->execute(*this);
		} else if(entry.getter) {
			private_data_scope scope(expose_private_data_);
			var = entry.getter->execute(*this);
		} else {
			var = private_data_[entry.variable];
		}

		ASSERT_LOG(entry.get_type->match(var), "AFTER WRITE TO " << entry.name << " IN CLASS " << class_->name() << " TYPE IS INVALID. EXPECTED " << entry.get_type->str() << " BUT FOUND " << var.write_json());
	}
}

void formula_object::validate() const
{
#ifndef NO_FFL_TYPE_SAFETY_CHECKS
	if(preferences::type_safety_checks() == false) {
		return;
	}

	int index = 0;
	foreach(const property_entry& entry, class_->slots()) {
		if(!entry.get_type) {
			++index;
			continue;
		}

		variant value;

		formula_ptr override;
		if(index < property_overrides_.size()) {
			override = property_overrides_[index];
		}
		if(override) {
			private_data_scope scope(expose_private_data_);
			value = override->execute(*this);
		} else if(entry.getter) {
			private_data_scope scope(expose_private_data_);
			value = entry.getter->execute(*this);
		} else if(entry.variable.is_null() == false) {
			value = private_data_[entry.variable];
		} else {
			++index;
			continue;
		}

		++index;

		ASSERT_LOG(entry.get_type->match(value), "OBJECT OF CLASS TYPE " << class_->name() << " HAS INVALID PROPERTY " << entry.name << ": " << value.write_json() << " EXPECTED " << entry.get_type->str());
	}
#endif
}

void formula_object::get_inputs(std::vector<formula_input>* inputs) const
{
	foreach(const property_entry& entry, class_->slots()) {
		FORMULA_ACCESS_TYPE type = FORMULA_READ_ONLY;
		if(entry.getter && entry.setter || entry.variable.is_null() == false) {
			type = FORMULA_READ_WRITE;
		} else if(entry.getter) {
			type = FORMULA_READ_ONLY;
		} else if(entry.setter) {
			type = FORMULA_WRITE_ONLY;
		} else {
			continue;
		}

		inputs->push_back(formula_input(entry.name, type));
	}
}

bool formula_class_valid(const std::string& type)
{
	return known_classes.count(type) != false || get_class_node(type).is_map();
}

namespace {

void invalidate_class_definition(const std::string& name)
{
	std::cerr << "INVALIDATE CLASS: " << name << "\n";
	for(std::map<std::string, variant>::iterator i = class_node_map.begin(); i != class_node_map.end(); ) {
		const std::string& class_name = i->first;
		std::string::const_iterator dot = std::find(class_name.begin(), class_name.end(), '.');
		std::string base_class(class_name.begin(), dot);
		if(base_class == name) {
			class_node_map.erase(i++);
		} else {
			++i;
		}
	}

	for(class_definition_map::iterator i = class_definitions.begin(); i != class_definitions.end(); )
	{
		const std::string& class_name = i->first;
		std::string::const_iterator dot = std::find(class_name.begin(), class_name.end(), '.');
		std::string base_class(class_name.begin(), dot);
		if(base_class == name) {
			class_definitions.erase(i++);
		} else {
			++i;
		}
	}

	for(classes_map::iterator i = classes_.begin(); i != classes_.end(); )
	{
		const std::string& class_name = i->first;
		std::string::const_iterator dot = std::find(class_name.begin(), class_name.end(), '.');
		std::string base_class(class_name.begin(), dot);
		if(base_class == name) {
			known_classes.erase(class_name);
			backup_classes_[i->first] = i->second;
			classes_.erase(i++);
		} else {
			++i;
		}
	}
}

}

}

