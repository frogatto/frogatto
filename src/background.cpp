#include <iostream>

#include "background.hpp"
#include "color_utils.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "level.hpp"
#include "raster.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

background::background(const wml::const_node_ptr& node)
{
	top_ = string_to_color(node->attr("top"));
	bot_ = string_to_color(node->attr("bottom"));
	width_ = wml::get_int(node, "width");
	height_ = wml::get_int(node, "height");

	wml::node::const_child_iterator i1 = node->begin_child("layer");
	wml::node::const_child_iterator i2 = node->end_child("layer");
	for(; i1 != i2; ++i1) {
		layer bg;
		bg.image = (*i1->second)["image"];
		bg.image_formula = i1->second->attr("image_formula");
		bg.xscale = wml::get_int(i1->second, "xscale", 100);
		bg.yscale = wml::get_int(i1->second, "yscale", 100);
		bg.yoffset = wml::get_int(i1->second, "yoffset", 0);
		bg.scale = wml::get_int(i1->second, "scale", 1);
		if(bg.scale < 1) {
			bg.scale = 1;
		}

		std::fill(bg.color, bg.color + 4, 0.0);
		bg.color[0] = wml::get_attr<GLfloat>(i1->second, "red", 1.0);
		bg.color[1] = wml::get_attr<GLfloat>(i1->second, "green", 1.0);
		bg.color[2] = wml::get_attr<GLfloat>(i1->second, "blue", 1.0);
		bg.color[3] = wml::get_attr<GLfloat>(i1->second, "alpha", 1.0);

		bg.y1 = wml::get_attr<int>(i1->second, "y1");
		bg.y2 = wml::get_attr<int>(i1->second, "y2");
		layers_.push_back(bg);
	}
}

wml::node_ptr background::write() const
{
	wml::node_ptr res(new wml::node("background"));
	char buf[128];
	sprintf(buf, "%02x%02x%02x", top_.r, top_.g, top_.b);
	res->set_attr("top", buf);
	sprintf(buf, "%02x%02x%02x", bot_.r, bot_.g, bot_.b);
	res->set_attr("bottom", buf);
	res->set_attr("width", formatter() << width_);
	res->set_attr("height", formatter() << height_);

	foreach(const layer& bg, layers_) {
		wml::node_ptr node(new wml::node("layer"));
		node->set_attr("image", bg.image);
		node->set_attr("xscale", formatter() << bg.xscale);
		node->set_attr("yscale", formatter() << bg.yscale);
		node->set_attr("yoffset", formatter() << bg.yoffset);
		node->set_attr("y1", formatter() << bg.y1);
		node->set_attr("y2", formatter() << bg.y2);
		node->set_attr("scale", formatter() << bg.scale);
		node->set_attr("red", formatter() << bg.color[0]);
		node->set_attr("green", formatter() << bg.color[1]);
		node->set_attr("blue", formatter() << bg.color[2]);
		node->set_attr("alpha", formatter() << bg.color[3]);
		res->add_child(node);
	}
	return res;
}

void background::draw(double xpos, double ypos, int rotation) const
{
	const int max_x = width_ - graphics::screen_width();
	const int max_y = height_ - graphics::screen_height();

	double x = xpos/max_x;
	double y = ypos/max_y;

	double y2 = y + double(graphics::screen_height())/double(height_);
	
	x = std::min<double>(1.0, std::max<double>(0.0, x));
	y = std::min<double>(1.0, std::max<double>(0.0, y));
	const GLfloat top_col[] = {top_.r/255.0,
	                           top_.g/255.0,
	                           top_.b/255.0};
	const GLfloat bot_col[] = {bot_.r/255.0,
	                           bot_.g/255.0,
	                           bot_.b/255.0};

	glShadeModel(GL_SMOOTH);
	glDisable(GL_TEXTURE_2D);
	glPushMatrix();
	glBegin(GL_POLYGON);

	glColor3fv(top_col);
	glVertex3i(xpos, ypos, 0);
	glVertex3i(xpos + graphics::screen_width(), ypos, 0);
	glColor3fv(bot_col);
	glVertex3i(xpos + graphics::screen_width(), ypos + height_, 0);
	glVertex3i(xpos, ypos + height_, 0);

	glEnd();

	if(rotation) {
		const int border = 100;
		glBegin(GL_POLYGON);
		glColor3fv(top_col);
		glVertex3i(-border, -border, 0);
		glVertex3i(graphics::screen_width() + border, -border, 0);
		glVertex3i(graphics::screen_width() + border, 0, 0);
		glVertex3i(-border, 0, 0);
		glEnd();

		glBegin(GL_POLYGON);
		glColor3fv(top_col);
		glVertex3i(-border, 0, 0);
		glVertex3i(graphics::screen_width() + border, 0, 0);
		glColor3fv(bot_col);
		glVertex3i(graphics::screen_width() + border, height_, 0);
		glVertex3i(-border, height_, 0);
		glEnd();
	}

	glColor3f(1.0,1.0,1.0);
	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);

	foreach(const layer& bg, layers_) {
		draw_layer(xpos, ypos, rotation, bg);
	}
}

