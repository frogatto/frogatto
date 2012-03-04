#include <iostream>
#include <set>

#include "foreach.hpp"
#include "level_solid_map.hpp"

namespace {
void merge_surface_info(surface_info& a, const surface_info& b)
{
	a.friction = std::max<int>(a.friction, b.friction);
	a.traction = std::max<int>(a.traction, b.traction);
	a.damage = std::max<int>(a.damage, b.damage);
	if(b.info) {
		a.info = b.info;
	}
}
}

const std::string* surface_info::get_info_str(const std::string& key)
{
	static std::set<std::string> info_set;
	return &*info_set.insert(key).first;
}

level_solid_map::level_solid_map()
{
}

level_solid_map::level_solid_map(const level_solid_map& m)
{
}

level_solid_map& level_solid_map::operator=(const level_solid_map& m)
{
	return *this;
}

level_solid_map::~level_solid_map()
{
	clear();
}

tile_solid_info& level_solid_map::insert_or_find(const tile_pos& pos)
{
	tile_solid_info** result = insert_raw(pos);
	if(!*result) {
		*result = new tile_solid_info;
	}

	return **result;
}

tile_solid_info** level_solid_map::insert_raw(const tile_pos& pos)
{
	row* r = NULL;
	if(pos.second >= 0) {
		if(positive_rows_.size() <= pos.second) {
			positive_rows_.resize(pos.second + 1);
		}

		r = &positive_rows_[pos.second];
	} else {
		const int index = -(pos.second+1);
		if(negative_rows_.size() <= index) {
			negative_rows_.resize(index + 1);
		}

		r = &negative_rows_[index];
	}

	if(pos.first >= 0) {
		if(r->positive_cells.size() <= pos.first) {
			r->positive_cells.resize(pos.first + 1);
		}

		return &r->positive_cells[pos.first];
	} else {
		const int index = -(pos.first+1);
		if(r->negative_cells.size() <= index) {
			r->negative_cells.resize(index + 1);
		}

		return &r->negative_cells[index];
	}
}

const tile_solid_info* level_solid_map::find(const tile_pos& pos) const
{
	const row* r = NULL;
	if(pos.second >= 0) {
		if(pos.second < positive_rows_.size()) {
			r = &positive_rows_[pos.second];
		} else {
			return NULL;
		}
	} else {
		const int index = -(pos.second+1);
		if(index < negative_rows_.size()) {
			r = &negative_rows_[index];
		} else {
			return NULL;
		}
	}

	if(pos.first >= 0) {
		if(pos.first < r->positive_cells.size()) {
			return r->positive_cells[pos.first];
		} else {
			return NULL;
		}
	} else {
		const int index = -(pos.first+1);
		if(index < r->negative_cells.size()) {
			return r->negative_cells[index];
		} else {
			return NULL;
		}
	}
}

void level_solid_map::erase(const tile_pos& pos)
{
	tile_solid_info** info = insert_raw(pos);
	delete *info;
	*info = NULL;
}

void level_solid_map::clear()
{
	foreach(row& r, positive_rows_) {
		foreach(tile_solid_info* info, r.positive_cells) {
			delete info;
		}

		foreach(tile_solid_info* info, r.negative_cells) {
			delete info;
		}
	}

	foreach(row& r, negative_rows_) {
		foreach(tile_solid_info* info, r.positive_cells) {
			delete info;
		}

		foreach(tile_solid_info* info, r.negative_cells) {
			delete info;
		}
	}

	positive_rows_.clear();
	negative_rows_.clear();
}

void level_solid_map::merge(const level_solid_map& map, int xoffset, int yoffset)
{
	for(int n = 0; n != map.negative_rows_.size(); ++n) {
		for(int m = 0; m != map.negative_rows_[n].negative_cells.size(); ++m) {
			const tile_pos pos(-m - 1 + xoffset, -n - 1 + yoffset);
			tile_solid_info& dst = insert_or_find(pos);
			const tile_solid_info* src = map.negative_rows_[n].negative_cells[m];
			if(!src) {
				continue;
			}

			dst.all_solid = dst.all_solid || src->all_solid;
			merge_surface_info(dst.info, src->info);
			if(!dst.all_solid) {
				dst.bitmap = dst.bitmap | src->bitmap;
			}
		}

		for(int m = 0; m != map.negative_rows_[n].positive_cells.size(); ++m) {
			const tile_pos pos(m + xoffset, -n - 1 + yoffset);
			tile_solid_info& dst = insert_or_find(pos);
			const tile_solid_info* src = map.negative_rows_[n].positive_cells[m];
			if(!src) {
				continue;
			}

			dst.all_solid = dst.all_solid || src->all_solid;
			merge_surface_info(dst.info, src->info);
			if(!dst.all_solid) {
				dst.bitmap = dst.bitmap | src->bitmap;
			}
		}
	}

	for(int n = 0; n != map.positive_rows_.size(); ++n) {
		for(int m = 0; m != map.positive_rows_[n].negative_cells.size(); ++m) {
			const tile_pos pos(-m - 1 + xoffset, n + yoffset);
			tile_solid_info& dst = insert_or_find(pos);
			const tile_solid_info* src = map.positive_rows_[n].negative_cells[m];
			if(!src) {
				continue;
			}

			dst.all_solid = dst.all_solid || src->all_solid;
			merge_surface_info(dst.info, src->info);
			if(!dst.all_solid) {
				dst.bitmap = dst.bitmap | src->bitmap;
			}
		}

		for(int m = 0; m != map.positive_rows_[n].positive_cells.size(); ++m) {
			const tile_pos pos(m + xoffset, n + yoffset);
			const tile_solid_info* src = map.positive_rows_[n].positive_cells[m];
			if(!src) {
				continue;
			}

			tile_solid_info& dst = insert_or_find(pos);

			dst.all_solid = dst.all_solid || src->all_solid;
			merge_surface_info(dst.info, src->info);
			if(!dst.all_solid) {
				dst.bitmap = dst.bitmap | src->bitmap;
			}
		}
	}
}
