#include "character.hpp"
#include "custom_object.hpp"
#include "custom_object_type.hpp"
#include "formatter.hpp"
#include "item.hpp"
#include "level.hpp"
#include "sound.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

item::item(wml::const_node_ptr node)
  : type_(item_type::get(node->attr("type"))),
     x_(wml::get_int(node, "x")),
     y_(wml::get_int(node, "y")),
	 time_in_frame_(0),
	 touched_(node->attr("touched").str() == "yes")
{
}

wml::node_ptr item::write() const
{
	wml::node_ptr node(new wml::node("item"));
	node->set_attr("type", type_->id());
	node->set_attr("x", formatter() << x_);
	node->set_attr("y", formatter() << y_);
	if(touched_) {
		node->set_attr("touched", "yes");
	}
	return node;
}

void item::process(level& lvl)
{
	++time_in_frame_;
	if(!touched_ && time_in_frame_ == type_->get_frame().duration()) {
		time_in_frame_ = 0;
	}

	if(!touched_) {
		character_ptr player = lvl.player();
		if(player) {
			if(rects_intersect(rect(x_, y_, type_->get_frame().width(), type_->get_frame().height()), player->body_rect()) &&
			   (type_->automatic_touch() || player->enter()) &&
			   (!type_->touch_condition() || type_->touch_condition()->execute(*player).as_bool())) {
				time_in_frame_ = 0;
				touched_ = true;

				if(type_->on_touch_music().empty() == false) {
					sound::play_music_interrupt(type_->on_touch_music());
				}

				for(int n = 0; n != type_->num_on_touch_particles(); ++n) {
					const_custom_object_type_ptr particle_type = custom_object_type::get(type_->on_touch_particles());
					if(!particle_type) {
						break;
					}

					const int particle_width = particle_type->default_frame().width();
					const int particle_height = particle_type->default_frame().height();

					const int x1 = x_ - particle_width/2;
					const int y1 = y_ - particle_height/2;
					const int x2 = x1 + type_->get_frame().width();
					const int y2 = y1 + type_->get_frame().height();
					entity_ptr particle(new custom_object(type_->on_touch_particles(), x1 + rand()%(x2-x1), y1 + rand()%(y2-y1), true));
					lvl.add_character(particle);
				}

				if(type_->target()) {
					variant var = type_->target()->execute(*player);
					player = character_ptr(var.convert_to<character>());
				}

				if(player) {
					for(std::map<std::string, game_logic::const_formula_ptr>::const_iterator i = type_->on_touch().begin(); i != type_->on_touch().end(); ++i) {
						if(!i->second) {
							continue;
						}

						player->mutate_value(i->first, i->second->execute(*player));
					}
				}
			}
		}
	}
}

bool item::destroyed() const
{
	if(touched_ && type_->destroy_on_touch() && (!type_->touched_frame() || time_in_frame_ >= type_->touched_frame()->duration())) {
		return true;
	}

	return false;
}

void item::draw() const
{
	const frame* f = NULL;
	if(touched_) {
		f = type_->touched_frame();

		if(f && time_in_frame_ == 0) {
			f->play_sound();
		}
	}
	
	if(!f) {
		f = &type_->get_frame();
	}

	f->draw(x_, y_, true, time_in_frame_);
}

variant item::get_value(const std::string& key) const
{
	return variant();
}

void item::get_inputs(std::vector<game_logic::formula_input>* inputs) const
{
}
