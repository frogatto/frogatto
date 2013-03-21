/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
if(matrix[5] == matrix[6] &&
matrix[5] == matrix[12] &&
matrix[5] == matrix[18] &&
true) {
	{
	PixelUnion pu;
	int red = 0, green = 0, blue = 0, count = 0;
	pu.value = matrix[6];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[7];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[8];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[11];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[12];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[13];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	if(count > 0) {
		red /= count;
		green /= count;
		blue /= count;
		pu.rgba[0] = red; pu.rgba[1] = green; pu.rgba[2] = blue; pu.rgba[3] = 255;
		out1 = pu.value;
	}
}
	}
if(matrix[8] == matrix[9] &&
matrix[8] == matrix[12] &&
matrix[8] == matrix[16] &&
true) {
	{
	PixelUnion pu;
	int red = 0, green = 0, blue = 0, count = 0;
	pu.value = matrix[6];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[7];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[8];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[11];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[12];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[13];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	if(count > 0) {
		red /= count;
		green /= count;
		blue /= count;
		pu.rgba[0] = red; pu.rgba[1] = green; pu.rgba[2] = blue; pu.rgba[3] = 255;
		out0 = pu.value;
	}
}
	}
if(matrix[6] == matrix[12] &&
matrix[6] == matrix[18] &&
matrix[6] == matrix[19] &&
true) {
	{
	PixelUnion pu;
	int red = 0, green = 0, blue = 0, count = 0;
	pu.value = matrix[11];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[12];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[13];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[16];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[17];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[18];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	if(count > 0) {
		red /= count;
		green /= count;
		blue /= count;
		pu.rgba[0] = red; pu.rgba[1] = green; pu.rgba[2] = blue; pu.rgba[3] = 255;
		out2 = pu.value;
	}
}
	}
if(matrix[8] == matrix[12] &&
matrix[8] == matrix[15] &&
matrix[8] == matrix[16] &&
true) {
	{
	PixelUnion pu;
	int red = 0, green = 0, blue = 0, count = 0;
	pu.value = matrix[11];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[12];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[13];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[16];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[17];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	pu.value = matrix[18];
	red += pu.rgba[0]*1*pu.rgba[3];
	green += pu.rgba[1]*1*pu.rgba[3];
	blue += pu.rgba[2]*1*pu.rgba[3];
	count += pu.rgba[3]* 1;
	if(count > 0) {
		red /= count;
		green /= count;
		blue /= count;
		pu.rgba[0] = red; pu.rgba[1] = green; pu.rgba[2] = blue; pu.rgba[3] = 255;
		out3 = pu.value;
	}
}
	}
