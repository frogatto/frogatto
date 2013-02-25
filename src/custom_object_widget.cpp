#include "custom_object_widget.hpp"
#include "level.hpp"

namespace gui
{
	custom_object_widget::custom_object_widget(const variant& v, game_logic::formula_callable* e)
		: widget(v, e)
	{
		ASSERT_LOG(get_environment() != 0, "You must specify a callable environment");
		ASSERT_LOG(v.has_key("object") == true, "You must provide an object");
		init(v);
	}

	custom_object_widget::~custom_object_widget()
	{
	}

	void custom_object_widget::init(const variant& v)
	{
		entity_.reset();
		if(v["object"].is_string()) {
			// type name, has obj_x, obj_y, facing			
			entity_ = entity_ptr(new custom_object(v["object"].as_string(), v["obj_x"].as_int(0), v["obj_y"].as_int(0), v["facing"].as_int(0)));
			entity_->finish_loading(NULL);
		} else if(v["object"].is_map()) {
			entity_ = entity_ptr(new custom_object(v["object"]));
			entity_->finish_loading(NULL);
		} else {
			entity_ = v["object"].try_convert<entity>();
			ASSERT_LOG(entity_ != NULL, "Couldn't convert 'object' attribue to an entity");
			entity_->finish_loading(NULL);
		}
		if(v.has_key("properties")) {
			ASSERT_LOG(v["properties"].is_map(), "properties field must be a map");
			const variant& properties = v["properties"];
			variant keys = properties.get_keys();
			for(int n = 0; n != keys.num_elements(); ++n) {
				variant value = properties[keys[n]];
				entity_->mutate_value(keys[n].as_string(), value);
			}
		}
		handle_process_on_entity_ = v["handle_process"].as_bool(false);
	}

	void custom_object_widget::set_entity(entity_ptr e)
	{
		entity_ = e;
	}

	entity_ptr custom_object_widget::get_entity()
	{
		return entity_;
	}

	const_entity_ptr custom_object_widget::get_entity() const
	{
		return entity_;
	}

	void custom_object_widget::set_value(const std::string& key, const variant& value)
	{
		if(key == "object") {
			std::map<variant, variant> m;
			m[variant("object")] = value;
			init(variant(&m));
			return;
		}
		widget::set_value(key, value);
	}

	variant custom_object_widget::get_value(const std::string& key) const
	{
		if(key == "object") {
			return variant(entity_.get());
		}
		return widget::get_value(key);
	}


	void custom_object_widget::handle_draw() const
	{
		if(entity_) {
			glPushMatrix();
			glTranslatef(GLfloat(x() & ~1), GLfloat(y() & ~1), 0.0);
			entity_->draw(x(), y());
			entity_->draw_later(x(), y());
			glPopMatrix();
		}
	}

	bool custom_object_widget::handle_event(const SDL_Event& event, bool claimed)
	{
		if(entity_) {
			custom_object* obj = static_cast<custom_object*>(entity_.get());
			return obj->handle_sdl_event(event, claimed);
		}
		return claimed;
	}

	void custom_object_widget::handle_process()
	{
		widget::handle_process();
		if(entity_ && handle_process_on_entity_) {
			custom_object* obj = static_cast<custom_object*>(entity_.get());
			obj->process(level::current());
		}
	}

}
