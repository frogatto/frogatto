#include "formula_constants.hpp"
#include "raster.hpp"

namespace game_logic
{

variant get_constant(const std::string& id)
{
	if(id == "SCREEN_WIDTH") {
		return variant(graphics::screen_width());
	} else if(id == "SCREEN_HEIGHT") {
		return variant(graphics::screen_height());
	} else {
		return variant();
	}
}

}
