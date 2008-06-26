#include <boost/regex.hpp>
#include <iostream>
#include <sstream>

#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_function.hpp"
#include "string_utils.hpp"
#include "tile_map.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {

const int TileSize = 32;

struct is_whitespace {
	bool operator()(char c) const { return isspace(c); }
};

struct tile_pattern {
	explicit tile_pattern(wml::const_node_ptr node)
	  : tile(new level_object(node)), reverse(node->attr("reverse") != "no"),
	    empty(node->attr("empty") == "yes"),
		extended_downwards(false),
		rotate(wml::get_int(node, "rotate", 0)),
		filter_formula(game_logic::formula::create_optional_formula(node->attr("filter")))
	{
		variations.push_back(tile);

		pattern_str = node->attr("pattern");
		std::cerr << "tile pattern: {{{" << node->attr("pattern") << "}}}\n";
		std::string pattern_str = node->attr("pattern");
		pattern_str.erase(std::remove_if(pattern_str.begin(), pattern_str.end(), is_whitespace()), pattern_str.end());
		std::vector<std::string> patterns = util::split(pattern_str, ',', 0);
		for(int i = 0; i != 12; ++i) {
			pattern[i] = boost::regex(".*");
		}
		switch(patterns.size()) {
		case 1: {
			pattern[4] = boost::regex(patterns[0]);
			break;
		}
		case 3: {
			for(int i = 0; i != 3; ++i) {
				if(patterns[i].empty()) {
					patterns[i] = "^$";
				}
				pattern[3 + i] = boost::regex(patterns[i]);
			}
			break;
		}
		case 12:
			extended_downwards = true;
		case 9: {
			for(int i = 0; i != patterns.size(); ++i) {
				if(patterns[i].empty()) {
					patterns[i] = "^$";
				}
				pattern[i] = boost::regex(patterns[i]);
			}
			break;
		}

		default:
			std::cerr << "Illegal tile pattern: (((" << node->attr("pattern") << ")))\n";
			assert(false);
		}

		wml::node::const_child_iterator i1 = node->begin_child("variation");
		wml::node::const_child_iterator i2 = node->end_child("variation");
		while(i1 != i2) {
			variations.push_back(level_object_ptr(new level_object(i1->second)));
			++i1;
		}
	}
	boost::regex pattern[12];
	std::string pattern_str;
	level_object_ptr tile;
	std::vector<level_object_ptr> variations;
	bool reverse;
	bool empty;
	bool extended_downwards; //if we use all 12 patterns
	int rotate;

	game_logic::const_formula_ptr filter_formula;
};

std::vector<tile_pattern> patterns;

class filter_callable : public game_logic::formula_callable {
	const tile_map& m_;
	int x_, y_;
public:
	filter_callable(const tile_map& m, int x, int y) : m_(m), x_(x), y_(y)
	{}

	variant get_value(const std::string& key) const {
		if(key == "tiles") {
			return variant(&m_);
		} else if(key == "x") {
			return variant(x_);
		} else if(key == "y") {
			return variant(y_);
		} else {
			return variant();
		}
	}
};

using namespace game_logic;

class tile_at_function : public function_expression {
public:
	explicit tile_at_function(const args_list& args)
	  : function_expression("tile_at", args, 2)
	{}

private:
	variant execute(const formula_callable& variables) const {
		variant v = args()[0]->evaluate(variables);
		tile_map* m = v.convert_to<tile_map>();
		return variant(m->get_tile(args()[1]->evaluate(variables).as_int(),
		                           args()[2]->evaluate(variables).as_int()));
	}
};

class tile_map_function_symbol_table : public function_symbol_table
{
public:
	expression_ptr create_function(
	           const std::string& fn, const std::vector<expression_ptr>& args) const {
		if(fn == "tile_at") {
			return expression_ptr(new tile_at_function(args));
		} else {
			return function_symbol_table::create_function(fn, args);
		}
	}
};

}

