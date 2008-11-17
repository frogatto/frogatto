#include <inttypes.h>
#include <iostream>
#include <cassert>

#include "surface.hpp"

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
			
			//if ((y > 0) && (y < input->h - 1) && (x > 0) && (x < input->w - 1)){
			if ((y > 0) && (y < input->h - 2) && (x > 0) && (x < input->w - 2)){
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
						
						/*const int index_up_left = y*input->w + x - input->w - 1;
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
						} */
				
				
			//  2xSai works on a square group of sixteen pixels, rather than the square group of nine that Eagle works on.  In Eagle, the current pixel being upsized is the one in the middle of this square of nine; in 2xSai, the current pixel is the one in the upper-left of the middle four.
			
			//  X X X X  |  0  1  2  3
			//  X * X X  |  4  5  6  7
			//  X X X X  |  8  9  10 11
			//  X X X X  |  12 13 14 15
				
						const int index_0 = y*input->w + x - input->w - 1;
						const int index_1 = y*input->w + x - input->w;
						const int index_2 = y*input->w + x - input->w + 1;
						const int index_3 = y*input->w + x - input->w + 2;
						const int index_4 = y*input->w + x - 1;
						const int index_5 = y*input->w + x;
						const int index_6 = y*input->w + x + 1;
						const int index_7 = y*input->w + x + 2;
						const int index_8 = y*input->w + x + input->w - 1;
						const int index_9 = y*input->w + x + input->w;
						const int index_10 = y*input->w + x + input->w + 1;
						const int index_11 = y*input->w + x + input->w + 2;
						const int index_12 = y*input->w + x + 2*input->w - 1;
						const int index_13 = y*input->w + x + 2*input->w;
						const int index_14 = y*input->w + x + 2*input->w + 1;
						const int index_15 = y*input->w + x + 2*input->w + 2;
						const int max_index = input->w*input->h;
						
						const int out_index_upper_left = (y*2)*result->w + x*2;
						const int out_index_upper_right = (y*2)*result->w + x*2 + 1;
						const int out_index_lower_left = (y*2 + 1)*result->w + x*2;
						const int out_index_lower_right = (y*2 + 1)*result->w + x*2 + 1;
				
						assert(index_2 < max_index);
						assert(index_3 < max_index);
						assert(index_6 < max_index);
						assert(index_7 < max_index);
						assert(index_8 < max_index);
						assert(index_9 < max_index);
						assert(index_10 < max_index);
						assert(index_11 < max_index);
						assert(index_12 < max_index);
						assert(index_13 < max_index);
						assert(index_14 < max_index);
						assert(index_15 < max_index);

						//the first check we do, before testing more complex patterns, is for diagonal lines
						if ( (in[index_9] == in[index_6]) && (in[index_5] != in[index_10]) ) {
							out[out_index_lower_right] = in[index_9];
							out[out_index_upper_right] = out[out_index_lower_right];
						} else if ( (in[index_9] != in[index_6]) && (in[index_5] == in[index_10]) ){
							out[out_index_lower_right] = in[index_5];
							out[out_index_upper_right] = out[out_index_lower_right];
						} else if ( (in[index_9] == in[index_6]) && (in[index_5] == in[index_10]) ){
							// these are crossed diagonal pairs of pixels in the inner, center set of four.
							// if they're the same, then we weight them against a surrounding ring of pixels, and see which
							// pair is more different from the ring.  The ring is this asterisked set of pixels:
							//  X * * X
							//  * X X *
							//  * X X *
							//  X * * X
							int difference_direction = 0;
							difference_direction += calculate_difference(in[index_6],in[index_5],in[index_8],in[index_13]);
							difference_direction += calculate_difference(in[index_6],in[index_5],in[index_4],in[index_1]);
							difference_direction += calculate_difference(in[index_6],in[index_5],in[index_14],in[index_11]);
							difference_direction += calculate_difference(in[index_6],in[index_5],in[index_2],in[index_7]);
								
							if (difference_direction > 0){
								out[out_index_upper_right] = in[index_6];
							}else if(difference_direction < 0){
								out[out_index_upper_right] = in[index_5];
							}else{
								out[out_index_upper_right] = interpolate_pixels(in[index_5],in[index_6]);
							}
							out[out_index_lower_right] = out[out_index_upper_right]; 
						} else {
							//now test for more complex patterns
							
							//calculate lower-right output pixel
							if( in[index_6] == in[index_10] && in[index_10] == in[index_13] && in[index_9] != in[index_14] && in[index_10] != in[index_12] ){
								//weight 10's color more heavily
								out[out_index_lower_right] = interpolate_pixels(in[index_10],in[index_10],in[index_10],in[index_9]);
							}else if( in[index_5] == in[index_9] && in[index_9] == in[index_14] && in[index_13] != in[index_10] && in[index_9] != in[index_15]){
								//weight 9's color more heavily
								out[out_index_lower_right] = interpolate_pixels(in[index_9],in[index_9],in[index_9],in[index_10]);
							}else{
								//mix 9 and 10 evenly
								out[out_index_lower_right] = interpolate_pixels(in[index_9],in[index_10]);
							}
							
							//calculate upper-right output pixel
							if( in[index_6] == in[index_10] && in[index_6] == in[index_1] && in[index_5] != in[index_2] && in[index_6] != in[index_0] ){
								//weight 6's color more heavily
								out[out_index_upper_right] = interpolate_pixels(in[index_6],in[index_6],in[index_6],in[index_5]);
							}else if( in[index_5] == in[index_9] && in[index_5] == in[index_2] && in[index_1] != in[index_6] && in[index_5] != in[index_3]){
								//weight 5's color more heavily
								out[out_index_upper_right] = interpolate_pixels(in[index_5],in[index_5],in[index_5],in[index_6]);
							}else{
								//mix 6 and 5 evenly
								out[out_index_upper_right] = interpolate_pixels(in[index_5],in[index_6]);
							}
							
							//calculate lower-left output pixel
							if( in[index_5] == in[index_10] && in[index_4] == in[index_5] && in[index_9] != in[index_6] && in[index_5] != in[index_14] ){
								// ?
								out[out_index_lower_left] = interpolate_pixels(in[index_9],in[index_5]);
							}else if( in[index_5] == in[index_8] && in[index_6] == in[index_5] && in[index_4] != in[index_9] && in[index_5] != in[index_12]){
								// ?
								out[out_index_lower_left] = interpolate_pixels(in[index_9],in[index_5]);
							}else{
								out[out_index_lower_left] = in[index_9];
							}
							
							//calculate upper-left output pixel
							if( in[index_9] == in[index_6] && in[index_5] != in[index_10] && in[index_8] == in[index_9] && in[index_9] != in[index_2] ){
								// ?
								out[out_index_upper_left] = interpolate_pixels(in[index_9],in[index_5]);
							}else if( in[index_4] == in[index_9] && in[index_10] == in[index_9] && in[index_8] != in[index_5] && in[index_9] != in[index_0]){
								// ?
								out[out_index_upper_left] = interpolate_pixels(in[index_9],in[index_5]);
							}else{
								out[out_index_upper_left] = in[index_5];
							}
							
						}
			}
		}
	}

	return result;
}

}
