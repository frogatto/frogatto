#ifndef WATER_HPP_INCLUDED
#define WATER_HPP_INCLUDED

#include <vector>

#include "wml_node_fwd.hpp"

class water
{
public:
	explicit water(wml::const_node_ptr node);

	wml::node_ptr write() const;
	void draw(int begin_layer, int end_layer, int x, int y, int w, int h) const;

	int min_zorder() const;
	int max_zorder() const;
	
private:

	int level_;

	struct zorder_pos {
		int zorder;
		int offset;
	};

	std::vector<zorder_pos> positions_;

	enum { BadOffset = -100000 };
	int get_offset(int zorder) const;
};

#endif
