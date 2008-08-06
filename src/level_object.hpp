#ifndef LEVEL_OBJECT_HPP_INCLUDED
#define LEVEL_OBJECT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <vector>

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
	int rotate;
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
		return a.zorder < b.zorder || a.zorder == b.zorder && a.x < b.x || a.zorder == b.zorder && a.x == b.x && a.y < b.y;
	}
};

struct level_tile_x_pos_comparer {
	bool operator()(const level_tile& a, int b) const {
		return a.x < b;
	}

	bool operator()(int a, const level_tile& b) const {
		return a < b.x;
	}
};

class level_object {
public:
	static std::vector<const_level_object_ptr> all();
	static level_tile build_tile(wml::const_node_ptr node);
	static void init(wml::const_node_ptr node);
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
	int damage() const { return damage_; }
	const graphics::texture& texture() const { return t_; }
	static void draw(const level_tile& t);
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
};

#endif
