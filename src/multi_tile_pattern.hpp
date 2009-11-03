#ifndef MULTI_TILE_PATTERN_HPP_INCLUDED
#define MULTI_TILE_PATTERN_HPP_INCLUDED

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

#include "level_object.hpp"
#include "wml_node_fwd.hpp"

const boost::regex& get_regex_from_pool(const std::string& key);

class multi_tile_pattern
{
public:
	static const std::vector<multi_tile_pattern>& get_all();
	static void init(wml::const_node_ptr node);
	explicit multi_tile_pattern(wml::const_node_ptr node);

	struct tile_entry {
		level_object_ptr tile;
		int zorder;
	};

	struct tile_info {
		const boost::regex* re;
		std::vector<tile_entry> tiles;
	};

	const tile_info& tile_at(int x, int y) const;

	int width() const;
	int height() const;

	int chance() const { return chance_; }

	const multi_tile_pattern& choose_random_alternative(int seed) const;
private:
	std::string id_;
	std::vector<tile_info> tiles_;
	std::vector<boost::shared_ptr<multi_tile_pattern> > alternatives_;
	int width_, height_;
	int chance_;
};

#endif
