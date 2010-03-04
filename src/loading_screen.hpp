#ifndef LOADING_SCREEN_HPP_INCLUDED
#define LOADING_SCREEN_HPP_INCLUDED

#include <string>

#include "wml_node_fwd.hpp"

class loading_screen
{
	public:
	loading_screen (int items=0);
	void load (wml::const_node_ptr node); // preload objects defined by preload children of node, blocking, and calling draw automatically
	void draw (const std::string& message);
	void increment_status ();
	void draw_and_increment (const std::string& message) {draw(message); increment_status();}
	void set_number_of_items (int items);
	
	private:
	int items_; // number of items we'll load
	int status_; // how many items we've loaded so far
};

#endif