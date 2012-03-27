#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <assert.h>
#include <map>

#include "asserts.hpp"
#include "custom_object.hpp"
#include "custom_object_callable.hpp"
#include "custom_object_functions.hpp"
#include "custom_object_type.hpp"
#include "draw_number.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "formula_callable_definition.hpp"
#include "frame.hpp"
#include "graphical_font.hpp"
#include "gui_formula_functions.hpp"
#include "iphone_controls.hpp"
#include "json_parser.hpp"
#include "level.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "string_utils.hpp"
#include "unit_test.hpp"
#include "variant_utils.hpp"

using namespace game_logic;

namespace {

typedef boost::function<void (const gui_algorithm* algo)> gui_command_function;

class gui_command : public formula_callable {
	variant get_value(const std::string& key) const { return variant(); }
public:

	virtual void execute(const gui_algorithm& algo) = 0;
};

class draw_text_command : public gui_command {
	const_graphical_font_ptr font_;
	std::string text_;
	int x_, y_;
public:
	draw_text_command(const_graphical_font_ptr font, const std::string& text, int x, int y)
	  : font_(font), text_(text), x_(x), y_(y)
	{}

	void execute(const gui_algorithm& algo) {
		font_->draw(x_, y_, text_);
	}
};

class draw_text_function : public function_expression {
public:
	explicit draw_text_function(const args_list& args)
	 : function_expression("draw_text", args, 3, 4)
	{}
private:
	variant execute(const formula_callable& variables) const {
		std::string font = "default";

		int arg = 0;
		if(args().size() == 4) {
			font = args()[arg++]->evaluate(variables).as_string();
		}

		std::string text = args()[arg++]->evaluate(variables).as_string();
		const int x = args()[arg++]->evaluate(variables).as_int();
		const int y = args()[arg++]->evaluate(variables).as_int();
		return variant(new draw_text_command(graphical_font::get(font), text, x, y));
	}
};

class draw_number_command : public gui_command {
	graphics::blit_queue blit_;
public:
	draw_number_command(int number, int places, int x, int y)
	{
		queue_draw_number(blit_, number, places, x, y);
	}

	void execute(const gui_algorithm& algo) {
		blit_.do_blit();
	}
};

class draw_number_function : public function_expression {
public:
	explicit draw_number_function(const args_list& args)
	  : function_expression("draw_number", args, 4, 4)
	{}
private:
	variant execute(const formula_callable& variables) const {
		static cache_entry cache[4];
		static int cache_loc;

		const int number = args()[0]->evaluate(variables).as_int();
		const int places = args()[1]->evaluate(variables).as_int();
		const int x = args()[2]->evaluate(variables).as_int();
		const int y = args()[3]->evaluate(variables).as_int();
		for(int n = 0; n != sizeof(cache)/sizeof(*cache); ++n) {
			if(x == cache[n].x && y == cache[n].y && number == cache[n].number && places == cache[n].places) {
				return cache[n].result;
			}
		}
		const int n = cache_loc++%(sizeof(cache)/sizeof(*cache));
		cache[n].x = x;
		cache[n].y = y;
		cache[n].number = number;
		cache[n].places = places;
		cache[n].result = variant(new draw_number_command(number, places, x, y));
		return cache[n].result;
	}

	struct cache_entry {
		cache_entry() : x(-1), y(-1) {}
		int number, places, x, y;
		variant result;
	};

};

class draw_animation_area_command : public gui_command {
	frame_ptr frame_;
	int x_, y_;
	rect area_;

	variant get_value(const std::string& key) const { return variant(); }
public:
	draw_animation_area_command(const frame_ptr& f, int x, int y, const rect& area)
	  : frame_(f), x_(x), y_(y), area_(area)
	{}

	void execute(const gui_algorithm& algo) {
		frame_->draw(x_, y_, area_);
	}
};

class draw_animation_area_function : public function_expression {
public:
	draw_animation_area_function(gui_algorithm* algo, const args_list& args)
	  : function_expression("draw_animation_area", args, 4, 7), algo_(algo)
	{}