void tile_map::init(wml::const_node_ptr node)
{
	patterns.clear();

	wml::node::const_child_iterator p1 = node->begin_child("tile_pattern");
	wml::node::const_child_iterator p2 = node->end_child("tile_pattern");
	for(; p1 != p2; ++p1) {
		const wml::const_node_ptr& p = p1->second;
		patterns.push_back(tile_pattern(p));
	}
}

tile_map::tile_map() : xpos_(0), ypos_(0), zorder_(0)
{
	//turn off reference counting
	add_ref();
}

tile_map::tile_map(wml::const_node_ptr node)
  : xpos_(wml::get_int(node, "x")), ypos_(wml::get_int(node, "y")),
    zorder_(wml::get_int(node, "zorder"))
{
	//turn off reference counting
	add_ref();
	{
	std::vector<std::string> lines = util::split(node->attr("variations"), '\n', 0);
	foreach(const std::string& line, lines) {
		variations_.resize(variations_.size()+1);
		std::vector<std::string> items = util::split(line, ',', util::STRIP_SPACES);
		foreach(const std::string& item, items) {
			int n = 0;
			if(item.empty() == false) {
				n = atoi(item.c_str());
			}

			variations_.back().push_back(n);
		}
	}
	}

	{
	std::vector<std::string> lines = util::split(node->attr("tiles"), '\n', 0);
	foreach(const std::string& line, lines) {
		map_.resize(map_.size()+1);
		std::vector<std::string> items = util::split(line, ',', util::STRIP_SPACES);
		foreach(std::string item, items) {
			tile_string str;
			std::fill(str.begin(), str.end(), '\0');
			if(item.size() > str.size()-1) {
				item.resize(str.size()-1);
			}

			std::copy(item.begin(), item.end(), str.begin());
			map_.back().push_back(str);
		}
	}
	}
}

wml::node_ptr tile_map::write() const
{
	wml::node_ptr res(new wml::node("tile_map"));
	res->set_attr("x", formatter() << xpos_);
	res->set_attr("y", formatter() << ypos_);
	res->set_attr("zorder", formatter() << zorder_);
	std::ostringstream tiles;
	foreach(const std::vector<tile_string>& row, map_) {
		tiles << "\n";
		for(int i = 0; i != row.size(); ++i) {
			if(i) {
				tiles << ",";
			}
			tiles << row[i].data();
		}
		
		if(row.empty()) {
			tiles << ",";
		}
	}

	std::ostringstream variations;
	foreach(const std::vector<int>& row, variations_) {
		variations << "\n";
		for(int i = 0; i != row.size(); ++i) {
			if(i) {
				variations << ",";
			}
			variations << row[i];
		}

		if(row.empty()) {
			variations << ",";
		}
	}

	res->set_attr("tiles", tiles.str());
	res->set_attr("variations", variations.str());
	return res;
}

const char* tile_map::get_tile(int y, int x) const
{
	if(x < 0 || y < 0 || y >= map_.size() || x >= map_[y].size()) {
		return "";
	}

	return map_[y][x].data();
}

namespace {
struct cstr_less {
	bool operator()(const char* a, const char* b) const {
		return strcmp(a, b) < 0;
	}
};

typedef std::map<const char*, std::vector<tile_pattern>, cstr_less> tile_pattern_cache_map;
struct tile_pattern_cache {
	tile_pattern_cache_map cache;
};

}

int tile_map::get_variations(int x, int y) const
{
	x -= xpos_/TileSize;
	y -= ypos_/TileSize;
	tile_pattern_cache cache;
	bool face_right = false;
	const tile_pattern* p = get_matching_pattern(x, y, cache, &face_right);
	if(p == NULL) {
	std::cerr << "get_variations(" << x << ", " << y << ") -> null\n";
		return 0;
	}

	std::cerr << "get_variations(" << x << ", " << y << ") -> " << p->variations.size() << "\n";

	return p->variations.size();
}

