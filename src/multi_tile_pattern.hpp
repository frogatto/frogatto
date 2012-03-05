#ifndef MULTI_TILE_PATTERN_HPP_INCLUDED
#define MULTI_TILE_PATTERN_HPP_INCLUDED

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include <deque>
#include <string>
#include <vector>

#include "geometry.hpp"
#include "level_object.hpp"
#include "wml_node_fwd.hpp"

const boost::regex& get_regex_from_pool(const std::string& key);

class multi_tile_pattern
{
public:
	//all multi tile patterns loaded. This is a deque meaning callers can
	//save pointers to members, knowing they will never be destroyed.
	static const std::deque<multi_tile_pattern>& get_all();
	static void init(wml::const_node_ptr node);
	static void load(wml::const_node_ptr node, const std::string& tile_id);
	multi_tile_pattern(wml::const_node_ptr node, const std::string& tile_id);

	struct tile_entry {
		level_object_ptr tile;
		int zorder;
	};

	struct tile_info {
		const boost::regex* re;
		std::vector<tile_entry> tiles;
	};

	const std::string& id() const { return id_; }

	const tile_info& tile_at(int x, int y) const;

	int width() const;
	int height() const;

	int chance() const { return chance_; }

	const multi_tile_pattern& choose_random_alternative(int seed) const;

	struct match_cell {
		point loc;
		int run_length;
	};

	//the order to try matches in, optimized to eliminate things as soon
	//as we possibly can.
	const std::vector<match_cell>& try_order() const { return try_order_; }
private:
	std::string default_tile_id_;
	std::string id_;
	std::vector<tile_info> tiles_;
	std::vector<boost::shared_ptr<multi_tile_pattern> > alternatives_;
	std::vector<match_cell> try_order_;
	int width_, height_;
	int chance_;
};

#endif
