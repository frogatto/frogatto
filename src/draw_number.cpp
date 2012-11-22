#include <string>

#include "draw_number.hpp"
#include "raster.hpp"
#include "texture.hpp"

namespace {

const int NumberWidth = 18;

void queue_blit_digit(graphics::blit_queue& q, const graphics::texture& t, char digit, int xpos, int ypos,
				double yoffset) {
	const int yadd = yoffset > 0.0 ? int(yoffset*14) : 0;
	const int ysub = yoffset < 0.0 ? int(yoffset*14) : 0;
	const int offset = (digit - '0') * 9;

	const int x = xpos;
	const int y = ypos + yadd;

	const int x2 = x + NumberWidth;
	const int y2 = y + 14 + ysub - yadd;

	const GLfloat u1 = t.translate_coord_x((234.0f + offset)/400.0f);
	const GLfloat v1 = t.translate_coord_y((63.0f - ysub/2.0f)/104.0f);
	const GLfloat u2 = t.translate_coord_x((242.0f + offset)/400.0f);
	const GLfloat v2 = t.translate_coord_y((70.0f - yadd/2.0f)/104.0f);

	q.add(x, y, u1, v1);
	q.add(x, y2, u1, v2);
	q.add(x2, y, u2, v1);
	q.add(x2, y2, u2, v2);
}

void blit_digit(const graphics::texture& t, char digit, int xpos, int ypos,
				double yoffset) {
	const int yadd = yoffset > 0.0 ? int(yoffset*14) : 0;
	const int ysub = yoffset < 0.0 ? int(yoffset*14) : 0;
	const int offset = (digit - '0') * 9;
	graphics::queue_blit_texture(t, xpos, ypos + yadd, 16, 14 + ysub - yadd,
					       (234.0f + offset)/400.0f,
						   (63.0f - ysub/2.0f)/104.0f,
						   (242.0f + offset)/400.0f,
						   (70.0f - yadd/2.0f)/104.0f);
	graphics::flush_blit_texture();
}

}

void queue_draw_number(graphics::blit_queue& q, int number, int places, int xpos, int ypos)
{
	static const std::string Texture = "statusbar.png";
	static const graphics::texture t = graphics::texture::get(Texture);

	q.set_texture(t.get_id());

	const int number_low = number/100;
	const int number_high = 1 + number/100;
	const int percent_high = number%100;
	const int percent_low = 100 - percent_high;

	char format_string[128];
	sprintf(format_string, "%%0%dd", places);
	char buf_high[128], buf_low[128];
	sprintf(buf_low, format_string, number_low);
	sprintf(buf_high, format_string, number_high);
	
	for(int n = 0; n != places; ++n, xpos += NumberWidth) {
		if(buf_low[n] == buf_high[n]) {
			queue_blit_digit(q, t, buf_low[n], xpos, ypos, 0.0);
			continue;
		}

		if(percent_low > 0) {
			queue_blit_digit(q, t, buf_low[n], xpos, ypos, (percent_low - 100)/100.0);
		}

		if(percent_high > 0) {
			queue_blit_digit(q, t, buf_high[n], xpos, ypos, (100 - percent_high)/100.0);
		}
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
