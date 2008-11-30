#include <boost/shared_ptr.hpp>

#include "gui.hpp"
#include "wml_node.hpp"

namespace {
std::map<std::string, boost::shared_ptr<frame> > cache;
}

void init_gui_frames(wml::const_node_ptr node)
{
	wml::node::const_child_iterator i1 = node->begin_child("frame");
	wml::node::const_child_iterator i2 = node->end_child("frame");
	for(; i1 != i2; ++i1) {
		cache[i1->second->attr("id")].reset(new frame(i1->second));
	}
}

const frame* get_gui_frame(const std::string& id)
{
	std::map<std::string, boost::shared_ptr<frame> >::const_iterator i = cache.find(id);
	if(i == cache.end()) {
		return NULL;
	}

	return i->second.get();
}
