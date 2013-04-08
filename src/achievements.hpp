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
#ifndef ACHIEVEMENTS_HPP_INCLUDED
#define ACHIEVEMENTS_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include <string>

#include "variant.hpp"

class achievement;

typedef boost::shared_ptr<const achievement> achievement_ptr;

class achievement
{
public:
	static achievement_ptr get(const std::string& id);

	explicit achievement(variant node);

	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }
	const std::string& description() const { return description_; }
	int points() const { return points_; }
private:
	std::string id_, name_, description_;
	int points_;
};

bool attain_achievement(const std::string& id);

#endif
