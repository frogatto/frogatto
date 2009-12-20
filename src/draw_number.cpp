#include <string>

#include "draw_number.hpp"
#include "raster.hpp"
#include "texture.hpp"

namespace {
void blit_digit(const graphics::texture& t, char digit, int xpos, int ypos,
				double yoffset) {
	const int yadd = yoffset > 0.0 ? int(yoffset*14) : 0;
	const int ysub = yoffset < 0.0 ? int(yoffset*14) : 0;
	const int offset = (digit - '0') * 9;
	graphics::queue_blit_texture(t, xpos, ypos + yadd, 16, 14 + ysub - yadd,
					       (234.0 + offset)/400.0,
						   (63.0 - ysub/2.0)/104.0,
						   (242.0 + offset)/400.0,
						   (70.0 - yadd/2.0)/104.0);
	graphics::flush_blit_texture();
}
}

void draw_number(int number, int places, int xpos, int ypos)
{
	static const std::string Texture = "statusbar.png";
	graphics::texture t = graphics::texture::get(Texture);

	const int number_low = number/100;
	const int number_high = 1 + number/100;
	const int percent_high = number%100;
	const int percent_low = 100 - percent_high;

	char format_string[128];
	sprintf(format_string, "%%0%dd", places);
	char buf_high[128], buf_low[128];
	sprintf(buf_low, format_string, number_low);
	sprintf(buf_high, format_string, number_high);
	
	for(int n = 0; n != places; ++n, xpos += 18) {
		if(buf_low[n] == buf_high[n]) {
			blit_digit(t, buf_low[n], xpos, ypos, 0.0);
			continue;
		}

		if(percent_low > 0) {
			blit_digit(t, buf_low[n], xpos, ypos, (percent_low - 100)/100.0);
		}

		if(percent_high > 0) {
			blit_digit(t, buf_high[n], xpos, ypos, (100 - percent_high)/100.0);
		}
	}
}
