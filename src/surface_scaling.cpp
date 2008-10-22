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
			//   first:        |Then 
			//   . . . --\ CC  |S T U  --\ 1 2
			//   . C . --/ CC  |V C W  --/ 3 4
			//   . . .         |X Y Z
			//                 | IF V==S==T => 1=S
			//                 | IF T==U==W => 2=U
			//                 | IF V==X==Y => 3=X
			//                 | IF W==Z==Y => 4=Z
						
						if ( (in[y*input->w + x - input->w - 1] == in[y*input->w + x - input->w]) && 
									(in[y*input->w + x - input->w - 1] == in[y*input->w + x - 1]) ) {
						  //IF V==S==T => 1=S
								out[(y*2)*result->w + x*2] = in[y*input->w + x + 1];
						}
						if ( (in[y*input->w + x - input->w + 1] == in[y*input->w + x - input->w]) && 
									(in[y*input->w + x - input->w + 1] == in[y*input->w + x + 1]) ) {
						  //IF T==U==W => 2=U
								out[(y*2)*result->w + x*2 + 1] = in[y*input->w + x + 1];
						}
						if ( (in[y*input->w + x + input->w - 1] == in[y*input->w + x + input->w]) && 
									(in[y*input->w + x + input->w - 1] == in[y*input->w + x - 1]) ) {
						  //IF V==X==Y => 3=X
								out[(y*2)*result->w + x*2 + 1] = in[y*input->w + x + 1];
						}
						if ( (in[y*input->w + x + input->w + 1] == in[y*input->w + x + input->w]) && 
									(in[y*input->w + x + input->w + 1] == in[y*input->w + x + 1]) ) {
						  //IF W==Z==Y => 4=Z
								out[(y*2)*result->w + x*2 + 1] = in[y*input->w + x + 1];
						}
			}*/
		}
	}

	return result;
}

}
