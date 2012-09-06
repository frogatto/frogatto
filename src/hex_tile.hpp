#pragma once
#ifndef HEX_TILE_CPP_INCLUDED
#define HEX_TILE_CPP_INCLUDED

#include <boost/intrusive_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>
#include <map>

#include "decimal.hpp"
#include "graphics.hpp"
#include "formula_callable.hpp"
#include "frame.hpp"
#include "hex_object_fwd.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "variant.hpp"

namespace hex {

	class hex_tile;

	class basic_hex_tile : public game_logic::formula_callable
	{
	public:
		explicit basic_hex_tile(variant node, hex_tile* owner);
		virtual ~basic_hex_tile();
		
		std::string type() const;
		int chance() const { return chance_; }
		int zorder() const { return zorder_; }
		hex_tile* owner() const { return owner_; }

		virtual void draw(int x, int y) const;
		virtual void get_texture();
		virtual variant write() const;

	protected:
		virtual variant get_value(const std::string&) const;
		virtual void set_value(const std::string& key, const variant& value);
	private:
		hex_tile* owner_;
		graphics::texture texture_;
		std::string image_;
		rect rect_;
		std::vector<variant> nodes_;

		boost::scoped_ptr<frame> frame_;
		mutable int cycle_;
		// Chance that we will use a variation instead of base pattern. 
		// 100 means we definitely will use a variation.
		int chance_;

		int zorder_;

		// Private default constructor and copy constructor to stop them
		// from being used.
		basic_hex_tile() {}
		basic_hex_tile(basic_hex_tile&) {}
	};

	typedef boost::intrusive_ptr<basic_hex_tile> basic_hex_tile_ptr;
	typedef boost::intrusive_ptr<const basic_hex_tile> const_basic_hex_tile_ptr;

	typedef std::map<std::string, std::vector<basic_hex_tile_ptr> > transition_map;

	class hex_tile : public game_logic::formula_callable
	{
	public:

		explicit hex_tile(const std::string& type, variant node);
		virtual ~hex_tile();
		virtual variant write() const;

		std::string type() const { return type_; }
		std::string name() const { return name_; }

		basic_hex_tile_ptr get_single_tile();
		transition_map* find_transition(const std::string& key);
		basic_hex_tile_ptr get_transition_tile(const std::string& key);

		variant get_transitions();
		basic_hex_tile_ptr best_terrain_match(hex_object_ptr hop,
			const std::string& terrain, 
			const std::vector<std::string>& matching, 
			const std::string& transition_type);
	protected:
		virtual variant get_value(const std::string&) const;
		virtual void set_value(const std::string& key, const variant& value);
	private:
		std::string type_;
		std::string name_;
		std::vector<basic_hex_tile_ptr> variations_;
		std::map<std::string, transition_map> transitions_;
	
		struct editor_info
		{
			std::string name;
			std::string image;
			mutable graphics::texture texture;
			std::string group;
			rect image_rect;
		} editor_info_;
	};

	typedef boost::intrusive_ptr<hex_tile> hex_tile_ptr;
	typedef boost::intrusive_ptr<const hex_tile> const_hex_tile_ptr;

}

#endif // HEX_TILE_CPP_INCLUDED
