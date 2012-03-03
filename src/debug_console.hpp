#ifndef DEBUG_CONSOLE_HPP_INCLUDED
#define DEBUG_CONSOLE_HPP_INCLUDED

class level;
class entity;

#include <string>

namespace debug_console
{

void add_message(const std::string& msg);
void draw();

void show_interactive_console(level& lvl, entity& obj);

}

#endif
