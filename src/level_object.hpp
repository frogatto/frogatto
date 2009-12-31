#ifndef LEVEL_OBJECT_HPP_INCLUDED
#define LEVEL_OBJECT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <vector>

#include "color_utils.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "wml_node_fwd.hpp"

class level_object;
typedef boost::shared_ptr<level_object> level_object_ptr;
typedef boost::shared_ptr<const level_object> const_level_object_ptr;

struct level_tile {
	bool is_solid(int x, int y) const;
	int x, y;
	int zorder;
	const_level_object_ptr object;
	bool face_right;
	graphics::blit_queue blit_queue_buf;
	graphics::blit_queue* blit_queue;
	short blit_queue_begin, blit_queue_end;
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

class level_object {
public:
	static std::vector<const_level_object_ptr> all();
	static level_tile build_tile(wml::const_node_ptr node);
	static void write_compiled();
	explicit level_object(wml::const_node_ptr node);
	int width() const;
	int height() const;
	bool is_passthrough() const { return passthrough_; }
	bool is_solid(int x, int y) const;
	bool flipped() const { return flip_; }
	bool has_solid() const { return !solid_.empty(); }
	bool all_solid() const { return all_solid_; }
	const std::string& id() const { return id_; }
	int friction() const { return friction_; }
	int traction() const { return traction_; }
	int damage() const { return damage_; }
	const graphics::texture& texture() const { return t_; }
	static void draw(const level_tile& t);
	static void queue_draw(graphics::blit_queue& q, const level_tile& t);

	bool is_opaque() const { return opaque_; }
	bool calculate_opaque() const;
	bool calculate_is_solid_color(graphics::color& col) const;

	const graphics::color* solid_color() const { return solid_color_.get(); }

	//write the compiled index of this object. buf MUST point to a buf
	//of at least 4 chars.
	void write_compiled_index(char* buf) const;

	//reads an object from its compiled index. buf MUST point to a buf of
	//at least 3 chars.
	static const_level_object_ptr get_compiled(const char* buf);

private:
	std::string id_;
	graphics::texture t_;
	int width_;
	std::vector<std::vector<int> > tiles_;
	std::vector<bool> solid_;
	bool all_solid_;
	bool passthrough_;
	bool flip_;
	int damage_;
	int friction_;
	int traction_;

	bool opaque_;

	boost::intrusive_ptr<graphics::color> solid_color_;

	int tile_index_;
};

#endif