int tile_map::variation(int x, int y) const
{
	std::cerr << "VARIATION: " << x << "," << y << "/" << (y < variations_.size() ? variations_[y].size() : 0) << "," << variations_.size() << "\n";
	if(x < 0 || y < 0 || y >= variations_.size() || x >= variations_[y].size()) {
		return 0;
	}

	return variations_[y][x];
}

void tile_map::flip_variation(int x, int y)
{
	const int variations = get_variations(x, y);
	if(variations <= 1) {
		return;
	}

	x -= xpos_/TileSize;
	y -= ypos_/TileSize;

	if(y >= variations_.size()) {
		variations_.resize(y + 1);
	}

	std::vector<int>& row = variations_[y];
	if(x >= row.size()) {
		row.resize(x + 1);
	}

	row[x]++;
	if(row[x] >= variations) {
		row[x] = 0;
	}
}

void tile_map::build_tiles(std::vector<level_tile>* tiles,
                           const rect* r) const
{
	const int begin_time = SDL_GetTicks();
	std::cerr << "build tiles...\n";
	int width = 0;
	foreach(const std::vector<tile_string>& row, map_) {
		if(row.size() > width) {
			width = row.size();
		}
	}

	tile_pattern_cache cache;

	int ntiles = 0;
	for(int y = -1; y <= static_cast<int>(map_.size()); ++y) {
		const int ypos = ypos_ + y*TileSize;

		if(r && ypos < r->y() || r && ypos > r->y2()) {
			continue;
		}

		for(int x = -1; x <= width; ++x) {
			bool face_right = true;
			const tile_pattern* p = get_matching_pattern(x, y, cache, &face_right);
			const int xpos = xpos_ + x*TileSize;

			if(p == NULL) {
				continue;
			}

			if(r && xpos < r->x() || r && xpos > r->x2()) {
				continue;
			}

			++ntiles;

			level_tile t;
			t.x = xpos;
			t.y = ypos;
			t.zorder = zorder_;

			const int variation_num = variation(x, y);
			std::cerr << variation_num << "/" << p->variations.size() << "\n";
			assert(variation_num < p->variations.size());
			assert(p->variations[variation_num]);
			t.object = p->variations[variation_num];
			t.face_right = face_right;
			if(t.object->flipped()) {
				t.face_right = !t.face_right;
			}
			t.rotate = p->rotate;
			tiles->push_back(t);
		}
	}
	std::cerr << "done build tiles: " << ntiles << " " << (SDL_GetTicks() - begin_time) << "\n";
}