	gui_algorithm* algo_;
private:
	variant execute(const formula_callable& variables) const {
		variant anim = args()[0]->evaluate(variables);
		const frame_ptr f = algo_->get_frame(anim.as_string());
		if(!f) {
			return variant();
		}

		const int x = args()[1]->evaluate(variables).as_int();
		const int y = args()[2]->evaluate(variables).as_int();

		int vals[4];
		for(int n = 0; n != args().size() - 3; ++n) {
			vals[n] = args()[n+3]->evaluate(variables).as_int();
		}

		rect area;
		if(args().size() == 4) {
			area = rect(0, 0, vals[0], f->height()/2);
		} else if(args().size() == 5) {
			area = rect(0, 0, vals[0], vals[1]);
			                  
		} else {
			ASSERT_EQ(args().size(), 7);
			area = rect(vals[0], vals[1], vals[2], vals[3]);
		}

		return variant(new draw_animation_area_command(f, x, y, area));
	}
};

class draw_animation_command : public gui_command {
	graphics::blit_queue blit_;
	variant get_value(const std::string& key) const { return variant(); }
public:
	draw_animation_command(const frame_ptr& f, int x, int y)
	{
		f->draw_into_blit_queue(blit_, x, y);
	}

	void execute(const gui_algorithm& algo) {
		blit_.do_blit();
	}
};

class draw_animation_function : public function_expression {
public:
	draw_animation_function(gui_algorithm* algo, const args_list& args)
	  : function_expression("draw_animation", args, 3, 3), algo_(algo)
	{}

	gui_algorithm* algo_;
private:
	variant execute(const formula_callable& variables) const {
		variant anim = args()[0]->evaluate(variables);
		const frame_ptr f = algo_->get_frame(anim.as_string());
		if(!f) {
			return variant();
		}

		const int x = args()[1]->evaluate(variables).as_int();
		const int y = args()[2]->evaluate(variables).as_int();

		return variant(new draw_animation_command(f, x, y));
	}
};

class color_command : public gui_command {
	unsigned char r_, g_, b_, a_;
public:
	color_command(int r, int g, int b, int a) : r_(r), g_(g), b_(b), a_(a)
	{}

	void execute(const gui_algorithm& algo) {
		glColor4ub(r_, g_, b_, a_);
	}
};

class color_function : public function_expression {
public:
	explicit color_function(const args_list& args)
	  : function_expression("color", args, 4, 4)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(new color_command(
		         args()[0]->evaluate(variables).as_int(),
		         args()[1]->evaluate(variables).as_int(),
		         args()[2]->evaluate(variables).as_int(),
		         args()[3]->evaluate(variables).as_int()));
	}
};

class gui_command_function_symbol_table : public function_symbol_table
{
	gui_algorithm* algo_;
public:
	gui_command_function_symbol_table(gui_algorithm* algo) : algo_(algo)
	{}

	expression_ptr create_function(
	                           const std::string& fn,
	                           const std::vector<expression_ptr>& args,
							   const formula_callable_definition* callable_def) const
	{
		if(fn == "draw_animation") {
			return expression_ptr(new draw_animation_function(algo_, args));
		} else if(fn == "draw_animation_area") {
			return expression_ptr(new draw_animation_area_function(algo_, args));
		} else if(fn == "draw_number") {
			return expression_ptr(new draw_number_function(args));
		} else if(fn == "draw_text") {
			return expression_ptr(new draw_text_function(args));
		} else if(fn == "color") {
			return expression_ptr(new color_function(args));
		}

		return function_symbol_table::create_function(fn, args, callable_def);
	}
};

const std::string AlgorithmProperties[] = {
	"level", "object", "cycle", "screen_width", "screen_height",
};

enum ALGORITHM_PROPERTY_ID {
	ALGO_LEVEL, ALGO_OBJECT, ALGO_CYCLE, ALGO_SCREEN_WIDTH, ALGO_SCREEN_HEIGHT,
};

class gui_algorithm_definition : public game_logic::formula_callable_definition
{
public:
	static const gui_algorithm_definition& instance() {
		static const gui_algorithm_definition def;
		return def;
	}

	gui_algorithm_definition() {
		for(int n = 0; n != sizeof(AlgorithmProperties)/sizeof(*AlgorithmProperties); ++n) {
			entries_.push_back(entry(AlgorithmProperties[n]));
			keys_to_slots_[AlgorithmProperties[n]] = n;
		}

		entries_[ALGO_OBJECT].type_definition = &custom_object_type::get("dummy_gui_object")->callable_definition();
		entries_[ALGO_LEVEL].type_definition = &level::get_formula_definition();
	}

	int get_slot(const std::string& key) const {
		std::map<std::string, int>::const_iterator i = keys_to_slots_.find(key);
		if(i == keys_to_slots_.end()) {
			return -1;
		}

		return i->second;
	}

	const entry* get_entry(int slot) const {
		if(slot < 0 || slot >= entries_.size()) {
			return NULL;
		}

		return &entries_[slot];
	}

	int num_slots() const {
		return entries_.size();
	}
private:
	std::vector<entry> entries_;
	std::map<std::string, int> keys_to_slots_;
};

} // namespace

