#ifndef LEVEL_OBJECT_HPP_INCLUDED
#define LEVEL_OBJECT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <vector>

#include "color_utils.hpp"
#include "draw_tile.hpp"
#include "formula_callable.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "variant.hpp"

class level_object;
typedef boost::intrusive_ptr<level_object> level_object_ptr;
typedef boost::intrusive_ptr<const level_object> const_level_object_ptr;

struct level_tile {
	level_tile() : object(NULL) {}
	bool is_solid(int x, int y) const;
	int x, y;
	int layer_from; //which zorder layer causes this tile to be built?
	int zorder;
	const level_object* object;
	bool face_right;
	bool draw_disabled;
};

struct level_tile_zorder_comparer {
	bool operator()(const level_tile& a, const level_tile& b) const {
		return a.zorder < b.zorder;
	}

	bool operator()(const level_tile& a, int b) const {
		return a.zorder < b;
	}

	bool operator()(int a, const level_tile& b) const {
		return a < b.zorder;
	}
};

struct level_tile_pos_comparer {
	bool operator()(const level_tile& a, const level_tile& b) const {
		return a.y < b.y || a.y == b.y && a.x < b.x;
	}

	bool operator()(const level_tile& a, const std::pair<int, int>& b) const {
		return a.y < b.second || a.y == b.second && a.x < b.first;
	}

	bool operator()(const std::pair<int, int>& a, const level_tile& b) const {
		return a.second < b.y || a.second == b.y && a.first < b.x;
	}
};

struct level_tile_zorder_pos_comparer {
	bool operator()(const level_tile& a, const level_tile& b) const {
		return a.zorder < b.zorder || a.zorder == b.zorder && a.y < b.y || a.zorder == b.zorder && a.y == b.y && a.x < b.x;
	}
};

struct level_tile_y_pos_comparer {
        bool operator()(const level_tile& a, int b) const {
                return a.y < b;
        }
 
        bool operator()(int a, const level_tile& b) const {
                return a < b.y;
        }
 
        bool operator()(const level_tile& a, const level_tile& b) const {
                return a.y < b.y;
        }
};

//utility which sets the palette for objects loaded within a scope
struct palette_scope {
	explicit palette_scope(const std::string& str);
	~palette_scope();	

	unsigned int original_value;
};

class level_object : public game_logic::formula_callable {
public:
	static std::vector<const_level_object_ptr> all();
	static level_tile build_tile(variant node);
	static void write_compiled();

	static void set_current_palette(unsigned int palette);
	explicit level_object(variant node);
	~level_object();

	int width() const;
	int height() const;
	bool is_passthrough() const { return passthrough_; }
	bool is_solid(int x, int y) const;
	bool flipped() const { return flip_; }
	bool has_solid() const { return !solid_.empty(); }
	bool all_solid() const { return all_solid_; }
	const std::string& id() const { return id_; }
	void set_id(const std::string& id) { id_ = id; }
	const std::string& info() const { return info_; }
	int friction() const { return friction_; }
	int traction() const { return traction_; }
	int damage() const { return damage_; }
	const graphics::texture& texture() const { return t_; }
	static void queue_draw(graphics::blit_queue& q, const level_tile& t);
	static int calculate_tile_corners(tile_corner* result, const level_tile& t);

	bool is_opaque() const { return opaque_; }
	bool calculate_opaque() const;
	bool calculate_is_solid_color(graphics::color& col) const;

	bool calculate_draw_area();

	const graphics::color* solid_color() const { return solid_color_.get(); }

	//write the compiled index of this object. buf MUST point to a buf
	//of at least 4 chars.
	void write_compiled_index(char* buf) const;

	//reads an object from its compiled index. buf MUST point to a buf of
	//at least 3 chars.
	static const_level_object_ptr get_compiled(const char* buf);

	//only used when compiling: notifies the object it is used at the
	//given zorder.
	level_object_ptr record_zorder(int zorder) const;

private:
	variant get_value(const std::string& key) const;

	std::string id_;
	std::string image_;
	std::string info_;
	graphics::texture t_;
	std::vector<int> tiles_;
	std::vector<bool> solid_;
	bool all_solid_;
	bool passthrough_;
	bool flip_;
	int damage_;
	int friction_;
	int traction_;

	bool opaque_;

	rect draw_area_;

	boost::intrusive_ptr<graphics::color> solid_color_;

	int tile_index_;

	//only used when compiling: records all possible zorders for the object.
	mutable std::vector<int> zorders_;

	unsigned int palettes_recognized_;
	unsigned int current_palettes_;

	void set_palette(unsigned int palette);

	void get_palettes_used(std::vector<int>& v) const;
};

#endif