const tile_pattern* tile_map::get_matching_pattern(int x, int y, tile_pattern_cache& cache, bool* face_right) const
{

	if (!*get_tile(y, x) &&
	    !*get_tile(y-1, x) &&
		!*get_tile(y+1, x) &&
		!*get_tile(y, x-1) &&
		!*get_tile(y, x+1)) {
		return NULL;
	}

	const int xpos = xpos_ + x*TileSize;

	filter_callable callable(*this, x, y);

	const char* pattern[9] = {
	  get_tile(y-1,x-1), get_tile(y-1,x), get_tile(y-1,x+1),
	  get_tile(y-0,x-1), get_tile(y-0,x), get_tile(y-0,x+1),
	  get_tile(y+1,x-1), get_tile(y+1,x), get_tile(y+1,x+1)};

	tile_pattern_cache_map::iterator itor = cache.cache.find(pattern[4]);
	if(itor == cache.cache.end()) {
		itor = cache.cache.insert(std::pair<const char*,std::vector<tile_pattern> >(pattern[4], std::vector<tile_pattern>())).first;
		foreach(const tile_pattern& p, patterns) {
			if(!p.pattern[4].empty() && !boost::regex_match(pattern[4], pattern[4] + strlen(pattern[4]), p.pattern[4])) {
				continue;
			}

			itor->second.push_back(p);
		}
	}

	const std::vector<tile_pattern>& matching_patterns = itor->second;

	static const int reverse_indexes[9] = {
	  2, 1, 0,
	  5, 4, 3,
	  8, 7, 6,
	};
	/*
	std::string reverse_pattern[9] = {
	  get_tile(y-1,x+1), get_tile(y-1,x), get_tile(y-1,x-1),
	  get_tile(y-0,x+1), get_tile(y-0,x), get_tile(y-0,x-1),
	  get_tile(y+1,x+1), get_tile(y+1,x), get_tile(y+1,x-1)};
	  */

	foreach(const tile_pattern& p, matching_patterns) {
		if(p.filter_formula && p.filter_formula->execute(callable).as_bool() == false) {
			continue;
		}

		if(!p.pattern[7].empty() && !boost::regex_match(pattern[7], pattern[7] + strlen(pattern[7]), p.pattern[7])) {
			continue;
		}

		bool match = true;

		for(int i = 0; i != 9; ++i) {
			if(i == 4 || i == 7) {
				continue;
			}
			if(!boost::regex_match(pattern[i], pattern[i] + strlen(pattern[i]), p.pattern[i])) {
				match = false;
				break;
			}
		}

		if(match && p.extended_downwards) {
			const char* pattern[3] = {
			 get_tile(y+2,x-1), get_tile(y+2,x), get_tile(y+2,x+1)};
			for(int i = 0; i != 3; ++i) {
				if(!boost::regex_match(pattern[i], pattern[i] + strlen(pattern[i]), p.pattern[9 + i])) {
					match = false;
					break;
				}
			}
		}

		if(match) {
			if(p.empty) {
				return NULL;
			}

			*face_right = false;
			return &p;
		}

		if(p.reverse) {
			match = true;

			for(int i = 0; i != 9; ++i) {
				if(i == 4) {
					continue;
				}
				const int index = reverse_indexes[i];
				if(!p.pattern[4].empty() && !boost::regex_match(pattern[index], pattern[index] + strlen(pattern[index]), p.pattern[i])) {
					match = false;
					break;
				}
			}
		}

		if(match && p.extended_downwards) {
			const char* pattern[3] = {
			 get_tile(y+2,x+1), get_tile(y+2,x), get_tile(y+2,x-1)};
			for(int i = 0; i != 3; ++i) {
				if(!boost::regex_match(pattern[i], pattern[i] + strlen(pattern[i]), p.pattern[9 + i])) {
					match = false;
					break;
				}
			}
		}

		if(match) {
			if(p.empty) {
				return NULL;
			}

			*face_right = true;
			return &p;
		}
	}

	return NULL;
}

bool tile_map::set_tile(int xpos, int ypos, const std::string& str)
{
	if(str.empty() && (xpos < xpos_ || ypos < ypos_)) {
		return false;
	}

	tile_string empty_tile;
	std::fill(empty_tile.begin(), empty_tile.end(), '\0');
	if(xpos < xpos_) {
		const int add_tiles = abs((xpos - xpos_)/TileSize);
		std::vector<tile_string> insert(add_tiles, empty_tile);
		foreach(std::vector<tile_string>& row, map_) {
			row.insert(row.begin(), insert.begin(), insert.end());
		}

		xpos_ = xpos;
	}

	while(ypos < ypos_) {
		map_.insert(map_.begin(), std::vector<tile_string>());
		ypos_ -= TileSize;
	}

	const int x = (xpos - xpos_)/TileSize;
	const int y = (ypos - ypos_)/TileSize;
	assert(x >= 0);
	assert(y >= 0);
	if(map_.size() <= y) {
		map_.resize(y + 1);
	}

	std::vector<tile_string>& row = map_[y];

	if(row.size() > x && strcmp(row[x].data(), str.c_str()) == 0) {
		return false;
	}

	while(row.size() <= x) {
		row.push_back(empty_tile);
	}

	std::string::const_iterator end = str.end();
	if(str.size() > 3) {
		end = str.begin() + 3;
	}

	row[x] = empty_tile;
	std::copy(str.begin(), end, row[x].begin());
	return true;
}
