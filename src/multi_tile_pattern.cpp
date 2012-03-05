#include <string.h>

#include "asserts.hpp"
#include "formatter.hpp"
#include "multi_tile_pattern.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

namespace {
//a pool of regular expressions. This makes sure that two regexes that
//are identical will point to the same place, and so we can easily
//test equality of regexes.
std::map<std::string, const boost::regex*> regex_pool;

std::deque<multi_tile_pattern>& patterns() {
	static std::deque<multi_tile_pattern> instance;
	return instance;
}

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

const std::deque<multi_tile_pattern>& multi_tile_pattern::get_all()
{
	return patterns();
}

void multi_tile_pattern::init(wml::const_node_ptr node)
{
	patterns().clear();
}

void multi_tile_pattern::load(wml::const_node_ptr node, const std::string& tile_id)
{
	wml::node::const_child_iterator p1 = node->begin_child("multi_tile_pattern");
	wml::node::const_child_iterator p2 = node->end_child("multi_tile_pattern");
	for(; p1 != p2; ++p1) {
		const wml::const_node_ptr& p = p1->second;
		patterns().push_back(multi_tile_pattern(p, tile_id));
	}
}

namespace {
bool compare_match_cell_by_run_length(const multi_tile_pattern::match_cell& a,
                                      const multi_tile_pattern::match_cell& b) {
	return a.run_length > b.run_length;
}

struct raw_cell {
	std::string regex;
	std::vector<std::string> map_to;
};

int parse_pattern(const std::string& pattern, std::vector<raw_cell>& out) {

	std::vector<std::string> lines = util::split(pattern, '\n', 0);
	const int height = lines.size();
	int width = -1;

	foreach(const std::string& line, lines) {
		std::vector<std::string> items = util::split(line, ',', util::STRIP_SPACES);
		if(width == -1) {
			width = items.size();
		}

		ASSERT_LOG(width == items.size(), "Inconsistent multi_tile_pattern size in pattern " << pattern);

		foreach(std::string item, items) {
			std::vector<const char*> arrows;
			const char* arrow = strstr(item.c_str(), "->");
			while(arrow) {
				arrows.push_back(arrow);
				arrow = strstr(arrow+2, "->");
			}

			raw_cell cell;

			if(arrows.empty() == false) {
				arrows.push_back(item.c_str() + item.size());
				for(int n = 0; n != arrows.size()-1; ++n) {
					std::string m(arrows[n]+2, arrows[n+1]);
					util::strip(m);
					cell.map_to.push_back(m);
				}

				item = std::string(item.c_str(), arrows.front());
			}

			util::strip(item);

			cell.regex = item;
			out.push_back(cell);
		}
	}

	return width;
}

}

multi_tile_pattern::multi_tile_pattern(wml::const_node_ptr const_node, const std::string& tile_id)
  : default_tile_id_(tile_id), id_(const_node->attr("id")), width_(-1), height_(-1), chance_(wml::get_int(const_node, "chance", 100))
{
	wml::node_ptr node = wml::deep_copy(const_node);

	//std::cerr << "INIT MTP: " << id_ << "\n";
	FOREACH_WML_CHILD(alternative_node, node, "alternative") {
		wml::node_ptr merged(new wml::node("multi_tile_pattern"));
		wml::merge_attr_over(node, merged);
		wml::merge_over(alternative_node, merged);
		alternatives_.push_back(boost::shared_ptr<multi_tile_pattern>(new multi_tile_pattern(merged, default_tile_id_)));
	}

	std::vector<raw_cell> cells;
	std::vector<raw_cell> cells_different_zorder;
	width_ = parse_pattern(node->attr("pattern"), cells);
	height_ = cells.size()/width_;

	cells_different_zorder.resize(cells.size());

	std::map<std::string, wml::node_ptr> base_nodes;

	FOREACH_WML_CHILD(range_node, const_node, "range") {
		const std::string from = range_node->attr("from");
		const std::string to = range_node->attr("to");

		const bool different_zorder = range_node->has_attr("zorder");

		ASSERT_LOG(from != "", "MTP " << id_ << " DOES NOT HAVE from SPECIFIED IN RANGE: " << wml::output(range_node));
		ASSERT_LOG(to != "", "MTP " << id_ << " DOES NOT HAVE to SPECIFIED IN RANGE");

		std::string tile_pos = range_node->attr("tiles");
		ASSERT_LOG(tile_pos.size() == 2, "In range for MTP " << id_ << " the tiles attribute is not in the correct format");

		int from_index = -1;
		int to_index = -1;
		for(int n = 0; n != cells.size(); ++n) {
			foreach(const std::string& m, cells[n].map_to) {
				if(m == from) {
					ASSERT_LOG(from_index == -1, "In multi_tile_pattern range specification for " << id_ << " the cell " << m << " is ambiguous since it appears multiple times");
					from_index = n;
				}

				if(m == to) {
					ASSERT_LOG(to_index == -1, "In multi_tile_pattern range specification for " << id_ << " the cell " << m << " is ambiguous since it appears multiple times");
					to_index = n;
				}
			}
		}

		ASSERT_LOG(from_index != -1, "In multi_tile_pattern range specification for " << id_ << " the cell '" << from << "' was not found");
		ASSERT_LOG(to_index != -1, "In multi_tile_pattern range specification for " << id_ << " the cell " << to << " was not found");
		ASSERT_LOG(to_index > from_index, "In multi_tile_pattern range specification for " << id_ << " the cell " << to << " comes before the cell " << from);

		const int from_x = from_index%width_;
		const int from_y = from_index/width_;
		const int to_x = to_index%width_;
		const int to_y = to_index/width_;

		char row = tile_pos[0];
		for(int y = from_y; y <= to_y; ++y) {
			char col = tile_pos[1];
			for(int x = from_x; x <= to_x; ++x) {
				const int index = y*width_ + x;
				ASSERT_LT(index, cells.size());
				foreach(std::string m, cells[index].map_to) {
					if(different_zorder) {
						m += "_zorder";
						cells_different_zorder[index].map_to.push_back(m);
					}

					wml::node_ptr base_node = wml::deep_copy(range_node);
					base_node->set_name(m);
					base_node->erase_attr("from");
					base_node->erase_attr("to");
					char buf[3] = {row, col, 0};
					base_node->set_attr("tiles", buf);

					if(node->get_child(m)) {
						ASSERT_LOG(base_nodes.count(m) == 0, "IN CALCULATING RANGE FOR MTP " << id_ << " TILE " << m << " APPEARS MULTIPLE TIMES");
						base_nodes[m] = base_node;
					} else {
						node->add_child(base_node);
					}
				}

				if(col == '9') {
					col = 'a';
				} else {
					++col;
				}
			}

			if(row == '9') {
				row = 'a';
			} else {
				++row;
			}
		}
	}

	if(const_node->get_child("range")) {
		//std::cerr << "PARSE MTP: " << wml::output(node) << "\n";
	}

	for(int n = 0; n != cells.size(); ++n) {
		foreach(const std::string& m, cells_different_zorder[n].map_to) {
			cells[n].map_to.push_back(m);
		}
	}

	std::map<std::string, level_object_ptr> objects;
	std::map<std::string, int> object_zorders;
	for(wml::node::const_all_child_iterator i = node->begin_children();
	    i != node->end_children(); ++i) {
		if((*i)->name() == "alternative" || (*i)->name() == "range") {
			continue;
		}

		wml::const_node_ptr obj_node = *i;
		if(base_nodes.count(obj_node->name())) {
			wml::merge_over(obj_node, base_nodes[obj_node->name()]);
			obj_node = base_nodes[obj_node->name()];
		}

		ASSERT_LOG(obj_node->attr("image").empty() == false, "object node has no image\n" << wml::output(obj_node) << "\n");

		level_object_ptr new_object(new level_object(obj_node));
		if(new_object->id().empty()) {
			new_object->set_id(default_tile_id_);
		}

		objects[obj_node->name()] = new_object;
		if(obj_node->has_attr("zorder")) {
			object_zorders[obj_node->name()] = wml::get_int(obj_node, "zorder");
		}
	}

	foreach(const raw_cell& cell, cells) {

		tile_info info;
		info.re = &get_regex_from_pool(cell.regex);

		foreach(const std::string& m, cell.map_to) {
			tile_entry entry;
			entry.zorder = INT_MIN;
			std::map<std::string, int>::const_iterator zorder_itor = object_zorders.find(m);
			if(zorder_itor != object_zorders.end()) {
				entry.zorder = zorder_itor->second;
			}

			if(cell.map_to.empty() == false) {
				entry.tile = objects[m];
			}

			info.tiles.push_back(entry);
		}

		tiles_.push_back(info);
	}

	ASSERT_EQ(tiles_.size(), width_*height_);

	for(int y = 0; y != height_; ++y) {
		int run_length = 0;
		for(int x = 0; x != width_; ++x) {
			if(x != 0) {
				if(tiles_[y*width_ + x].re == tiles_[y*width_ + x-1].re) {
					++run_length;
				} else {
					run_length = 0;
				}
			}

			match_cell cell;
			cell.loc = point(x, y);
			cell.run_length = run_length;
			try_order_.push_back(cell);
		}
	}
	
	//try_order_ dictates the order in which patterns will be tried to see
	//if an MTP matches.
	//
	//we want to order try_order_ to allow maximum chance of patterns being
	//excluded from matching quickly.
	std::sort(try_order_.begin(), try_order_.end(),
	          compare_match_cell_by_run_length);
	if(!try_order_.empty()) {
		for(int n = 0; n != try_order_.size(); ++n) {
			const match_cell& cell = try_order_[n];
			if(tiles_[cell.loc.y*width_ + cell.loc.x].re != &get_regex_from_pool("")) {
				if(n != 0) {
					match_cell c = try_order_[n];
					try_order_.erase(try_order_.begin() + n);
					try_order_.insert(try_order_.begin(), c);
				}
				break;
			}
		}

		if(try_order_.size() > 2 && tiles_[try_order_[0].loc.y*width_ + try_order_[0].loc.x].re == tiles_[try_order_[1].loc.y*width_ + try_order_[1].loc.x].re) {
			const boost::regex* re = tiles_[try_order_[0].loc.y*width_ + try_order_[0].loc.x].re;

			for(int n = 2; n != try_order_.size(); ++n) {
				const match_cell& cell = try_order_[n];
				if(tiles_[cell.loc.y*width_ + cell.loc.x].re != re) {
					match_cell c = try_order_[n];
					try_order_.erase(try_order_.begin() + n);
					try_order_.insert(try_order_.begin() + 1, c);
				}
			}
		}
	}
}

const multi_tile_pattern::tile_info& multi_tile_pattern::tile_at(int x, int y) const
{
	//asserts commented out for performance
//	ASSERT_GE(x, 0);
//	ASSERT_GE(y, 0);
//	ASSERT_LT(x, width_);
//	ASSERT_LT(y, height_);
//	ASSERT_EQ(tiles_.size(), width_*height_);

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
