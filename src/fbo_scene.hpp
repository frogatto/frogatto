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
#ifndef FBO_SCENE_HPP_INCLUDED
#define FBO_SCENE_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

#include "entity.hpp"
#include "formula_callable.hpp"
#include "geometry.hpp"
#include "texture.hpp"

class texture_object : public game_logic::formula_callable
{
public:
	explicit texture_object(const graphics::texture& texture);
	const graphics::texture& texture() const { return texture_; }
private:
	variant get_value(const std::string& key) const;

	graphics::texture texture_;
};

graphics::texture render_fbo(const rect& area, const std::vector<entity_ptr> objects);

#endif
