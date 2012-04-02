/* $Id: formula_callable.hpp 25895 2008-04-17 18:57:13Z mordante $ */
/*
   Copyright (C) 2008 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2
   or at your option any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef FORMULA_CALLABLE_HPP_INCLUDED
#define FORMULA_CALLABLE_HPP_INCLUDED

#include <iostream>
#include <map>
#include <string>

#include "reference_counted_object.hpp"
#include "variant.hpp"

namespace game_logic
{

enum FORMULA_ACCESS_TYPE { FORMULA_READ_ONLY, FORMULA_WRITE_ONLY, FORMULA_READ_WRITE };
struct formula_input {
	std::string name;
	FORMULA_ACCESS_TYPE access;
	formula_input(const std::string& name, FORMULA_ACCESS_TYPE access=FORMULA_READ_WRITE)
			: name(name), access(access)
	{}
};

//interface for objects that can have formulae run on them
class formula_callable : public reference_counted_object {
public:
	explicit formula_callable(bool has_self=true) : has_self_(has_self)
	{}

	variant query_value(const std::string& key) const {
		if(has_self_ && key == "self") {
			return variant(this);
		}
		return get_value(key);
	}

	variant query_value_by_slot(int slot) const {
		return get_value_by_slot(slot);
	}

	void mutate_value(const std::string& key, const variant& value) {
		set_value(key, value);
	}

	void mutate_value_by_slot(int slot, const variant& value) {
		set_value_by_slot(slot, value);
	}

	std::vector<formula_input> inputs() const {
		std::vector<formula_input> res;
		get_inputs(&res);
		return res;
	}

	bool equals(const formula_callable* other) const {
		return do_compare(other) == 0;
	}

	bool less(const formula_callable* other) const {
		return do_compare(other) < 0;
	}

	virtual void get_inputs(std::vector<formula_input>* /*inputs*/) const {};

	void serialize(std::string& str) const {
		serialize_to_string(str);
	}

	bool has_key(const std::string& key) const 
		{ return !query_value(key).is_null(); }

protected:
	virtual ~formula_callable() {}

	virtual void set_value(const std::string& key, const variant& value);
	virtual void set_value_by_slot(int slot, const variant& value);
	virtual int do_compare(const formula_callable* callable) const {
		return this < callable ? -1 : (this == callable ? 0 : 1);
	}

	virtual void serialize_to_string(std::string& str) const {
		std::cerr << "CONTENTS {{{" << str << "}}}\n";
		throw type_error("Tried to serialize type which cannot be serialized");
	}
private:
	virtual variant get_value(const std::string& key) const = 0;
	virtual variant get_value_by_slot(int slot) const;
	bool has_self_;
};

class formula_callable_no_ref_count : public formula_callable {
public:
	formula_callable_no_ref_count() {
		turn_reference_counting_off();
	}
	virtual ~formula_callable_no_ref_count() {}
};

class formula_callable_with_backup : public formula_callable {
	const formula_callable& main_;
	const formula_callable& backup_;
	variant get_value(const std::string& key) const {
		variant var = main_.query_value(key);
		if(var.is_null()) {
			return backup_.query_value(key);
		}

		return var;
	}

	void get_inputs(std::vector<formula_input>* inputs) const {
		main_.get_inputs(inputs);
		backup_.get_inputs(inputs);
	}
public:
	formula_callable_with_backup(const formula_callable& main, const formula_callable& backup) : formula_callable(false), main_(main), backup_(backup)
	{}
};

class formula_variant_callable_with_backup : public formula_callable {
	variant var_;
	const formula_callable& backup_;
	variant get_value(const std::string& key) const {
		variant var = var_.get_member(key);
		if(var.is_null()) {
			return backup_.query_value(key);
		}

		return var;
	}

	void get_inputs(std::vector<formula_input>* inputs) const {
		backup_.get_inputs(inputs);
	}

public:
	formula_variant_callable_with_backup(const variant& var, const formula_callable& backup) : formula_callable(false), var_(var), backup_(backup)
	{}
};

class map_formula_callable : public formula_callable {
public:
	explicit map_formula_callable(variant node);
	explicit map_formula_callable(const formula_callable* fallback=NULL);
	explicit map_formula_callable(const std::map<std::string, variant>& m);
	variant write() const;
	map_formula_callable& add(const std::string& key, const variant& value);
	void set_fallback(const formula_callable* fallback) { fallback_ = fallback; }

	//adds an entry and gets direct access to the variant. Use with caution
	//and for cases where calling add() repeatedy isn't efficient enough.
	variant& add_direct_access(const std::string& key);

	bool empty() const { return values_.empty(); }
	void clear() { values_.clear(); }
	bool contains(const std::string& key) const { return values_.count(key) != 0; }

	const std::map<std::string, variant>& values() const { return values_; }

	typedef std::map<std::string,variant>::const_iterator const_iterator;

	const_iterator begin() const { return values_.begin(); }
	const_iterator end() const { return values_.end(); }

	variant& ref(const std::string& key) { return values_[key]; }

private:
	//map_formula_callable(const map_formula_callable&);

	variant get_value(const std::string& key) const;
	void get_inputs(std::vector<formula_input>* inputs) const;
	void set_value(const std::string& key, const variant& value);
	std::map<std::string,variant> values_;
	const formula_callable* fallback_;
};

typedef boost::intrusive_ptr<formula_callable> formula_callable_ptr;
typedef boost::intrusive_ptr<const formula_callable> const_formula_callable_ptr;

typedef boost::intrusive_ptr<map_formula_callable> map_formula_callable_ptr;
typedef boost::intrusive_ptr<const map_formula_callable> const_map_formula_callable_ptr;

class command_callable : public formula_callable {
public:
	virtual void execute(formula_callable& context) const = 0;
private:
	variant get_value(const std::string& key) const { return variant(); }
	void get_inputs(std::vector<game_logic::formula_input>* inputs) const {}
};

}

#endif
