#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <assert.h>
#include <map>

#include "asserts.hpp"
#include "custom_object.hpp"
#include "custom_object_functions.hpp"
#include "custom_object_type.hpp"
#include "draw_number.hpp"
#include "formula.hpp"
#include "frame.hpp"
#include "gui_formula_functions.hpp"
#include "level.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"

using namespace game_logic;

namespace {

typedef boost::function<void (const gui_algorithm* algo)> gui_command_function;

class gui_command : public formula_callable {
	gui_command_function fn_;
	variant get_value(const std::string& key) const { return variant(); }
public:
	explicit gui_command(const gui_command_function& fn) : fn_(fn) {
	}

	void execute(const gui_algorithm& algo) {
		fn_(&algo);
	}
};

class draw_number_function : public function_expression {
public:
	explicit draw_number_function(const args_list& args)
	  : function_expression("draw_number", args, 4, 4)
	{}
private:
	static void draw(const gui_algorithm*, int number, int places, int x, int y) {
		draw_number(number, places, x, y);
	}
	variant execute(const formula_callable& variables) const {
		const int number = args()[0]->evaluate(variables).as_int();
		const int places = args()[1]->evaluate(variables).as_int();
		const int x = args()[2]->evaluate(variables).as_int();
		const int y = args()[3]->evaluate(variables).as_int();

		return variant(new gui_command(boost::bind(draw, _1, number, places, x, y)));
	}
};

class draw_animation_function : public function_expression {
public:
	explicit draw_animation_function(const args_list& args)
	  : function_expression("draw_animation", args, 3, 5)
	{}
private:
	variant execute(const formula_callable& variables) const {
		std::vector<variant> v;
		v.reserve(args().size());
		for(int n = 0; n != args().size(); ++n) {
			v.push_back(args()[n]->evaluate(variables));
		}

		int index = 0;
		std::string object;
		if(v[1].is_string()) {
			index = 1;
			object = v[0].as_string();
			ASSERT_GE(v.size(), 4);
		}

		std::string anim = v[index].as_string();
		const int x = v[index+1].as_int();
		const int y = v[index+2].as_int();
		int time = -1;
		if(index+3 < v.size()) {
			time = v[index+3].as_int();
		}

		return variant(new gui_command(boost::bind(&gui_algorithm::draw_animation, _1, object, anim, x, y, time)));
	}
};

class gui_command_function_symbol_table : public function_symbol_table
{
public:
	static gui_command_function_symbol_table& instance() {
		static gui_command_function_symbol_table result;
		return result;
	}

	expression_ptr create_function(
	                           const std::string& fn,
	                           const std::vector<expression_ptr>& args) const
	{
		if(fn == "draw_animation") {
			return expression_ptr(new draw_animation_function(args));
		} else if(fn == "draw_number") {
			return expression_ptr(new draw_number_function(args));
		}

		return function_symbol_table::create_function(fn, args);
	}
};

} // namespace

gui_algorithm::gui_algorithm(wml::const_node_ptr node)
	  : lvl_(NULL),
	  draw_formula_(formula::create_optional_formula(node->attr("on_draw"), &gui_command_function_symbol_table::instance())),
	  process_formula_(formula::create_optional_formula(node->attr("on_process"), &get_custom_object_functions_symbol_table())),
	  cycle_(0), object_(new custom_object("dummy_gui_object", 0, 0, true))
{
	object_->add_ref();
	std::cerr << "dummy_gui_object REF COUNT " << object_->refcount() << "\n";
	FOREACH_WML_CHILD(frame_node, node, "animation") {
		frame_ptr f(new frame(frame_node));
		frames_[frame_node->attr("id")] = f;
	}
}

gui_algorithm::~gui_algorithm()
{
}

gui_algorithm_ptr gui_algorithm::get(const std::string& key) {
	static std::map<std::string, gui_algorithm_ptr> algorithms;
	gui_algorithm_ptr& ptr = algorithms[key];
	if(!ptr) {
		ptr = create(key);
	}

	return ptr;
}

void gui_algorithm::process(level& lvl) {
	lvl_ = &lvl;
	++cycle_;
	if(process_formula_) {
		object_->set_level(lvl);
		variant result = process_formula_->execute(*this);
		object_->execute_command(result);
	}
}

void gui_algorithm::draw(const level& lvl) {
	lvl_ = &lvl;
	if(draw_formula_) {
		variant result = draw_formula_->execute(*this);
		execute_command(result);
	}
}

void gui_algorithm::execute_command(variant v) {
	if(v.is_list()) {
		for(int n = 0; n != v.num_elements(); ++n) {
			execute_command(v[n]);
		}

		return;
	}

	gui_command* cmd = v.try_convert<gui_command>();
	if(cmd) {
		cmd->execute(*this);
	}
}

gui_algorithm_ptr gui_algorithm::create(const std::string& key) {
	return gui_algorithm_ptr(new gui_algorithm(wml::parse_wml_from_file("data/gui/" + key + ".cfg")));
}

void gui_algorithm::draw_animation(const std::string& object_name, const std::string& anim, int x, int y, int cycle) const {
	const frame* f = NULL;
	if(object_name.empty() == false) {
		const_custom_object_type_ptr obj = custom_object_type::get(object_name);
		if(obj) {
			f = &obj->get_frame(anim);
		}
	}

	if(!f) {
		const std::map<std::string, frame_ptr>::const_iterator itor = frames_.find(anim);
		if(itor != frames_.end()) {
			f = itor->second.get();
		}
	}

	if(f) {
		if(cycle == -1) {
			cycle = cycle_%f->duration();
		}

		f->draw(x, y, true, false, cycle);
	}
}

variant gui_algorithm::get_value(const std::string& key) const
{
	if(key == "level") {
		return variant(lvl_);
	} else if(key == "object") {
		std::cerr << "GET dummy_gui_object: " << object_->refcount() << "\n";
		return variant(object_.get());
	} else if(key == "cycle") {
		return variant(cycle_);
	} else {
		return variant();
	}
}
