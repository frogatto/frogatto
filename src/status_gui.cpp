#include "entity.hpp"
#include "foreach.hpp"
#include "status_gui.hpp"

variant status_gui::get_value(const std::string& key) const
{
	if(key == "objects") {
		std::vector<variant> v;
		v.reserve(objects_.size());
		foreach(const entity_ptr& obj, objects_) {
			v.push_back(variant(obj.get()));
		}

		return variant(&v);
	}
	return variant();
}

void status_gui::set_value(const std::string& key, const variant& value)
{
	if(key == "objects") {
		objects_.clear();
		if(value.is_list()) {
			for(int n = 0; n != value.num_elements(); ++n) {
				objects_.push_back(value[n].convert_to<entity>());
			}
		}
	}
}

void status_gui::process()
{
	foreach(const entity_ptr& obj, objects_) {
		obj->handle_event("process");
	}
}

void status_gui::draw() const
{
	foreach(const entity_ptr& obj, objects_) {
		obj->draw();
	}
}
