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
