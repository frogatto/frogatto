#ifndef TEXTURE_FRAME_BUFFER_HPP_INCLUDED
#define TEXTURE_FRAME_BUFFER_HPP_INCLUDED

namespace texture_frame_buffer {

bool unsupported();
void init(int width=128, int height=128);

void set_as_current_texture();
int width();
int height();

struct render_scope {
	render_scope();
	~render_scope();
};

}

#endif
