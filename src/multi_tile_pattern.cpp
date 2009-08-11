#include <string.h>

#include "asserts.hpp"
#include "formatter.hpp"
#include "multi_tile_pattern.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {
//a pool of regular expressions. This makes sure that two regexes that
//are identical will point to the same place, and so we can easily
//test equality of regexes.
std::map<std::string, const boost::regex*> regex_pool;

std::vector<multi_tile_pattern> patterns;

}

const boost::regex& get_regex_from_pool(const std::string& key)
{
	const boost::regex*& re = regex_pool[key];
	if(!re) {
		re = new boost::regex(key);
	}

	return *re;
}

const std::vector<multi_tile_pattern>& multi_tile_pattern::get_all()
{
	return patterns;
}

void multi_tile_pattern::init(wml::const_node_ptr node)
{
	patterns.clear();
	wml::node::const_child_iterator p1 = node->begin_child("multi_tile_pattern");
	wml::node::const_child_iterator p2 = node->end_child("multi_tile_pattern");
	for(; p1 != p2; ++p1) {
		const wml::const_node_ptr& p = p1->second;
		patterns.push_back(multi_tile_pattern(p));
	}
}

multi_tile_pattern::multi_tile_pattern(wml::const_node_ptr node)
  : id_(node->attr("id")), width_(-1), height_(-1), chance_(wml::get_int(node, "chance", 100))
{
	std::map<std::string, level_object_ptr> objects;
	for(wml::node::const_all_child_iterator i = node->begin_children();
	    i != node->end_children(); ++i) {
		objects[(*i)->name()].reset(new level_object(*i));
	}

	std::vector<std::string> lines = util::split(node->attr("pattern"), '\n', 0);
	height_ = lines.size();
	foreach(const std::string& line, lines) {
		std::vector<std::string> items = util::split(line, ',', util::STRIP_SPACES);
		if(width_ == -1) {
			width_ = items.size();
		}

		ASSERT_LOG(width_ == items.size(), "Inconsistent multi_tile_pattern size in pattern " << id_);

		foreach(std::string item, items) {
			std::string map_to;
			const char* arrow = strstr(item.c_str(), "->");
			if(arrow) {
				map_to = arrow+2;
				util::strip(map_to);
				item.resize(arrow - item.c_str());
			}

			util::strip(item);
			util::strip(map_to);

			tile_info info;
			info.re = &get_regex_from_pool(item);

			if(map_to.empty() == false) {
				info.tile = objects[map_to];
			}

			tiles_.push_back(info);
		}
	}

	ASSERT_EQ(tiles_.size(), width_*height_);
}

const multi_tile_pattern::tile_info& multi_tile_pattern::tile_at(int x, int y) const
{
	ASSERT_GE(x, 0);
	ASSERT_GE(y, 0);
	ASSERT_LT(x, width_);
	ASSERT_LT(y, height_);
	ASSERT_EQ(tiles_.size(), width_*height_);

	return tiles_[y*width_ + x];
}

int multi_tile_pattern::width() const
{
	return width_;
}

int multi_tile_pattern::height() const
{
	return height_;
}
