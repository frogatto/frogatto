#include <inttypes.h>

#include "surface.hpp"

namespace graphics {

surface scale_surface(surface input) {
	surface result(surface::create(input->w*2, input->h*2));

	const uint32_t* in = reinterpret_cast<const uint32_t*>(input->pixels);
	uint32_t* out = reinterpret_cast<uint32_t*>(result->pixels);
	for(int y = 0; y != input->h; ++y) {
		for(int x = 0; x != input->w; ++x) {
		  //do nearest neighbor interpolation
			out[(y*2)*result->w + x*2] = in[y*input->w + x];
			out[(y*2 + 1)*result->w + x*2] = in[y*input->w + x];
			out[(y*2 + 1)*result->w + x*2 + 1] = in[y*input->w + x];
			out[(y*2)*result->w + x*2 + 1] = in[y*input->w + x];
			
			/*if ((y > 0) && (y < input->h) && (x > 0) && (x < input->w)){
			//do additional eagle interpolation
						//code goes here
			
			}*/
		}
	}

	return result;
}

}
