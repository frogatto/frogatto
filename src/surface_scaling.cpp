#include <boost/array.hpp>
#include <boost/cstdint.hpp>
#include <iostream>
#include <cassert>

#include "asserts.hpp"
#include "foreach.hpp"
#include "json_parser.hpp"
#include "string_utils.hpp"
#include "surface_cache.hpp"
#include "surface.hpp"
#include "unit_test.hpp"
#include "variant.hpp"

namespace graphics {

uint32_t interpolate_pixels (uint32_t sourcePixelOne, uint32_t sourcePixelTwo){
	//Take the average of the colors in two pixels.  Each channel needs to be mixed separately.
	
	uint32_t resultPixel;
	uint8_t* result_pixel_color_channel = reinterpret_cast<uint8_t*>(&resultPixel);
	uint8_t* pixel_one_channel = reinterpret_cast<uint8_t*>(&sourcePixelOne);
	uint8_t* pixel_two_channel = reinterpret_cast<uint8_t*>(&sourcePixelTwo);
	
	for ( int color = 0; color < 4; ++color) { 
		if(!color==3){
			result_pixel_color_channel[color] = (pixel_one_channel[color] + pixel_two_channel[color]) / 2;
		}else{
			//special handling for alpha channel                 
			result_pixel_color_channel[color] =  std::min(pixel_one_channel[color], pixel_two_channel[color]);
		}
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
		if(!color==3){
			resultPixel.rgba[color] = (pixel_one.rgba[color] + pixel_two.rgba[color] + pixel_three.rgba[color] + pixel_four.rgba[color]) / 4;
		}else{
			//special handling for alpha channel
			resultPixel.rgba[color] =  std::min(std::min(pixel_one.rgba[color], pixel_two.rgba[color]), std::min(pixel_three.rgba[color], pixel_four.rgba[color]));
		}
	}
	return resultPixel.value;
}
	
int calculate_difference(uint32_t sourcePixelOne, uint32_t sourcePixelTwo, uint32_t sourcePixelThree, uint32_t sourcePixelFour){
	//determines which of pixels one and two, is more different from pixels three and four
	// return values are between -1 and 1; positive values signify pixel one being more different, and vice versa
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
	
	
	
surface scale_surface_v1(surface input) {
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
				const int upper_left = (y*2)*result->w + x*2;
				const int upper_right = (y*2)*result->w + x*2 + 1;
				const int lower_left = (y*2 + 1)*result->w + x*2;
				const int lower_right = (y*2 + 1)*result->w + x*2 + 1;
				
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
						out[upper_right] = in[px[1][1]];
					}else{
						if( ! ((in[px[1][1]] == in[px[0][1]])) ){
							out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
						}
					}
					
					if ( ((in[px[1][1]] == in[px[1][0]]) && (in[px[2][1]] == in[px[3][2]])) || ((in[px[1][1]] == in[px[1][2]]) && (in[px[1][1]] == in[px[2][0]]) && (in[px[1][0]] != in[px[2][1]]) && (in[px[2][1]] == in[px[3][0]]))){
						out[lower_left] = in[px[1][1]];
					}else{
						if( ! ((in[px[1][1]] == in[px[1][0]])) ){
							out[lower_left] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
						}
					}
					out[lower_right] = in[px[1][1]];
				} else if ( (in[px[1][2]] == in[px[2][1]]) && (in[px[1][1]] != in[px[2][2]]) ) {
					if ( ((in[px[1][2]] == in[px[0][2]]) && (in[px[1][1]] == in[px[2][0]])) || ((in[px[1][2]] == in[px[0][1]]) && (in[px[1][2]] == in[px[2][2]]) && (in[px[1][2]] != in[px[0][2]]) && (in[px[1][1]] == in[px[0][0]]))){
						out[upper_right] = in[px[1][2]];
					}else{
						out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
					}
					
					if ( ((in[px[2][1]] == in[px[2][0]]) && (in[px[1][1]] == in[px[0][2]])) || ((in[px[2][1]] == in[px[1][0]]) && (in[px[2][1]] == in[px[2][2]]) && (in[px[1][1]] != in[px[2][0]]) && (in[px[1][1]] == in[px[0][0]]))){
						out[lower_left] = in[px[2][1]];
					}else{
						out[lower_left] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
					}
					out[lower_right] = in[px[1][2]];
					
				}else if ( (in[px[1][1]] == in[px[2][2]]) && (in[px[1][2]] == in[px[2][1]]) ) {
					// these are crossed diagonal pairs of pixels in the inner, center set of four.
					// if they're the same, then we weight them against a surrounding ring of pixels, and see which
					// pair is more different from the ring.  The ring is this asterisked set of pixels:
					//  X * * X
					//  * X X *
					//  * X X *
					//  X * * X
					if (in[px[1][1]] == in[px[1][2]]){
						out[lower_right] = in[px[1][1]];
						out[lower_left] = in[px[1][1]];
						out[upper_right] = in[px[1][1]];
						out[upper_left] = in[px[1][1]];
						
					} else {
						
						int difference_direction = 0;
						out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
						out[lower_left] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
						
						difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[0][1]],in[px[1][0]]);
						difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[0][2]],in[px[1][3]]);
						difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[3][2]],in[px[2][3]]);
						difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[2][0]],in[px[3][1]]);
						
						if (difference_direction > 0){
							out[lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[1][2]],in[px[1][2]]);
							out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[1][2]],in[px[1][2]]);
						}else if(difference_direction < 0){
							out[lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][2]]);
							out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][2]]);
						}else{
							out[lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[2][1]],in[px[2][2]]);
							out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[2][1]],in[px[2][2]]);
						}								
						
					}
				} else {
					if ( ( (in[px[1][1]] != in[px[2][1]]) || (in[px[1][1]] != in[px[0][1]]) ) && ( (in[px[0][2]] == in[px[1][2]]) && (in[px[1][2]] == in[px[2][2]]) ) ){
						out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][2]]);
						out[lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][2]]);
					}
					
					if ( ( (in[px[0][2]] == in[px[1][1]]) || (in[px[1][1]] == in[px[2][0]]) ) && ( (in[px[1][1]] != in[px[2][1]]) && (in[px[1][1]] != in[px[1][2]])) ){
						out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][2]]);
						out[lower_left] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[2][1]]);
						out[lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[1][1]],in[px[2][1]]);
					}
					
					if ( ((in[px[1][1]] == in[px[0][0]]) && (in[px[1][1]] == in[px[1][2]])) && ( (in[px[1][1]] != in[px[1][0]]) && (in[px[1][1]] != in[px[2][1]]) ) ){
						out[upper_left] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][0]]);
						out[lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[2][1]]);
						out[lower_left] = interpolate_pixels(in[px[1][0]],in[px[2][1]]);
					}
					
					
					if ( (in[px[1][1]] == in[px[2][1]]) && (in[px[1][1]] == in[px[0][2]]) && (in[px[1][2]] != in[px[0][1]]) && (in[px[1][2]] == in[px[0][3]]) ){
						out[upper_right] = in[px[1][1]];
					} else if ( (in[px[1][2]] == in[px[0][1]]) && (in[px[1][2]] == in[px[2][2]]) && (in[px[1][1]] != in[px[0][2]]) && (in[px[1][1]] == in[px[0][0]]) ){
						out[upper_right] = in[px[1][2]];
					} else {
						//out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
						//out[lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
					}
					
					if ( (in[px[1][1]] == in[px[1][2]]) && (in[px[1][1]] == in[px[2][0]]) && (in[px[1][0]] != in[px[2][1]]) && (in[px[2][1]] == in[px[3][0]]) ){
						out[lower_left] = in[px[1][1]];
					} else if ( (in[px[2][1]] == in[px[1][0]]) && (in[px[2][1]] == in[px[2][2]]) && (in[px[1][1]] != in[px[2][0]]) && (in[px[1][1]] == in[px[0][0]]) ){
						out[lower_left] = in[px[2][1]];
					} else {
						//out[lower_right] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
						//out[lower_left] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
					}
					//out[lower_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[2][1]],in[px[2][2]]);
					//out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[2][1]],in[px[2][2]]);
				}
				
			}
		}
	}
	
	return result;
}