gui_algorithm::gui_algorithm(variant node)
	  : lvl_(NULL),
	  process_formula_(formula::create_optional_formula(node["on_process"], &get_custom_object_functions_symbol_table(), &gui_algorithm_definition::instance())),
	  cycle_(0), object_(new custom_object("dummy_gui_object", 0, 0, true))
{
	gui_command_function_symbol_table symbols(this);

	object_->add_ref();
	std::vector<std::string> includes = util::split(node["includes"].as_string_default());
	foreach(const std::string& inc, includes) {
		includes_.push_back(get(inc));
	}

	set_object(object_);

	foreach(variant frame_node, node["animation"].as_list()) {
		frame_ptr f(new frame(frame_node));
		frames_[frame_node["id"].as_string()] = f;
	}

	draw_formula_ = formula::create_optional_formula(node["on_draw"], &symbols, &gui_algorithm_definition::instance());

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

void gui_algorithm::set_object(boost::intrusive_ptr<custom_object> obj) {
	object_ = obj;
	foreach(gui_algorithm_ptr inc, includes_) {
		inc->set_object(obj);
	}
}

void gui_algorithm::new_level() {
	cycle_ = 0;
	set_object(boost::intrusive_ptr<custom_object>(new custom_object("dummy_gui_object", 0, 0, true)));
}

void gui_algorithm::process(level& lvl) {
	lvl_ = &lvl;
	++cycle_;
	if((cycle_%2) == 0 && process_formula_) {
		object_->set_level(lvl);
		variant result = process_formula_->execute(*this);
		object_->execute_command(result);
	}

	foreach(gui_algorithm_ptr p, includes_) {
		p->process(lvl);
	}
}

void gui_algorithm::draw(const level& lvl) {
	lvl_ = &lvl;

	//fprintf(stderr, "GUI_DRAW: %p %d\n", this, cycle_);
	if((cycle_%2) == 0) {
		cached_draw_commands_ = variant();
	}

	if(cached_draw_commands_.is_null() && draw_formula_) {
		cached_draw_commands_ = draw_formula_->execute(*this);
	}

	execute_command(cached_draw_commands_);

	glColor4ub(255, 255, 255, 255);

	foreach(gui_algorithm_ptr p, includes_) {
		//fprintf(stderr, "DRAW CHILD: %p -> %p\n", this, p.get());
		p->draw(lvl);
	}
}

void gui_algorithm::execute_command(variant v) {
	if(v.is_list()) {
		const int num_elements = v.num_elements();
		for(int n = 0; n != num_elements; ++n) {
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
	static const std::string path = preferences::load_compiled() ? "data/compiled/gui/" : "data/gui/";
	return gui_algorithm_ptr(new gui_algorithm(json::parse_from_file(path + key + ".cfg")));
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

void gui_algorithm::color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) const
{
	glColor4ub(r, g, b, a);
}

frame_ptr gui_algorithm::get_frame(const std::string& id) const
{
	const std::map<std::string, frame_ptr>::const_iterator itor = frames_.find(id);
	if(itor != frames_.end()) {
		return itor->second;
	} else {
		return frame_ptr();
	}
}

variant gui_algorithm::get_value(const std::string& key) const
{
	if(key == "level") {
		return variant(lvl_);
	} else if(key == "object") {
		return variant(object_.get());
	} else if(key == "cycle") {
		return variant(cycle_);
	} else if(key == "screen_width") {
		return variant(graphics::screen_width());
	} else if(key == "screen_height") {
		return variant(graphics::screen_height());
	} else {
		return variant();
	}
}

variant gui_algorithm::get_value_by_slot(int slot) const
{
	switch(slot) {
	case ALGO_LEVEL:
		return variant(lvl_);
	case ALGO_OBJECT:
		return variant(object_.get());
	case ALGO_CYCLE:
		return variant(cycle_);
	case ALGO_SCREEN_WIDTH:
		return variant(graphics::screen_width());
	case ALGO_SCREEN_HEIGHT:
		return variant(graphics::screen_height());
	}

	return variant();
}

#include "level.hpp"

BENCHMARK(gui_algorithm_bench)
{
	static boost::intrusive_ptr<level> lvl;
	if(!lvl) {
		lvl = boost::intrusive_ptr<level>(new level("titlescreen.cfg"));
		lvl->finish_loading();
		lvl->set_as_current_level();
	}

	static gui_algorithm_ptr gui(gui_algorithm::get("default"));
	BENCHMARK_LOOP {
		gui->draw(*lvl);
	}
}
