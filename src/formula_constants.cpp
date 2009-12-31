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
	} else if(id == "LOW_END_SYSTEM") {
#if TARGET_IPHONE_SIMULATOR
		return variant(1);
#else
		return variant(0);
#endif
	} else if(id == "HIGH_END_SYSTEM") {
		return variant(!get_constant("LOW_END_SYSTEM").as_bool());
	} else {
		return variant();
	}
}

}
