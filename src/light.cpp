#include <math.h>

#include "custom_object.hpp"
#include "formatter.hpp"
#include "light.hpp"
#include "raster.hpp"
#include "variant_utils.hpp"

light_ptr light::create_light(const custom_object& obj, variant node)
{
	if(node["type"].as_string() == "circle") {
		return light_ptr(new circle_light(obj, node));
	} else {
		return light_ptr();
	}
}

light::light(const custom_object& obj) : obj_(obj)
{}

light::~light() {}

circle_light::circle_light(const custom_object& obj, variant node)
  : light(obj), center_(obj.midpoint()), radius_(node["radius"].as_int())
{}

circle_light::circle_light(const custom_object& obj, int radius)
  : light(obj), center_(obj.midpoint()), radius_(radius)
{}

variant light::get_value(const std::string& key) const
{
	return variant();
}

variant circle_light::write() const
{
	variant_builder res;
	res.add("type", "circle");
	res.add("radius", radius_);

	return res.build();
}

void circle_light::process()
{
	center_ = object().midpoint();
}

bool circle_light::on_screen(const rect& screen_area) const
{
	return true;
}

namespace {
	int fade_length = 64;
}

void circle_light::draw(const rect& screen_area, const unsigned char* color) const
{
	static std::vector<float> x_angles;
	static std::vector<float> y_angles;

	if(x_angles.empty()) {
		for(float angle = 0.0f; angle < 3.1459f*2.0f; angle += 0.2f) {
			x_angles.push_back(cos(angle));
			y_angles.push_back(sin(angle));
		}
	}

	static std::vector<GLfloat> varray;

	const int x = center_.x;
	const int y = center_.y;

	varray.clear();
	varray.push_back(x);
	varray.push_back(y);
	for(int n = 0; n != x_angles.size(); ++n) {
		const float xpos = x + radius_*x_angles[n];
		const float ypos = y + radius_*y_angles[n];
		varray.push_back(xpos);
		varray.push_back(ypos);
	}

	//repeat the first coordinate to complete the circle
	varray.push_back(varray[2]);
	varray.push_back(varray[3]);

	glColor4ub(color[0], color[1], color[2], 255);

#if defined(USE_GLES2)
	gles2::manager gles2_manager(gles2::get_simple_shader());
	gles2::active_shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
#else
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, &varray.front());
#endif
	glDrawArrays(GL_TRIANGLE_FAN, 0, varray.size()/2);

	varray.clear();
	for(int n = 0; n != x_angles.size(); ++n) {
		const float xpos = x + radius_*x_angles[n];
		const float ypos = y + radius_*y_angles[n];
		const float xpos2 = x + (radius_+fade_length)*x_angles[n];
		const float ypos2 = y + (radius_+fade_length)*y_angles[n];
		varray.push_back(xpos);
		varray.push_back(ypos);
		varray.push_back(xpos2);
		varray.push_back(ypos2);
	}

	varray.push_back(varray[0]);
	varray.push_back(varray[1]);
	varray.push_back(varray[2]);
	varray.push_back(varray[3]);

	//the color array always just alternates between full-alpha and no-alpha,
	//always white. So just expand it until it's as big as needed.
	static std::vector<GLbyte> carray;
	carray.clear();
	while(carray.size() < (x_angles.size()+1)*8) {
		carray.push_back(color[0]);
		carray.push_back(color[1]);
		carray.push_back(color[2]);
		carray.push_back(255);
		carray.push_back(color[0]);
		carray.push_back(color[1]);
		carray.push_back(color[2]);
		carray.push_back(0);
	}
#if defined(USE_GLES2)
	// May need to use a frgament shader implementing phong shading here.
	gles2::active_shader()->vertex_array(2, GL_FLOAT, 0, 0, &varray.front());
	gles2::active_shader()->color_array(4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &carray.front());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, varray.size()/2);
#else
	glEnableClientState(GL_COLOR_ARRAY);
	glShadeModel(GL_SMOOTH);

	glVertexPointer(2, GL_FLOAT, 0, &varray.front());
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, &carray.front());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, varray.size()/2);

	glDisableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
#endif

	glColor4ub(255, 255, 255, 255);
}

light_fade_length_setter::light_fade_length_setter(int value)
  : old_value_(fade_length)
{
	fade_length = value;
}

light_fade_length_setter::~light_fade_length_setter()
{
	fade_length = old_value_;
}
