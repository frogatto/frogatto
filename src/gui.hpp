#ifndef GUI_HPP_INCLUDED
#define GUI_HPP_INCLUDED

#include <string>

#include "frame.hpp"
#include "wml_node_fwd.hpp"

void init_gui_frames(wml::const_node_ptr node);
const frame* get_gui_frame(const std::string& id);

#endif
