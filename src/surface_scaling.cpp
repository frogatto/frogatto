#include <inttypes.h>
#include <iostream>
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
			
			if ((y > 0) && (y < input->h - 1) && (x > 0) && (x < input->w - 1)){
			//do additional eagle interpolation
				
			// Eagle is a pixel art scaling algorithm designed to smooth rough edges.  It works as follows:  First, it doubles the scale of the art, turning every source pixel into four destination pixels, just like nearest neighbor scaling would.
				
			// Then, to smooth things out, it conditionally alters each of these four pixels.  Each of the four destination pixels represents a quadrant of the source pixel, and likewise a direction pointing away from the center of the source pixel.  To choose if it 'smoothes an edge" in a given direction, it checks additional, adjacent source pixels in the direction represented by the quadrant.  If they're all the same color, it represents a diagonal slope of pixels, and the given quadrant pixel is filled in with the same color to smooth out the diagonal.
				
			// The following diagram illustrates first the scaling of a single input pixel into 4 output pixels (left side of diagram), and then the choices made to choose which color the output pixels are given (right side of diagram)
				
			//   first:        |Then 
			//   . . . --\ CC  |S T U  --\ 1 2
			//   . C . --/ CC  |V C W  --/ 3 4
			//   . . .         |X Y Z
			//                 | IF V==S==T => 1=S
			//                 | IF T==U==W => 2=U
			//                 | IF V==X==Y => 3=X
			//                 | IF W==Z==Y => 4=Z
						
						const int index_up_left = y*input->w + x - input->w - 1;
						const int index_up = y*input->w + x - input->w;
						const int index_up_right = y*input->w + x - input->w + 1;
						const int index_right = y*input->w + x + 1;
						const int index_down_right = y*input->w + x + input->w + 1;
						const int index_down = y*input->w + x + input->w;
						const int index_down_left = y*input->w + x + input->w - 1;
						const int index_left = y*input->w + x - 1;
						const int max_index = input->w*input->h;
						
						assert(index_down_left < max_index);
						assert(index_down_right < max_index);
						assert(index_down < max_index);
				
						if ( (in[index_up_left] == in[index_up]) && (in[index_up_left] == in[index_left]) ) {
								out[(y*2)*result->w + x*2] = in[index_up_left];
						}
						if ( (in[index_up_right] == in[index_up]) && (in[index_up_right] == in[index_right]) ) {
								out[(y*2)*result->w + x*2 + 1] = in[index_up_right];
						}
						if ( (in[index_down_left] == in[index_down]) && (in[index_down_left] == in[index_left]) ) {
								out[(y*2 + 1)*result->w + x*2] = in[index_down_left];
						}
						if ( (in[index_down_right] == in[index_down]) && (in[index_down_right] == in[index_right]) ) {
								out[(y*2 + 1)*result->w + x*2 + 1] = in[index_down_right];
						}
			}
		}
	}

	return result;
}

}
