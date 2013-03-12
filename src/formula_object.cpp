#include <map>
#include <string>
#include <stdio.h>

#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_object.hpp"
#include "json_parser.hpp"
#include "module.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"

namespace game_logic
{

namespace {
boost::intrusive_ptr<const formula_class> get_class(const std::string& type);

struct property_entry {
	property_entry() {
	}
	property_entry(const std::string& name, variant node) {
		if(node.is_string()) {
			getter = game_logic::formula::create_optional_formula(node);
			return;
		}

		variant variable_setting = node["variable"];
		if(variable_setting.is_bool() && variable_setting.as_bool()) {
			variable = variant(name);
		} else if(variable_setting.is_string()) {
			variable = variable_setting;
		}

		if(node["get"].is_string()) {
			getter = game_logic::formula::create_optional_formula(node["get"]);
		}
		if(node["set"].is_string()) {
			setter = game_logic::formula::create_optional_formula(node["set"]);
		}

		variant valid_types = node["type"];
		if(valid_types.is_list()) {
			for(int n = 0; n != valid_types.num_elements(); ++n) {
				variant item = valid_types[n];
				const variant::TYPE t = variant::string_to_type(item.as_string());
				ASSERT_LOG(t != variant::VARIANT_TYPE_INVALID, "INVALID FORMULA CLASS TYPE: " << item.as_string() << " -- " << item.debug_location());

				types.push_back(t);
			}
		} else if(valid_types.is_string()) {

				const variant::TYPE t = variant::string_to_type(valid_types.as_string());
			ASSERT_LOG(t != variant::VARIANT_TYPE_INVALID, "INVALID FORMULA CLASS TYPE: " << valid_types.as_string() << " -- " << valid_types.debug_location());

				types.push_back(t);
		} else if(!valid_types.is_null()) {
			ASSERT_LOG(false, "INVALID FORMULA CLASS TYPE: " << valid_types.to_debug_string() << " -- " << valid_types.debug_location());
		}
	}

	variant variable;
	game_logic::const_formula_ptr getter, setter;
	std::vector<variant::TYPE> types;
};

typedef std::map<std::string, boost::intrusive_ptr<formula_class> > classes_map;

bool in_unit_test = false;
std::vector<formula_class*> unit_test_queue;

}

class formula_class : public reference_counted_object
{
public:
	explicit formula_class(const variant& node);
	void set_name(const std::string& name);
	const std::string& name() const { return name_; }
	const variant& name_variant() const { return name_variant_; }
	const variant& private_data() const { return private_data_; }
	const std::vector<game_logic::const_formula_ptr>& constructor() const { return constructor_; }
	const std::map<std::string, property_entry>& properties() const { return properties_; }
	const classes_map& sub_classes() const { return sub_classes_; }

	void run_unit_tests();

private:
	std::string name_;
	variant name_variant_;
	variant private_data_;
	std::vector<game_logic::const_formula_ptr> constructor_;
	std::map<std::string, property_entry> properties_;
	classes_map sub_classes_;