void background::draw_layer(int x, int y, int rotation, const background::layer& bg) const
{
	if(!bg.texture.valid()) {
		bg.texture = graphics::texture::get(bg.image, bg.image_formula);
		if(bg.y2 == 0) {
			bg.y2 = bg.texture.height();
		}
	}

	const double ScaleImage = 2.0;
	const double xscale = double(bg.xscale)/100.0;
	const double xpos = int(double(x)*xscale)/double(bg.texture.width()*ScaleImage);
	const double xpos2 = xpos + double(graphics::screen_width())/(bg.texture.width()*ScaleImage);
	
	glPushMatrix();
	glColor4fv(bg.color);
	graphics::blit_texture(bg.texture, x, y + bg.yoffset*ScaleImage - (y*bg.yscale)/100,
	                       graphics::screen_width(),
					       (bg.y2 - bg.y1)*ScaleImage, 0.0, xpos,
						   double(bg.y1)/double(bg.texture.height()),
						   xpos2,
						   double(bg.y2)/double(bg.texture.height()));

	glColor4f(1.0,1.0,1.0,1.0);
	glPopMatrix();
}
#ifdef SDFLJADFA
void background::draw_layer(int x, int y, int rotation, const background::layer& bg) const
{
	if(!bg.texture.valid()) {
		bg.texture = graphics::texture::get(bg.image, bg.image_formula);
		if(bg.y2 == 0) {
			bg.y2 = bg.texture.height();
		}
	}

	const double ScaleImage = 2.0;

	const double xscale = double(bg.xscale)/100.0;
	const double xpos = int(double(x)*xscale)/double(bg.texture.width()*ScaleImage);
	const double xpos2 = xpos + double(graphics::screen_width())/(bg.texture.width()*ScaleImage);
	const double yscale = double(bg.yscale)/100.0;
	double ypos = double(-bg.yoffset)/double(bg.y2 - bg.y1) + int(double(y)*yscale)/double((bg.y2 - bg.y1)*ScaleImage);

	double ypos2 = ypos + double(graphics::screen_height())/((bg.y2 - bg.y1)*ScaleImage);

	if(ypos >= 1.0) {
		return;
	}

	if(ypos2 <= 0.0) {
		return;
	}

	int ydst = 0, height = graphics::screen_height();
	if(ypos < 0.0) {
		double ratio_cut = -ypos/(ypos2 - ypos);
		ydst += height*ratio_cut;
		height -= height*ratio_cut;
		ypos = 0.0;
	}

	if(ypos2 > 1.0) {
		double ratio_cut = (ypos2 - 1.0)/(ypos2 - ypos);
		height -= height*ratio_cut;
		ypos2 = 1.0;
	}

	//normalize ypos and ypos2 to the (y1,y2) range that the background is in.
	const double min_y = double(bg.y1)/double(bg.texture.height());
	const double max_y = double(bg.y2)/double(bg.texture.height());

	ypos = min_y + ypos*(max_y - min_y);
	ypos2 = min_y + ypos2*(max_y - min_y);

	if(bg.y2 == 185) {
	std::cerr << "ypos: " << ypos << " " << ypos2 << "\n";
	}

	glPushMatrix();
	glColor4fv(bg.color);
	graphics::blit_texture(bg.texture, 0, ydst, graphics::screen_width(), height, 0.0, xpos, ypos, xpos2, ypos2);

	//stretch the low bit of the background over any remaining screen area
/*	
	const int height_below = graphics::screen_height() - (ydst + height);
	std::cerr << "height below: " << height_below << "\n";
	if(height_below > 0) {
		graphics::blit_texture(bg.texture, 0, ydst + height - 1, graphics::screen_width(), height_below, 0.0, xpos, 0.98, xpos2, 1.0);
	}
	*/
	glColor4f(1.0,1.0,1.0,1.0);
	glPopMatrix();
}
#endif
