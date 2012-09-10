#include <boost/bind.hpp>

#include "asserts.hpp"
#include "foreach.hpp"
#include "hex_object.hpp"
#include "hex_tile.hpp"
#include "string_utils.hpp"
#include "variant_utils.hpp"

namespace hex {

basic_hex_tile::basic_hex_tile(const variant node, hex_tile* owner)
	: cycle_(0), chance_(node["chance"].as_int(100)), owner_(owner), zorder_(node["zorder"].as_int(-500)),
	offset_x_(0), offset_y_(0)
{
	ASSERT_LOG(chance_ >= 1 && chance_ <= 100, "Chance must be between 1 and 100, inclusive.");
	image_ = node["image"].as_string_default();
	if(node.has_key("rect")) {
		ASSERT_LOG(node["rect"].num_elements() == 4 && node["rect"].is_list(), "rect must be a list of four(4) integers.");
		rect_ = rect::from_coordinates(node["rect"][0].as_int(), node["rect"][1].as_int(), node["rect"][2].as_int(), node["rect"][3].as_int());
	}
	if(node.has_key("animation")) {	
		if(node.is_list()) {
			nodes_ = node["animation"].as_list();
		} else {
			nodes_.push_back(node["animation"]);
		}
		frame_.reset(new frame(nodes_.front()));
	}
	if(node.has_key("offset")) {
		ASSERT_LOG(node["offset"].num_elements() == 2 && node["offset"].is_list(), "Offset field is specified as a list of two(2) elements");
		offset_x_ = node["offset"][0].as_int();
		offset_y_ = node["offset"][1].as_int();
	}
}

basic_hex_tile::~basic_hex_tile()
{}

void basic_hex_tile::draw(int x, int y) const
{
	point p(hex_map::get_pixel_pos_from_tile_pos(x,y));
	p.x -= offset_x_;
	p.y -= offset_y_;
	if(frame_) {
		frame_->draw(p.x, p.y, true, false, cycle_);
		if(++cycle_ >= frame_->duration()) {
			cycle_ = 0;
			// XXX: here we could do stuff like cycling through animations automatically
			// or calling event handlers to grab the next animation to play etc.
		}
	} else {
		graphics::blit_texture(texture_, p.x, p.y, rect_.w(), rect_.h(), 0.0f, 
			GLfloat(rect_.x())/GLfloat(texture_.width()),
			GLfloat(rect_.y())/GLfloat(texture_.height()),
			GLfloat(rect_.x2())/GLfloat(texture_.width()),
			GLfloat(rect_.y2())/GLfloat(texture_.height()));
	}
}

void basic_hex_tile::get_texture()
{
	if(!texture_.valid() && !image_.empty()) {
		texture_ = graphics::texture::get(image_);
	}
}

variant basic_hex_tile::get_value(const std::string& key) const
{
	if(key == "self") {
		return variant(this);
	} else if(key == "type") {
		return variant(type());
	} else if(key == "owner") {
		return variant(owner_);
	}
	return variant();
}

void basic_hex_tile::set_value(const std::string& key, const variant& value)
{
}

variant basic_hex_tile::write() const
{
	// XXX todo
	return variant();
}

std::string basic_hex_tile::type() const 
{ 
	ASSERT_LOG(owner_ != NULL, "Owner of tile was set to NULL!");
	return owner_->type(); 
}


hex_tile::hex_tile(const std::string& type, variant node)
	: type_(type), name_(node["name"].as_string())
{
	if(node.has_key("editor_info")) {
		ASSERT_LOG(node["editor_info"].is_map(), "Must have editor info map, none found in: " << type_);
		editor_info_.name = node["editor_info"]["name"].as_string();
		editor_info_.image = node["editor_info"]["image"].as_string();
		editor_info_.group = node["editor_info"]["group"].as_string();
		editor_info_.type = node["editor_info"]["type"].as_string();
		ASSERT_LOG(node["editor_info"]["rect"].num_elements() == 4 && node["editor_info"]["rect"].is_list(), "rect must be a list of four(4) integers.");
		editor_info_.image_rect = rect::from_coordinates(node["editor_info"]["rect"][0].as_int(), 
			node["editor_info"]["rect"][1].as_int(), 
			node["editor_info"]["rect"][2].as_int(), 
			node["editor_info"]["rect"][3].as_int());

		if(!editor_info_.texture.valid() && !editor_info_.image.empty()) {
			editor_info_.texture = graphics::texture::get(editor_info_.image);
		}
	}

	if(node.has_key("variations")) {
		ASSERT_LOG(node["variations"].is_list(), "Variations field in \"" << type_ << "\" must be a list type.");
		for(size_t i = 0; i < node["variations"].num_elements(); ++i) {
			variations_.push_back(basic_hex_tile_ptr(new basic_hex_tile(node["variations"][i], this)));
		}
	}
	if(node.has_key("transitions")) {
		ASSERT_LOG(node["transitions"].is_map(), "Transitions field in \"" << type_ << "\" must be a map type.");
		foreach(const variant_pair& p, node["transitions"].as_map()) {
			ASSERT_LOG(p.second.is_map(), "Inner of transitions of \"" << type_ << "\" must be a map type.");
			transition_map tmap;
			foreach(const variant_pair& p2, p.second.as_map()) {
				std::vector<basic_hex_tile_ptr> v;
				if(p2.second.is_list()) {
					for(size_t i = 0; i != p2.second.num_elements(); ++i) {
						v.push_back(basic_hex_tile_ptr(new basic_hex_tile(p2.second[i], this)));
					}
				} else {
					v.push_back(basic_hex_tile_ptr(new basic_hex_tile(p2.second, this)));
				}
				tmap[p2.first.as_string()] = v;
			}
			transitions_[p.first.as_string()] = tmap;
		}
	}
}

hex_tile::~hex_tile()
{}

transition_map* hex_tile::find_transition(const std::string& key)
{
	std::map<std::string, transition_map>::iterator it = transitions_.find(key);
	if(it == transitions_.end()) {
		return NULL;
	}
	return &it->second;
}

variant hex_tile::get_transitions()
{
	std::vector<variant> v;
	std::map<std::string, transition_map>::const_iterator it = transitions_.begin();
	while(it != transitions_.end()) {
		v.push_back(variant(it->first));
		++it;
	}
	return variant(&v);
}

class transition_map_callable : public game_logic::formula_callable 
{
	hex_tile_ptr tile_;
	transition_map* tm_;
	variant get_value(const std::string& key) const 
	{
		transition_map::const_iterator it = tm_->find(key);
		if(it == tm_->end()) {
			if(key == "values") {
				it = tm_->begin();
				std::vector<variant> v;
				while(it != tm_->end()) {
					v.push_back(variant(it->first));
					++it;
				}
				return variant(&v);
			}
			return variant();
		}

		int roll = rand() % it->second.size();
		it->second[roll]->get_texture();
		return variant(it->second[roll].get());
	}
	void set_value(const std::string& key, const variant& value) 
	{}
public:
	explicit transition_map_callable(const hex_tile& tile, transition_map* tm) 
		: tile_(const_cast<hex_tile*>(&tile)), tm_(tm)
	{}
};

class transition_callable : public game_logic::formula_callable 
{
	hex_tile_ptr tile_;
	variant get_value(const std::string& key) const 
	{
		transition_map* tm = tile_->find_transition(key);
		if(tm) {
			return variant(new transition_map_callable(*tile_, tm));
		}
		if(key == "values") {
			return tile_->get_transitions();
		}
		return variant();
	}
	void set_value(const std::string& key, const variant& value) 
	{}
public:
	explicit transition_callable(const hex_tile& tile) 
		: tile_(const_cast<hex_tile*>(&tile))
	{}
};

class editor_info_callable : public game_logic::formula_callable
{
	hex_tile_ptr tile_;
	variant get_value(const std::string& key) const
	{
		if(key == "type") {
			return variant(tile_->get_editor_info().type);
		} else if(key == "name") {
			return variant(tile_->get_editor_info().name);
		} else if(key == "image") {
			return variant(tile_->get_editor_info().image);
		} else if(key == "rect") {
			std::vector<variant> v;
			v.push_back(variant(tile_->get_editor_info().image_rect.x()));
			v.push_back(variant(tile_->get_editor_info().image_rect.y()));
			v.push_back(variant(tile_->get_editor_info().image_rect.w()));
			v.push_back(variant(tile_->get_editor_info().image_rect.h()));
			return variant(&v);
		} else if(key == "group") {
			return variant(tile_->get_editor_info().group);
		}
		std::map<variant, variant> m;
		m[variant("type")] = variant(tile_->get_editor_info().type);
		m[variant("name")] = variant(tile_->get_editor_info().name);
		m[variant("image")] = variant(tile_->get_editor_info().image);
		m[variant("group")] = variant(tile_->get_editor_info().group);
		std::vector<variant> v;
		v.push_back(variant(tile_->get_editor_info().image_rect.x()));
		v.push_back(variant(tile_->get_editor_info().image_rect.y()));
		v.push_back(variant(tile_->get_editor_info().image_rect.w()));
		v.push_back(variant(tile_->get_editor_info().image_rect.h()));
		m[variant("rect")] = variant(&v);
		return variant(&m);
	}
	void set_value(const std::string& key, const variant& value)
	{}
public:
	explicit editor_info_callable(const hex_tile& tile)
		: tile_(const_cast<hex_tile*>(&tile))
	{}
};

variant hex_tile::get_value(const std::string& key) const
{
	if(key == "variations") {
	} else if(key == "transitions") {
		return variant(new transition_callable(*this));
	} else if(key == "type") {
		return variant(type());
	} else if(key == "name") {
		return variant(name());
	} else if(key == "editor_info") {
		return variant(new editor_info_callable(*this));
	}
	return variant();
}

void hex_tile::set_value(const std::string& key, const variant& value)
{
}

variant hex_tile::write() const
{
	// XXX todo
	return variant();
}

basic_hex_tile_ptr hex_tile::get_single_tile()
{
	// Select a tile from among the variations.
	ASSERT_LOG(!variations_.empty(), "No tiles found! " << type());
	int roll = rand() % 100;
	foreach(const basic_hex_tile_ptr& htp, variations_) {
		if(roll < htp->chance()) {
			htp->get_texture();
			return htp;
		}
		roll -= htp->chance();
	}
	// Ideally this shouldn't happen, but we'll just return the front item if it does.
	variations_.front()->get_texture();
	return variations_.front();
}

void hex_tile::editor_info::draw(int x, int y) const
{
	point p(hex_map::get_pixel_pos_from_tile_pos(x,y));
	graphics::blit_texture(texture, p.x, p.y, image_rect.w(), image_rect.h(), 0.0f, 
		GLfloat(image_rect.x())/GLfloat(texture.width()),
		GLfloat(image_rect.y())/GLfloat(texture.height()),
		GLfloat(image_rect.x2())/GLfloat(texture.width()),
		GLfloat(image_rect.y2())/GLfloat(texture.height()));
}

}
