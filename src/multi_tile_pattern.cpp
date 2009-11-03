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
	if(key.empty()) {
		static boost::regex res("^$");
		return res;
	}

	const boost::regex*& re = regex_pool[key];
	if(!re) {
		if(key.empty() == false && key[0] == '!') {
			//if the re starts with a ! we treat that as 'not matches'.
			//This is marked by setting the lowest bit in the pointer.
			re = new boost::regex(std::string(key.begin() + 1, key.end()));
			re = reinterpret_cast<const boost::regex*>(reinterpret_cast<intptr_t>(re)+1);
		} else {
			re = new boost::regex(key);
		}
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
	FOREACH_WML_CHILD(alternative_node, node, "alternative") {
		wml::node_ptr merged(new wml::node("multi_tile_pattern"));
		wml::merge_attr_over(node, merged);
		wml::merge_over(alternative_node, merged);
		alternatives_.push_back(boost::shared_ptr<multi_tile_pattern>(new multi_tile_pattern(merged)));
	}

	std::map<std::string, level_object_ptr> objects;
	std::map<std::string, int> object_zorders;
	for(wml::node::const_all_child_iterator i = node->begin_children();
	    i != node->end_children(); ++i) {
		if((*i)->name() == "alternative") {
			continue;
		}

		objects[(*i)->name()].reset(new level_object(*i));
		if((*i)->has_attr("zorder")) {
			object_zorders[(*i)->name()] = wml::get_int(*i, "zorder");
		}
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
			std::vector<const char*> arrows;
			const char* arrow = strstr(item.c_str(), "->");
			while(arrow) {
				arrows.push_back(arrow);
				arrow = strstr(arrow+2, "->");
			}

			std::vector<std::string> map_to;
			if(arrows.empty() == false) {
				arrows.push_back(item.c_str() + item.size());
				for(int n = 0; n != arrows.size()-1; ++n) {
					std::string m(arrows[n]+2, arrows[n+1]);
					util::strip(m);
					map_to.push_back(m);
				}

				item = std::string(item.c_str(), arrows.front());
			}

			util::strip(item);

			tile_info info;
			info.re = &get_regex_from_pool(item);

			foreach(const std::string& m, map_to) {
				tile_entry entry;
				entry.zorder = INT_MIN;
				std::map<std::string, int>::const_iterator zorder_itor = object_zorders.find(m);
				if(zorder_itor != object_zorders.end()) {
					entry.zorder = zorder_itor->second;
				}

				if(map_to.empty() == false) {
					entry.tile = objects[m];
				}

				info.tiles.push_back(entry);
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

const multi_tile_pattern& multi_tile_pattern::choose_random_alternative(int seed) const
{
	if(alternatives_.empty()) {
		return *this;
	}

	const int index = seed%(alternatives_.size() + 1);
	if(index == alternatives_.size()) {
		return *this;
	}

	return *alternatives_[index];
}
