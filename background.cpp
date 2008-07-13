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
		node->set_attr("scale", formatter() << bg.scale);
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
	glVertex3i(0, 0, 0);
	glVertex3i(graphics::screen_width(), 0, 0);
	glColor3fv(bot_col);
	glVertex3i(graphics::screen_width(), height_, 0);
	glVertex3i(0, height_, 0);

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
	}

	const int border = rotation ? 100 : 0;

	const double scaling_factor = 1.0/double(bg.scale);
	const double xscale = double(bg.xscale)/100.0;
	const double xpos = (double(x - border)*xscale)/double(graphics::screen_width());
	const double xpos2 = xpos + scaling_factor * (1.0 + double(border*2)/double(graphics::screen_width()));
	const double yscale = double(bg.yscale)/100.0;
	double ypos = (double(y - bg.yoffset - border)*yscale)/double(graphics::screen_height());
	double ypos2 = ypos + scaling_factor * (1.0 + double(border*2)/double(graphics::screen_height()));

	if(ypos >= 1.0) {
		return;
	}

	if(ypos2 <= 0.0) {
		return;
	}

	int ydst = -border, height = graphics::screen_height() + border*2;
	if(ypos < 0.0) {
		double ratio_cut = -ypos/(ypos2 - ypos);
		ydst += height*ratio_cut;
		height -= height*ratio_cut;
		ypos = 0.0;
	}

	if(ypos2 > 1.0) {
		height -= graphics::screen_height()*((ypos2 - 1.0)/scaling_factor);
		ypos2 = 1.0;
	}

	glPushMatrix();
	glColor4f(1.0,1.0,1.0,1.0);
	graphics::blit_texture(bg.texture, -border, ydst, graphics::screen_width() + border*2, height, 0.0, xpos, ypos, xpos2, ypos2);
	glPopMatrix();
}
