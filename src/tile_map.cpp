#include <boost/regex.hpp>
#include <iostream>
#include <math.h>
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

//a pool of regular expressions. This makes sure that two regexes that
//are identical will point to the same place, and so we can easily
//test equality of regexes.
std::map<std::string, const boost::regex*> regex_pool;

typedef std::map<const boost::regex*, bool> regex_match_map;
std::map<boost::array<char, 4>, regex_match_map> re_matches;
const boost::regex& get_regex_from_pool(const std::string& key)
{
	const boost::regex*& re = regex_pool[key];
	if(!re) {
		re = new boost::regex(key);
	}

	return *re;
}

bool match_regex(boost::array<char, 4> str, const boost::regex* re) {
	std::map<const boost::regex*, bool>& m = re_matches[str];
	std::map<const boost::regex*, bool>::const_iterator i = m.find(re);
	if(i != m.end()) {
		return i->second;
	}

	const bool match = boost::regex_match(str.data(), str.data() + strlen(str.data()), *re);
	m[re] = match;
	return match;
}

const int TileSize = 32;

struct is_whitespace {
	bool operator()(char c) const { return isspace(c); }
};

}

struct tile_pattern {
	explicit tile_pattern(wml::const_node_ptr node)
	  : tile(new level_object(node)), reverse(node->attr("reverse") != "no"),
	    empty(node->attr("empty") == "yes"),
		rotate(wml::get_int(node, "rotate", 0)),
		filter_formula(game_logic::formula::create_optional_formula(node->attr("filter")))
	{
		variations.push_back(tile);

		pattern_str = node->attr("pattern");
		std::cerr << "tile pattern: {{{" << node->attr("pattern") << "}}}\n";
		std::string pattern_str = node->attr("pattern");
		pattern_str.erase(std::remove_if(pattern_str.begin(), pattern_str.end(), is_whitespace()), pattern_str.end());
		std::vector<std::string> patterns = util::split(pattern_str, ',', 0);

		assert(!patterns.empty());

		//the main pattern is always the very middle one.
		int main_tile = patterns.size()/2;


		int width = wml::get_int(node, "pattern_width", sqrt(static_cast<float>(patterns.size())));
		int height = patterns.size()/width;

		int top = -height/2;
		int left = -width/2;

		//pattern with size 12 is a special case
		if(patterns.size() == 12 && !node->has_attr("pattern_width")) {
			width = 3;
			height = 4;
			top = -1;
			left = -1;
			main_tile = 4;
		}

		current_tile_pattern = &get_regex_from_pool(patterns[main_tile].empty() ? "^$" : patterns[main_tile]);

		for(int n = 0; n != patterns.size(); ++n) {
			if(n == main_tile) {
				continue;
			}

			const int x = left + n%width;
			const int y = top + n/width;
			surrounding_tiles.push_back(surrounding_tile(x, y, patterns[n]));
		}

		wml::node::const_child_iterator i1 = node->begin_child("variation");
		wml::node::const_child_iterator i2 = node->end_child("variation");
		while(i1 != i2) {
			variations.push_back(level_object_ptr(new level_object(i1->second)));
			++i1;
		}

		i1 = node->begin_child("tile");
		i2 = node->end_child("tile");
		while(i1 != i2) {
			std::cerr << "added object...\n";
			added_tile t;
			t.object = level_object_ptr(new level_object(i1->second));
			t.zorder = wml::get_int(i1->second, "zorder");
			added_tiles.push_back(t);
			++i1;
		}
	}

	const boost::regex* current_tile_pattern;

	struct surrounding_tile {
		surrounding_tile(int x, int y, const std::string& s)
		  : xoffset(x), yoffset(y), pattern(&get_regex_from_pool(s.empty() ? "^$" : s))
		{}
		int xoffset;
		int yoffset;
		const boost::regex* pattern;
	};

	std::vector<surrounding_tile> surrounding_tiles;

	std::string pattern_str;
	level_object_ptr tile;
	std::vector<level_object_ptr> variations;
	bool reverse;
	bool empty;
	int rotate;

	struct added_tile {
		level_object_ptr object;
		int zorder;
	};

	std::vector<added_tile> added_tiles;

	game_logic::const_formula_ptr filter_formula;
};

namespace {

std::vector<tile_pattern> patterns;
int current_patterns_version = 0;

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

	++current_patterns_version;
}

tile_map::tile_map() : xpos_(0), ypos_(0), x_speed_(100), y_speed_(100), zorder_(0), patterns_version_(-1)
{
	//turn off reference counting
	add_ref();

	//make an entry for the empty string.
	pattern_index_.push_back(pattern_index_entry());
}

