#ifndef DEBUG_CONSOLE_HPP_INCLUDED
#define DEBUG_CONSOLE_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

#include "decimal.hpp"
#include "dialog.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "text_editor_widget.hpp"

class level;
class entity;

#include <string>

namespace debug_console
{

void add_graph_sample(const std::string& id, decimal value);
void process_graph();
void draw_graph();

void add_message(const std::string& msg);
void draw();

class console_dialog : public gui::dialog
{
public:
	console_dialog(level& lvl, entity& obj);
	~console_dialog();

	bool has_keyboard_focus() const;

	void add_message(const std::string& msg);

	void set_focus(entity_ptr e);
private:
	console_dialog(const console_dialog&);
	void init();
	bool handle_event(const SDL_Event& event, bool claimed);

	gui::text_editor_widget* text_editor_;

	boost::intrusive_ptr<level> lvl_;
	entity_ptr focus_;

	void on_move_cursor();
	bool on_begin_enter();
	void on_enter();

	void load_history();
	std::vector<std::string> history_;
	int history_pos_;
};

void show_interactive_console(level& lvl, entity& obj);

}

#endif
