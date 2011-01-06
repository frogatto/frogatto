#ifndef TEXTURE_FRAME_BUFFER_HPP_INCLUDED
#define TEXTURE_FRAME_BUFFER_HPP_INCLUDED

namespace texture_frame_buffer {

bool unsupported();
void init();

void set_as_current_texture();
inline int width() { return 128; }
inline int height() { return 128; }

struct render_scope {
	render_scope();
	~render_scope();
};

}

#endif
