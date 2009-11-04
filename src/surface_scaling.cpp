#include <inttypes.h>
#include <iostream>
#include <cassert>

#include "surface_cache.hpp"
#include "surface.hpp"
#include "unit_test.hpp"

namespace graphics {

uint32_t interpolate_pixels (uint32_t sourcePixelOne, uint32_t sourcePixelTwo){
	//Take the average of the colors in two pixels.  Each channel needs to be mixed separately.
	
	uint32_t resultPixel;
	uint8_t* result_pixel_color_channel = reinterpret_cast<uint8_t*>(&resultPixel);
	uint8_t* pixel_one_channel = reinterpret_cast<uint8_t*>(&sourcePixelOne);
	uint8_t* pixel_two_channel = reinterpret_cast<uint8_t*>(&sourcePixelTwo);
	
	for ( int color = 0; color < 4; ++color) { 
		result_pixel_color_channel[color] = (pixel_one_channel[color] + pixel_two_channel[color]) / 2;
	}
	return resultPixel;
}

uint32_t interpolate_pixels (uint32_t sourcePixelOne, uint32_t sourcePixelTwo, uint32_t sourcePixelThree, uint32_t sourcePixelFour){
	//Same as above, but with four pixels instead of two.  The implementation was unnecessarily made different, solely for the programmer's edification.

	union PixelUnion {
		uint32_t value;
		uint8_t rgba[4];
	};
	
	PixelUnion resultPixel, pixel_one, pixel_two, pixel_three, pixel_four;
	pixel_one.value = sourcePixelOne;
	pixel_two.value = sourcePixelTwo;
	pixel_three.value = sourcePixelThree;
	pixel_four.value = sourcePixelFour;
	

	for ( int color = 0; color < 4; ++color) { 
		resultPixel.rgba[color] = (pixel_one.rgba[color] + pixel_two.rgba[color] + pixel_three.rgba[color] + pixel_four.rgba[color]) / 4;
	}
	return resultPixel.value;
}
	
int calculate_difference(uint32_t sourcePixelOne, uint32_t sourcePixelTwo, uint32_t sourcePixelThree, uint32_t sourcePixelFour){
	//determines which of pixels one and two, is more different from pixels three and four
	return ((sourcePixelOne != sourcePixelThree || sourcePixelOne != sourcePixelFour) - (sourcePixelTwo != sourcePixelThree || sourcePixelTwo != sourcePixelFour));
}

	
	
	
surface scale_surface_eagle(surface input) {
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
			
			if ((y > 0) && (y < input->h - 2) && (x > 0) && (x < input->w - 2)){
				
				
			//do additional 2xSaI interpolation
				
			//  2xSai works on a square group of sixteen pixels, rather than the square group of nine that Eagle works on.  In Eagle, the current pixel being upsized is the one in the middle of this square of nine; in 2xSai, the current pixel is the one in the upper-left of the middle four.  The other pixels in this group are all input pixels that are being examined to determine if we have an edge to smooth out.
			
			//  X X X X  |  0  1  2  3
			//  X * X X  |  4  5  6  7
			//  X X X X  |  8  9  10 11
			//  X X X X  |  12 13 14 15
						const int px[4][4] = {{	//[x][y]
							y*input->w + x - input->w - 1,
							y*input->w + x - input->w,
							y*input->w + x - input->w + 1,
							y*input->w + x - input->w + 2},
							{y*input->w + x - 1,
							y*input->w + x,
							y*input->w + x + 1,
							y*input->w + x + 2},
							{y*input->w + x + input->w - 1,
							y*input->w + x + input->w,
							y*input->w + x + input->w + 1,
							y*input->w + x + input->w + 2},
							{y*input->w + x + 2*input->w - 1,
							y*input->w + x + 2*input->w,
							y*input->w + x + 2*input->w + 1,
							y*input->w + x + 2*input->w + 2}};
						
						//these are the four output pixels corresponding to the one input pixel
						const int out_index_upper_left = (y*2)*result->w + x*2;
						const int out_index_upper_right = (y*2)*result->w + x*2 + 1;
						const int out_index_lower_left = (y*2 + 1)*result->w + x*2;
						const int out_index_lower_right = (y*2 + 1)*result->w + x*2 + 1;

						//make sure we're not going out-of-bounds
						const int max_index = input->w*input->h;
						assert(px[0][2] < max_index);
						assert(px[0][3] < max_index);
						assert(px[1][2] < max_index);
						assert(px[1][3] < max_index);
						assert(px[2][0] < max_index);
						assert(px[2][1] < max_index);
						assert(px[2][2] < max_index);
						assert(px[2][3] < max_index);
						assert(px[3][0] < max_index);
						assert(px[3][1] < max_index);
						assert(px[3][2] < max_index);
						assert(px[3][3] < max_index);

						if ( (in[px[1][1]] == in[px[2][2]]) && (in[px[1][2]] != in[px[2][1]]) ) {
							if ( ((in[px[1][1]] == in[px[0][1]]) && (in[px[1][2]] == in[px[2][3]])) || ((in[px[1][1]] == in[px[2][1]]) && (in[px[1][1]] == in[px[0][2]]) && (in[px[1][2]] != in[px[0][1]]) && (in[px[1][2]] == in[px[0][3]]))){
								out[out_index_upper_right] = in[px[1][1]];
							}else{
								if( ! ((in[px[1][1]] == in[px[0][1]])) ){
									out[out_index_upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
								}
							}
							
							if ( ((in[px[1][1]] == in[px[1][0]]) && (in[px[2][1]] == in[px[3][2]])) || ((in[px[1][1]] == in[px[1][2]]) && (in[px[1][1]] == in[px[2][0]]) && (in[px[1][0]] != in[px[2][1]]) && (in[px[2][1]] == in[px[3][0]]))){
								out[out_index_lower_left] = in[px[1][1]];
							}else{
								if( ! ((in[px[1][1]] == in[px[1][0]])) ){
									out[out_index_lower_left] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
								}
							}
							out[out_index_lower_right] = in[px[1][1]];
						} else if ( (in[px[1][2]] == in[px[2][1]]) && (in[px[1][1]] != in[px[2][2]]) ) {
							if ( ((in[px[1][2]] == in[px[0][2]]) && (in[px[1][1]] == in[px[2][0]])) || ((in[px[1][2]] == in[px[0][1]]) && (in[px[1][2]] == in[px[2][2]]) && (in[px[1][2]] != in[px[0][2]]) && (in[px[1][1]] == in[px[0][0]]))){
								out[out_index_upper_right] = in[px[1][2]];
							}else{
								out[out_index_upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
							}
							
							if ( ((in[px[2][1]] == in[px[2][0]]) && (in[px[1][1]] == in[px[0][2]])) || ((in[px[2][1]] == in[px[1][0]]) && (in[px[2][1]] == in[px[2][2]]) && (in[px[1][1]] != in[px[2][0]]) && (in[px[1][1]] == in[px[0][0]]))){
								out[out_index_lower_left] = in[px[2][1]];
							}else{
								out[out_index_lower_left] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
							}
							out[out_index_lower_right] = in[px[1][2]];
							
						}else if ( (in[px[1][1]] == in[px[2][2]]) && (in[px[1][2]] == in[px[2][1]]) ) {
							// these are crossed diagonal pairs of pixels in the inner, center set of four.
							// if they're the same, then we weight them against a surrounding ring of pixels, and see which
							// pair is more different from the ring.  The ring is this asterisked set of pixels:
							//  X * * X
							//  * X X *
							//  * X X *
							//  X * * X
							if (in[px[1][1]] == in[px[1][2]]){
								out[out_index_lower_right] = in[px[1][1]];
								out[out_index_lower_left] = in[px[1][1]];
								out[out_index_upper_right] = in[px[1][1]];
								out[out_index_upper_left] = in[px[1][1]];

							} else {
								
								int difference_direction = 0;
								out[out_index_upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
								out[out_index_lower_left] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
								
								difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[0][1]],in[px[1][0]]);
								difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[0][2]],in[px[1][3]]);
								difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[3][2]],in[px[2][3]]);
								difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[2][0]],in[px[3][1]]);
								
								if (difference_direction > 0){
									out[out_index_lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[1][2]],in[px[1][2]]);
									out[out_index_upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[1][2]],in[px[1][2]]);
								}else if(difference_direction < 0){
									out[out_index_lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][2]]);
									out[out_index_upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][2]]);
								}else{
									out[out_index_lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[2][1]],in[px[2][2]]);
									out[out_index_upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[2][1]],in[px[2][2]]);
								}								
								
							}
						} else {
							if ( ( (in[px[1][1]] != in[px[2][1]]) || (in[px[1][1]] != in[px[0][1]]) ) && ( (in[px[0][2]] == in[px[1][2]]) && (in[px[1][2]] == in[px[2][2]]) ) ){
								out[out_index_upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][2]]);
								out[out_index_lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][2]]);
							}
							
							if ( ( (in[px[0][2]] == in[px[1][1]]) || (in[px[1][1]] == in[px[2][0]]) ) && ( (in[px[1][1]] != in[px[2][1]]) && (in[px[1][1]] != in[px[1][2]])) ){
								out[out_index_upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][2]]);
								out[out_index_lower_left] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[2][1]]);
								out[out_index_lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[1][1]],in[px[2][1]]);
							}
							
							if ( ((in[px[1][1]] == in[px[0][0]]) && (in[px[1][1]] == in[px[1][2]])) && ( (in[px[1][1]] != in[px[1][0]]) && (in[px[1][1]] != in[px[2][1]]) ) ){
								out[out_index_upper_left] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][0]]);
								out[out_index_lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[2][1]]);
								out[out_index_lower_left] = interpolate_pixels(in[px[1][0]],in[px[2][1]]);
							}
							
							
							if ( (in[px[1][1]] == in[px[2][1]]) && (in[px[1][1]] == in[px[0][2]]) && (in[px[1][2]] != in[px[0][1]]) && (in[px[1][2]] == in[px[0][3]]) ){
								 out[out_index_upper_right] = in[px[1][1]];
							} else if ( (in[px[1][2]] == in[px[0][1]]) && (in[px[1][2]] == in[px[2][2]]) && (in[px[1][1]] != in[px[0][2]]) && (in[px[1][1]] == in[px[0][0]]) ){
								 out[out_index_upper_right] = in[px[1][2]];
							} else {
								//out[out_index_upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
								//out[out_index_lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
							}
							
							if ( (in[px[1][1]] == in[px[1][2]]) && (in[px[1][1]] == in[px[2][0]]) && (in[px[1][0]] != in[px[2][1]]) && (in[px[2][1]] == in[px[3][0]]) ){
								out[out_index_lower_left] = in[px[1][1]];
							} else if ( (in[px[2][1]] == in[px[1][0]]) && (in[px[2][1]] == in[px[2][2]]) && (in[px[1][1]] != in[px[2][0]]) && (in[px[1][1]] == in[px[0][0]]) ){
								out[out_index_lower_left] = in[px[2][1]];
							} else {
								//out[out_index_lower_right] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
								//out[out_index_lower_left] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
							}
							//out[out_index_lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[2][1]],in[px[2][2]]);
							//out[out_index_upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[2][1]],in[px[2][2]]);
						}
							
			}
		}
	}

	return result;
}

BENCHMARK(surface_scaling)
{
	surface s(graphics::surface_cache::get("characters/frogatto-spritesheet1.png"));
	assert(s.get());

	surface target(SDL_CreateRGBSurface(SDL_SWSURFACE,s->w,s->h,32,SURFACE_MASK));
	SDL_BlitSurface(s.get(), NULL, target.get(), NULL);
	BENCHMARK_LOOP {
		scale_surface(target);
	}
}

}