tile_map::tile_map(wml::const_node_ptr node)
  : xpos_(wml::get_int(node, "x")), ypos_(wml::get_int(node, "y")),
	x_speed_(wml::get_int(node, "x_speed", 100)), y_speed_(wml::get_int(node, "y_speed", 100)),
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

	//make an entry for the empty string.
	pattern_index_.push_back(pattern_index_entry());

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

			int index_entry = 0;
			foreach(const pattern_index_entry& e, pattern_index_) {
				if(strcmp(e.str.data(), str.data()) == 0) {
					break;
				}

				++index_entry;
			}

			if(index_entry == pattern_index_.size()) {
				pattern_index_.push_back(pattern_index_entry());
				pattern_index_.back().str = str;
			}

			map_.back().push_back(index_entry);
		}
	}

	build_patterns();

	}
}

void tile_map::build_patterns()
{
	std::vector<const boost::regex*> all_regexes;

	patterns_version_ = current_patterns_version;
	const int begin_time = SDL_GetTicks();
	patterns_.clear();
	foreach(const tile_pattern& p, patterns) {
		std::vector<const boost::regex*> re;
		std::vector<const boost::regex*> accepted_re;
		if(!p.current_tile_pattern->empty()) {
			re.push_back(p.current_tile_pattern);
		}
		
		foreach(const tile_pattern::surrounding_tile& t, p.surrounding_tiles) {
			re.push_back(t.pattern);
		}

		int matches = 0;
		foreach(pattern_index_entry& e, pattern_index_) {
			foreach(const boost::regex*& regex, re) {
				if(regex && match_regex(e.str, regex)) {
					accepted_re.push_back(regex);
					regex = NULL;
					++matches;
					if(matches == re.size()) {
						break;
					}
				}
			}

			if(matches == re.size()) {
				break;
			}
		}

		if(matches == re.size()) {
			all_regexes.insert(all_regexes.end(), accepted_re.begin(), accepted_re.end());
			patterns_.push_back(&p);
		}
	}

	std::sort(all_regexes.begin(), all_regexes.end());
	all_regexes.erase(std::unique(all_regexes.begin(), all_regexes.end()), all_regexes.end());

	foreach(pattern_index_entry& e, pattern_index_) {
		e.matching_patterns.clear();

		foreach(const boost::regex* re, all_regexes) {
			if(match_regex(e.str, re)) {
				e.matching_patterns.push_back(re);
			}
		}
	}

	const int end_time = SDL_GetTicks();
	static int total_time = 0;
	total_time += (end_time - begin_time);
	std::cerr << "BUILD PATTERNS: " << (end_time - begin_time) << " " << total_time << "\n";
}

const std::vector<const tile_pattern*>& tile_map::get_patterns() const
{
	if(patterns_version_ != current_patterns_version) {
		const_cast<tile_map*>(this)->build_patterns();
	}

	return patterns_;
}

