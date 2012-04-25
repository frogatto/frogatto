#ifndef GUI_FORMULA_FUNCTIONS_HPP_INCLUDED
#define GUI_FORMULA_FUNCTIONS_HPP_INCLUDED

#include "custom_object.hpp"
#include "variant.hpp"

#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <string>

class frame;
class level;

class gui_algorithm;
typedef boost::intrusive_ptr<gui_algorithm> gui_algorithm_ptr;
typedef boost::shared_ptr<frame> frame_ptr;

class gui_algorithm : public game_logic::formula_callable {
public:
	gui_algorithm(variant node);
	~gui_algorithm();

	static gui_algorithm_ptr get(const std::string& key);
	static gui_algorithm_ptr create(const std::string& key);

	void new_level();

	void process(level& lvl);
	void draw(const level& lvl);
	void load(level& lvl);

	void draw_animation(const std::string& object_name, const std::string& anim, int x, int y, int cycle) const;
	void color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) const;

	frame_ptr get_frame(const std::string& id) const;

	const custom_object* get_object() const { return object_.get(); }

private:
	void set_object(boost::intrusive_ptr<custom_object> obj);

	gui_algorithm(const gui_algorithm&);
	void operator=(const gui_algorithm&);

	variant get_value(const std::string& key) const;
	variant get_value_by_slot(int slot) const;

	void execute_command(variant v);

	const level* lvl_;
	game_logic::formula_ptr draw_formula_, process_formula_, load_formula_;
	int cycle_;
	bool loaded_;

	std::map<std::string, frame_ptr> frames_;

	boost::intrusive_ptr<custom_object> object_;

	variant cached_draw_commands_;

	std::vector<gui_algorithm_ptr> includes_;
};

#endif