surface scale_surface_newer(surface input) {
	surface result(surface::create(input->w*2, input->h*2));

	const uint32_t* in = reinterpret_cast<const uint32_t*>(input->pixels);
	uint32_t* out = reinterpret_cast<uint32_t*>(result->pixels);

	for(int y = 0; y != input->h; ++y) {
		for(int x = 0; x != input->w; ++x) {
			uint32_t& out0 = out[y*2*result->w + x*2];
			uint32_t& out1 = out[y*2*result->w + x*2+1];
			uint32_t& out2 = out[(y*2+1)*result->w + x*2];
			uint32_t& out3 = out[(y*2+1)*result->w + x*2+1];

			out0 = out1 = out2 = out3 = in[y*input->w + x];


			const uint32_t matrix[] = {
x >= 2 && y >= 2 ? in[(y-2)*input->w + x-2] : 0, x >= 1 && y >= 2 ? in[(y-2)*input->w + x-1] : 0, y >= 2 ? in[(y-2)*input->w + x] : 0, x < input->w-1 && y >= 2 ? in[(y-2)*input->w + x+1] : 0, x < input->w-2 && y >= 2 ? in[(y-2)*input->w + x+2] : 0,
x >= 2 && y >= 1 ? in[(y-1)*input->w + x-2] : 0, x >= 1 && y >= 2 ? in[(y-1)*input->w + x-1] : 0, y >= 1 ? in[(y-1)*input->w + x] : 0, x < input->w-1 && y >= 1 ? in[(y-1)*input->w + x+1] : 0, x < input->w-2 && y >= 1 ? in[(y-1)*input->w + x+2] : 0,
x >= 2 ? in[(y)*input->w + x-2] : 0, x >= 1 ? in[(y)*input->w + x-1] : 0, in[(y)*input->w + x], x < input->w-1 ? in[(y)*input->w + x+1] : 0, x < input->w-2 ? in[(y)*input->w + x+2] : 0,
x >= 2 && y < input->h-1 ? in[(y+1)*input->w + x-2] : 0, x >= 1 && y < input->h-1 ? in[(y+1)*input->w + x-1] : 0, y < input->h-1 ? in[(y+1)*input->w + x] : 0, x < input->w-1 && y < input->h-1 ? in[(y+1)*input->w + x+1] : 0, x < input->w-2 && y < input->h-1 ? in[(y+1)*input->w + x+2] : 0,
x >= 2 && y < input->h-2 ? in[(y+2)*input->w + x-2] : 0, x >= 1 && y < input->h-2 ? in[(y+2)*input->w + x-1] : 0, y < input->h-2 ? in[(y+2)*input->w + x] : 0, x < input->w-1 && y < input->h-2 ? in[(y+2)*input->w + x+1] : 0, x < input->w-2 && y < input->h-2 ? in[(y+2)*input->w + x+2] : 0,
			};

	union PixelUnion {
		uint32_t value;
		uint8_t rgba[4];
	};
#include "surface_scaling_generated.hpp"
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
						const int upper_left = (y*2)*result->w + x*2;
						const int upper_right = (y*2)*result->w + x*2 + 1;
						const int lower_left = (y*2 + 1)*result->w + x*2;
						const int lower_right = (y*2 + 1)*result->w + x*2 + 1;

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
						
						
						
						// The following blocks are a visual representation of the conditional right above them.  When I have multiple instances of the same number, such as two 1s, then those places are equal.  When I have two different numbers in the same block, then they *must* be inequal - e.g. the value at 2 does not equal the value at 1, and also does not equal the value at 3. When I have a number, and an alphanumeric character in a block, the character does not need to be different from the number, but must equal itself.
								//  X X X X 
								//  X X X X 
								//  X X X X 
								//  X X X X 

						if ( (in[px[1][1]] == in[px[2][2]]) && (in[px[1][2]] != in[px[2][1]]) ) {
								//  X X X X    
								//  X 1 2 X    
								//  X 3 1 X    
								//  X X X X    
							if ( ((in[px[1][1]] == in[px[0][1]]) && (in[px[1][2]] == in[px[2][3]])) || ((in[px[1][1]] == in[px[2][1]]) && (in[px[1][1]] == in[px[0][2]]) && (in[px[1][2]] != in[px[0][1]]) && (in[px[1][2]] == in[px[0][3]]))){
								   //  X 1 X X         X 2 1 1
								   //  X 1 A X         X 1 1 X
								   //  X X X A   or    X X X X
								   //  X X X X         X X X X
								out[upper_right] = in[px[1][1]];    
							}else{
								if( ! ((in[px[1][1]] == in[px[0][1]])) ){
									//  X 2 X X 
									//  X 1 X X 
									//  X X X X 
									//  X X X X 
									out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
								}
							}
							
							if ( ((in[px[1][1]] == in[px[1][0]]) && (in[px[2][1]] == in[px[3][2]])) || ((in[px[1][1]] == in[px[1][2]]) && (in[px[1][1]] == in[px[2][0]]) && (in[px[1][0]] != in[px[2][1]]) && (in[px[2][1]] == in[px[3][0]]))){
								//  X X X X           X X X X 
								//  1 1 X X           2 A A X 
								//  X A X X    or     A 3 X X 
								//  X X A X           X 3 X X 
								out[lower_left] = in[px[1][1]];
							}else{
								if( ! ((in[px[1][1]] == in[px[1][0]])) ){
										//  X 2 X X 
										//  X 1 X X 
										//  X X X X 
										//  X X X X 
									out[lower_left] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
								}
							}
							out[lower_right] = in[px[1][1]];
							
							
							
							
						} else if ( (in[px[1][2]] == in[px[2][1]]) && (in[px[1][1]] != in[px[2][2]]) ) {       
								//  X X X X   
								//  X 2 1 X   
								//  X 1 3 X   
								//  X X X X   
							
							if ( ((in[px[1][2]] == in[px[0][2]]) && (in[px[1][1]] == in[px[2][0]])) || ((in[px[1][2]] == in[px[0][1]]) && (in[px[1][2]] == in[px[2][2]]) && (in[px[1][2]] != in[px[0][2]]) && (in[px[1][1]] == in[px[0][0]]))){
								//  X X 1 X           2 1 1 X 
								//  X A 1 X           X 2 1 X 
								//  A X X X    or     X X 1 X 
								//  X X X X           X X X X 
								out[upper_right] = in[px[1][2]];
							}else{
								out[upper_right] = interpolate_pixels(in[px[1][1]],in[px[1][2]]);
							}
							
							if ( ((in[px[2][1]] == in[px[2][0]]) && (in[px[1][1]] == in[px[0][2]])) || ((in[px[2][1]] == in[px[1][0]]) && (in[px[2][1]] == in[px[2][2]]) && (in[px[1][1]] != in[px[2][0]]) && (in[px[1][1]] == in[px[0][0]]))){
								//  X X A X           X 2 X X 
								//  X A X X           A 2 X X 
								//  1 1 X X    or     3 A A X 
								//  X X X X           X X X X 
								out[lower_left] = in[px[2][1]];
							}else{
								out[lower_left] = interpolate_pixels(in[px[1][1]],in[px[2][1]]);
							}
							out[lower_right] = in[px[1][2]];
							
						}else if ( (in[px[1][1]] == in[px[2][2]]) && (in[px[1][2]] == in[px[2][1]]) ) {/*
						 
							// these are two crossed diagonal pairs of pixels in the inner, center set of four.
							// If they're the same, then they're just a solid square.
							// but if they're not the same, then we weigh them against a surrounding ring of pixels, and see which
							// pair is more different from the ring.
							
							// The pair that is more different will end up being visually subdued - the result will be only 1/4 that pair, and 3/4 the other.
							
							//The ring is this asterisked set of pixels:
							//  X * * X
							//  * X X *
							//  * X X *
							//  X * * X
							if (in[px[1][1]] == in[px[1][2]]){  //First, check if they're the same.  If so, it's just a solid square.
									out[lower_right] = out[upper_right] = out[upper_left] = out[lower_left] = in[px[1][1]];

							} else {
								
								int difference_direction = 0;
								//These following lines compare the corners in this order, to the center pixels A and B.
								
								//  X 1 * X          X * 2 X          X * * X             X * * X
								//  1 A B *          * A B 2          * A B *             * A B *
								//  * B A *    then: * B A *    then: * B A 3      then:  4 B A *
								//  X * * X          X * * X          X * 3 X             X 4 * X     
								
							   	// These get summed up.  If the final sum is positive, then A is the non-matching color and gets subdued.
								// if the final sum is negative, then B is the non-matching color and gets subdued.
								difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[0][1]],in[px[1][0]]);
								difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[1][3]],in[px[0][2]]);
								difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[3][2]],in[px[2][3]]);
								difference_direction += calculate_difference(in[px[1][2]],in[px[1][1]],in[px[2][0]],in[px[3][1]]);
								
								if (difference_direction > 0){
									out[lower_right] = out[upper_right] = out[upper_left] = out[lower_left] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[1][2]],in[px[1][2]]);
								}else if(difference_direction < 0){
									out[lower_right] = out[upper_right] = out[upper_left] = out[lower_left] = interpolate_pixels(in[px[1][1]],in[px[1][1]],in[px[1][1]],in[px[1][2]]);
   								}else{
									out[lower_right] = out[upper_right] = out[upper_left] = out[lower_left] = interpolate_pixels(in[px[1][1]],in[px[1][2]],in[px[2][1]],in[px[2][2]]);
								}								
								
							}
						*/}
							
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

namespace {
	typedef boost::array<char, 4> OutputPixels;
	typedef boost::array<char, 25> InputMatrix;

	void flip_matrix(OutputPixels& pixels) {
		std::swap(pixels[0], pixels[1]);
		std::swap(pixels[2], pixels[3]);
	}

	void flop_matrix(OutputPixels& pixels) {
		std::swap(pixels[0], pixels[2]);
		std::swap(pixels[1], pixels[3]);
	}

	void flip_matrix(InputMatrix& pixels) {
		for(int y = 0; y != 5; ++y) {
			for(int x = 0; x < 5/2; ++x) {
				std::swap(pixels[y*5 + x], pixels[y*5 + 4 - x]);
			}
		}
	}

	void flop_matrix(InputMatrix& pixels) {
		for(int x = 0; x != 5; ++x) {
			for(int y = 0; y < 5/2; ++y) {
				std::swap(pixels[y*5 + x], pixels[((5-1) - y)*5 + x]);
			}
		}
	}

	struct ScalingOutput {
		OutputPixels output;
		InputMatrix input;
	};

	struct ScalingPattern {
		InputMatrix input;
		std::vector<ScalingOutput> output;
	};

	void flip_pattern(ScalingPattern& pattern) {
		flip_matrix(pattern.input);
		foreach(ScalingOutput& output, pattern.output) {
			flip_matrix(output.output);
			flip_matrix(output.input);
		}
	}

	void flop_pattern(ScalingPattern& pattern) {
		flop_matrix(pattern.input);
		foreach(ScalingOutput& output, pattern.output) {
			flop_matrix(output.output);
			flop_matrix(output.input);
		}
	}

}

UTILITY(generate_scaling_code)
{
	variant node(json::parse_from_file("./surface_scaling.cfg"));

	std::vector<ScalingPattern> patterns;
	foreach(variant pattern_node, node["pattern"].as_list()) {
		ScalingPattern pattern;
		const std::string& pattern_str = pattern_node["pattern"].as_string();
		int n = 0;
		foreach(char c, pattern_str) {
			if(!util::c_isspace(c)) {
				ASSERT_LOG(n < pattern.input.size(), "SURFACE SCALING PATTERN HAS TOO MANY CHARACTERS: " << pattern_str);
				pattern.input[n++] = c;
			}
		}

		ASSERT_LOG(n == 25, "SURFACE SCALING_PATTERN HAS TOO FEW CHARACTERS: " << pattern_str);

		foreach(variant output_node, pattern_node["output"].as_list()) {
			ScalingOutput output;
			const std::string& input_str = output_node["input"].as_string();
			int n = 0;
			foreach(char c, input_str) {
				if(!util::c_isspace(c)) {
					ASSERT_LOG(n < output.input.size(), "SURFACE SCALING INPUT HAS TOO MANY CHARACTERS: " << input_str);
					output.input[n++] = c;
				}
			}

			ASSERT_LOG(n == 25, "SURFACE SCALING INPUT HAS TOO FEW CHARACTERS: " << input_str);

			const std::string& output_str = output_node["output"].as_string();
			n = 0;
			foreach(char c, output_str) {
				if(!util::c_isspace(c)) {
					ASSERT_LOG(n < output.output.size(), "SURFACE SCALING OUTPUT HAS TOO MANY CHARACTERS: " << output_str);
					output.output[n++] = c;
				}
			}
			
			ASSERT_LOG(n == 4, "SURFACE SCALING OUTPUT HAS TOO FEW CHARACTERS: " << output_str);

			pattern.output.push_back(output);
		}

		patterns.push_back(pattern);
		flip_pattern(pattern);
		patterns.push_back(pattern);
		flop_pattern(pattern);
		patterns.push_back(pattern);
		flip_pattern(pattern);
		patterns.push_back(pattern);
	}

	foreach(ScalingPattern& pattern, patterns) {
		std::cout << "if(";
		for(int n = 0; n != pattern.input.size(); ++n) {
			if(pattern.input[n] != '*') {
				for(int m = n+1; m != pattern.input.size(); ++m) {
					if(pattern.input[n] == pattern.input[m]) {
						pattern.input[m] = '*';

						std::cout << "matrix[" << n << "] == matrix[" << m << "] &&\n";
					}
				}
			}
		}

		std::cout << "true) {\n";
		foreach(const ScalingOutput& output, pattern.output) {
			std::cout << "\t{\n\tPixelUnion pu;\n\tint red = 0, green = 0, blue = 0, count = 0;\n";
			int value_count = 0;
			for(int n = 0; n != output.input.size(); ++n) {
				if(util::c_isdigit(output.input[n]) && output.input[n] != '0') {
					const int value = output.input[n] - '0';
					value_count += value;
					std::cout << "\tpu.value = matrix[" << n << "];\n";
					std::cout << "\tred += pu.rgba[0]*" << value << "*pu.rgba[3];\n";
					std::cout << "\tgreen += pu.rgba[1]*" << value << "*pu.rgba[3];\n";
					std::cout << "\tblue += pu.rgba[2]*" << value << "*pu.rgba[3];\n";
					std::cout << "\tcount += pu.rgba[3]* " << value << ";\n";
				}
			}

			std::cout << "\tif(count > 0) {\n";
			std::cout << "\t\tred /= count;\n";
			std::cout << "\t\tgreen /= count;\n";
			std::cout << "\t\tblue /= count;\n";
			std::cout << "\t\tpu.rgba[0] = red; pu.rgba[1] = green; pu.rgba[2] = blue; pu.rgba[3] = 255;\n";

			for(int n = 0; n != output.output.size(); ++n) {
				if(output.output[n] != '0') {
					std::cout << "\t\tout" << n << " = pu.value;\n";
				}
			}

			std::cout << "\t}\n}\n";
		}

		std::cout << "\t}\n";
	}
}

}
