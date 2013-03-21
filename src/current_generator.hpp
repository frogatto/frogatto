/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CURRENT_GENERATOR_HPP_INCLUDED
#define CURRENT_GENERATOR_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

#include "formula_callable.hpp"
#include "geometry.hpp"
#include "variant.hpp"

typedef boost::intrusive_ptr<class current_generator> current_generator_ptr;

class current_generator : public game_logic::formula_callable
{
public:
	static current_generator_ptr create(variant node);

	virtual ~current_generator();

	virtual void generate(int center_x, int center_y, int target_x, int target_y, int target_mass, int* velocity_x, int* velocity_y) = 0;
	virtual variant write() const = 0;
private:
	virtual variant get_value(const std::string& key) const;
};

class radial_current_generator : public current_generator
{
public:
	radial_current_generator(int intensity, int radius);
	explicit radial_current_generator(variant node);
	virtual ~radial_current_generator() {}

	virtual void generate(int center_x, int center_y, int target_x, int target_y, int target_mass, int* velocity_x, int* velocity_y);
	virtual variant write() const;
private:
	int intensity_;
	int radius_;
};

class rect_current_generator : public current_generator
{
public:
	rect_current_generator(const rect& r, int xvelocity, int yvelocity, int strength);
	explicit rect_current_generator(variant node);
	virtual void generate(int center_x, int center_y, int target_x, int target_y, int target_mass, int* velocity_x, int* velocity_y);

	virtual variant write() const;
private:
	rect rect_;
	int xvelocity_, yvelocity_;
	int strength_;
};

#endif