	variant unit_test_;
};

formula_class::formula_class(const variant& node)
{
	std::vector<boost::intrusive_ptr<const formula_class> > bases;
	variant bases_v = node["bases"];
	if(bases_v.is_null() == false) {
		for(int n = 0; n != bases_v.num_elements(); ++n) {
			bases.push_back(get_class(bases_v[n].as_string()));
		}
	}

	std::map<variant, variant> m;
	private_data_ = variant(&m);

	foreach(boost::intrusive_ptr<const formula_class> base, bases) {
		merge_variant_over(&private_data_, base->private_data_);
	}

	if(node["private"].is_map()) {
		merge_variant_over(&private_data_, node["private"]);
	}

	if(node["constructor"].is_string()) {
		constructor_.push_back(game_logic::formula::create_optional_formula(node["constructor"]));
	}

	foreach(boost::intrusive_ptr<const formula_class> base, bases) {
		for(std::map<std::string, property_entry>::const_iterator i = base->properties_.begin(); i != base->properties_.end(); ++i) {
			properties_[i->first] = i->second;
		}
	}

	const variant properties = node["properties"];
	if(properties.is_map()) {
		foreach(variant key, properties.get_keys().as_list()) {
			const variant prop_node = properties[key];
			property_entry entry(key.as_string(), prop_node);
			properties_[key.as_string()] = entry;
			if(prop_node.has_key("default") && entry.variable.is_string()) {
				private_data_.add_attr(entry.variable, prop_node["default"]);
			}
		}
	}

	const variant classes = node["classes"];
	if(classes.is_map()) {
		foreach(variant key, classes.get_keys().as_list()) {
			const variant class_node = classes[key];
			sub_classes_[key.as_string()].reset(new formula_class(class_node));
		}
	}

	unit_test_ = node["test"];
}

void formula_class::set_name(const std::string& name)
{
	name_ = name;
	name_variant_ = variant(name);
	for(classes_map::iterator i = sub_classes_.begin(); i != sub_classes_.end(); ++i) {
		i->second->set_name(name + "." + i->first);
	}
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

classes_map classes_;

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

	const variant v = json::parse_from_file("data/classes/" + type + ".cfg");
	ASSERT_LOG(v.is_map(), "COULD NOT FIND FFL CLASS: " << type);

	boost::intrusive_ptr<formula_class> result(new formula_class(v));
	result->set_name(type);
	classes_[type] = result;
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
	boost::intrusive_ptr<formula_object> res(new formula_object(type, args));
	res->call_constructors(args);
	return res;
}

formula_object::formula_object(const std::string& type, variant args)
  : class_(get_class(type))
{
	private_data_ = deep_copy_variant(class_->private_data());
}

void formula_object::call_constructors(variant args)
{
	if(args.is_map()) {
		foreach(const variant& key, args.get_keys().as_list()) {
			std::map<std::string, property_entry>::const_iterator itor = class_->properties().find(key.as_string());
			if(itor != class_->properties().end() && itor->second.setter.get() == NULL && itor->second.variable.is_null()) {
				//A read-only property. Set the formula to what is passed in.
				property_overrides_.insert(std::pair<std::string, formula_ptr>(key.as_string(), formula_ptr(new formula(args[key]))));
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
  : class_(get_class(data["@class"].as_string()))
{
	if(data.is_map() && data["private"].is_map()) {
		private_data_ = deep_copy_variant(data["private"]);
	} else {
		private_data_ = deep_copy_variant(class_->private_data());
	}

	if(data.is_map() && data["property_overrides"].is_map()) {
		const variant overrides = data["property_overrides"];
		foreach(const variant::map_pair& p, overrides.as_map()) {
			property_overrides_[p.first.as_string()].reset(new formula(p.second));
		}
	}

	set_addr(data["_addr"].as_string());
}

formula_object::~formula_object()
{}

variant formula_object::serialize_to_wml() const
{
	std::map<variant, variant> result;
	result[variant("@class")] = variant(class_->name());
	result[variant("private")] = deep_copy_variant(private_data_);

	if(property_overrides_.empty() == false) {
		std::map<variant, variant> properties;
		for(std::map<std::string, formula_ptr>::const_iterator i = property_overrides_.begin(); i != property_overrides_.end(); ++i) {
			properties[variant(i->first)] = variant(i->second->str());
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

	std::map<std::string, formula_ptr>::const_iterator override_itor = property_overrides_.find(key);
	if(override_itor != property_overrides_.end()) {
		private_data_scope scope(expose_private_data_);
		return override_itor->second->execute(*this);
	}

	std::map<std::string, property_entry>::const_iterator itor = class_->properties().find(key);
	ASSERT_LOG(itor != class_->properties().end(), "UNKNOWN PROPERTY ACCESS " << key << " IN CLASS " << class_->name() << "\nFORMULA LOCATION: " << get_call_stack());

	if(itor->second.getter) {
		private_data_scope scope(expose_private_data_);
		return itor->second.getter->execute(*this);
	} else if(itor->second.variable.is_null() == false) {
		return private_data_[itor->second.variable];
	} else {
		ASSERT_LOG(false, "ILLEGAL READ PROPERTY ACCESS OF NON-READABLE VARIABLE " << key << " IN CLASS " << class_->name());
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

	std::map<std::string, property_entry>::const_iterator itor = class_->properties().find(key);
	ASSERT_LOG(itor != class_->properties().end(), "UNKNOWN PROPERTY ACCESS " << key << " IN CLASS " << class_->name());

	if(itor->second.types.empty() == false) {
		if(std::find(itor->second.types.begin(), itor->second.types.end(), value.type()) == itor->second.types.end()) {
			ASSERT_LOG(false, "ILLEGAL WRITE PROPERTY ACCESS: SETTING VARIABLE " << key << " IN CLASS " << class_->name() << " TO INVALID TYPE " << variant::variant_type_to_string(value.type()));
		}
	}

	if(itor->second.setter) {
		private_data_scope scope(expose_private_data_, &tmp_value_, &value);
		execute_command(itor->second.setter->execute(*this));
	} else if(itor->second.variable.is_null() == false) {
		private_data_.add_attr_mutation(itor->second.variable, value);
	} else {
		ASSERT_LOG(false, "ILLEGAL WRITE PROPERTY ACCESS OF NON-WRITABLE VARIABLE " << key << " IN CLASS " << class_->name());
	}

}

void formula_object::get_inputs(std::vector<formula_input>* inputs) const
{
	for(std::map<std::string, property_entry>::const_iterator i = class_->properties().begin(); i != class_->properties().end(); ++i) {
		FORMULA_ACCESS_TYPE type = FORMULA_READ_ONLY;
		if(i->second.getter && i->second.setter || i->second.variable.is_null() == false) {
			type = FORMULA_READ_WRITE;
		} else if(i->second.getter) {
			type = FORMULA_READ_ONLY;
		} else if(i->second.setter) {
			type = FORMULA_WRITE_ONLY;
		} else {
			continue;
		}

		inputs->push_back(formula_input(i->first, type));
	}
}

}