wml::node_ptr tile_map::write() const
{
	wml::node_ptr res(new wml::node("tile_map"));
	res->set_attr("x", formatter() << xpos_);
	res->set_attr("y", formatter() << ypos_);
	res->set_attr("x_speed", formatter() << x_speed_);
	res->set_attr("y_speed", formatter() << y_speed_);
	res->set_attr("zorder", formatter() << zorder_);
	std::ostringstream tiles;
	foreach(const std::vector<int>& row, map_) {
		tiles << "\n";
		for(int i = 0; i != row.size(); ++i) {
			if(i) {
				tiles << ",";
			}
			tiles << pattern_index_[row[i]].str.data();
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

const char* tile_map::get_tile_from_pixel_pos(int xpos, int ypos) const
{
	const int x = (xpos - xpos_)/TileSize;
	const int y = (ypos - ypos_)/TileSize;
	return get_tile(y, x);
}

const char* tile_map::get_tile(int y, int x) const
{
	if(x < 0 || y < 0 || y >= map_.size() || x >= map_[y].size()) {
		return "";
	}

	return pattern_index_[map_[y][x]].str.data();
}

const tile_map::pattern_index_entry& tile_map::get_tile_entry(int y, int x) const
{
	if(x < 0 || y < 0 || y >= map_.size() || x >= map_[y].size()) {
		return pattern_index_.front();
	}

	return pattern_index_[map_[y][x]];
}

namespace {
struct cstr_less {
	bool operator()(const char* a, const char* b) const {
		return strcmp(a, b) < 0;
	}
};

typedef std::map<const char*, std::vector<const tile_pattern*>, cstr_less> tile_pattern_cache_map;
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
	if(x < 0 || y < 0 || y >= variations_.size() || x >= variations_[y].size()) {
		return 0;
	}

	return variations_[y][x];
}

void tile_map::flip_variation(int x, int y, int delta)
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

	row[x] += delta;
	while(row[x] < 0) {
		row[x] += variations;
	}

	while(row[x] >= variations) {
		row[x] = row[x]%variations;
	}
}

void tile_map::build_tiles(std::vector<level_tile>* tiles,
                           const rect* r) const
{
	const int begin_time = SDL_GetTicks();
	std::cerr << "build tiles... " << patterns_.size() << "/" << patterns.size() << "\n";
	int width = 0;
	foreach(const std::vector<int>& row, map_) {
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

			int variation_num = variation(x, y);
			if(variation_num >= p->variations.size()) {
				variation_num = 0;
			}
			assert(p->variations[variation_num]);
			t.object = p->variations[variation_num];
			t.face_right = face_right;
			if(t.object->flipped()) {
				t.face_right = !t.face_right;
			}
			t.rotate = p->rotate;
			tiles->push_back(t);

			foreach(const tile_pattern::added_tile& a, p->added_tiles) {
				std::cerr << "added_tile\n";
				level_tile t;
				t.x = xpos;
				t.y = ypos;
				t.zorder = zorder_;
				if(a.zorder) {
					t.zorder = a.zorder;
				}
				t.object = a.object;
				t.face_right = face_right;
				if(t.object->flipped()) {
					t.face_right = !t.face_right;
				}

				t.rotate = p->rotate;
				tiles->push_back(t);
			}
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

	const char* current_tile = get_tile(y,x);

	//we build a cache of all of the patterns which have some chance of
	//matching the current tile.
	tile_pattern_cache_map::iterator itor = cache.cache.find(current_tile);
	if(itor == cache.cache.end()) {
		itor = cache.cache.insert(std::pair<const char*,std::vector<const tile_pattern*> >(current_tile, std::vector<const tile_pattern*>())).first;
		foreach(const tile_pattern* p, get_patterns()) {
			if(!p->current_tile_pattern->empty() && !boost::regex_match(current_tile, current_tile + strlen(current_tile), *p->current_tile_pattern)) {
				continue;
			}

			itor->second.push_back(&*p);
		}
	}

	const std::vector<const tile_pattern*>& matching_patterns = itor->second;

	foreach(const tile_pattern* ptr, matching_patterns) {
		const tile_pattern& p = *ptr;
		if(p.filter_formula && p.filter_formula->execute(callable).as_bool() == false) {
			continue;
		}

		bool match = true;
		foreach(const tile_pattern::surrounding_tile& t, p.surrounding_tiles) {
			const pattern_index_entry& entry = get_tile_entry(y + t.yoffset, x + t.xoffset);
			if(std::find(entry.matching_patterns.begin(), entry.matching_patterns.end(), t.pattern) == entry.matching_patterns.end()) {
				match = false;
				break;
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

			foreach(const tile_pattern::surrounding_tile& t, p.surrounding_tiles) {
				const pattern_index_entry& entry = get_tile_entry(y + t.yoffset, x - t.xoffset);
				if(std::find(entry.matching_patterns.begin(), entry.matching_patterns.end(), t.pattern) == entry.matching_patterns.end()) {
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
		std::vector<int> insert(add_tiles, get_pattern_index_entry(empty_tile));
		foreach(std::vector<int>& row, map_) {
			row.insert(row.begin(), insert.begin(), insert.end());
		}

		xpos_ = xpos;
	}

	while(ypos < ypos_) {
		map_.insert(map_.begin(), std::vector<int>());
		ypos_ -= TileSize;
	}

	const int x = (xpos - xpos_)/TileSize;
	const int y = (ypos - ypos_)/TileSize;
	assert(x >= 0);
	assert(y >= 0);
	if(map_.size() <= y) {
		map_.resize(y + 1);
	}

	std::vector<int>& row = map_[y];

	tile_string tstr;
	memset(&tstr[0], 0, tstr.size());
	std::string::const_iterator end = str.end();
	if(str.size() > 3) {
		end = str.begin() + 3;
	}

	std::copy(str.begin(), end, tstr.begin());

	const int index = get_pattern_index_entry(tstr);
	if(row.size() > x && row[x] == index) {
		std::cerr << "tile unchanged: '" << pattern_index_[index].str.data() << "'\n";
		return false;
	}

	const int empty_index = get_pattern_index_entry(empty_tile);
	while(row.size() <= x) {
		row.push_back(empty_index);
	}

		std::cerr << "tile changed: '" << pattern_index_[row[x]].str.data() << "' -> '" << pattern_index_[index].str.data() << "' -- " << row[x] << " -> " << index << "\n";
	row[x] = index;

	// clear out variations info
	if (y < variations_.size() && x < variations_[y].size()) {
		variations_[y][x] = 0;
	}
	return true;
}

int tile_map::get_pattern_index_entry(const tile_string& str) {
	int index = 0;
	foreach(pattern_index_entry& e, pattern_index_) {
		if(strcmp(e.str.data(), str.data()) == 0) {
			return index;
		}
		std::cerr << "tile changed ne " << index << ": (" << e.str.data() << ") (" << str.data() << ")\n";
		++index;
	}

	pattern_index_.push_back(pattern_index_entry());
	pattern_index_.back().str = str;
	build_patterns();
	return index;
}
