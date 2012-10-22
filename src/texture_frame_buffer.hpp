#ifndef TEXTURE_FRAME_BUFFER_HPP_INCLUDED
#define TEXTURE_FRAME_BUFFER_HPP_INCLUDED

namespace texture_frame_buffer {

bool unsupported();
void init(int width=128, int height=128);
void switch_texture();

void set_as_current_texture();
int width();
int height();

void set_render_to_texture();
void set_render_to_screen();

struct render_scope {
	render_scope();
	~render_scope();
};

}

#endif
