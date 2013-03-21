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
//#define ZLIB_CONST

#include "asserts.hpp"
#include "compress.hpp"
#include "unit_test.hpp"
#include "zlib.h"

#define CHUNK 16384

namespace zip {

std::vector<char> compress(const std::vector<char>& data, int compression_level)
{
	ASSERT_LOG(compression_level >= -1 && compression_level <= 9, "Compression level must be between -1(default) and 9.");
	if(data.empty()) {
		return data;
	}

	std::vector<char> output(compressBound(data.size()));

	Bytef* dst = reinterpret_cast<Bytef*>(&output[0]);
	uLongf dst_len = output.size();
	const Bytef* src = reinterpret_cast<const Bytef*>(&data[0]);

	const int result = compress2(dst, &dst_len, src, data.size(), compression_level);
	ASSERT_EQ(result, Z_OK);

	output.resize(dst_len);
	return output;
}

std::vector<char> decompress(const std::vector<char>& data)
{
	const unsigned int MAX_OUTPUT_SIZE = 256*1024*1024;

	unsigned int output_size = data.size()*10;
	if(output_size > MAX_OUTPUT_SIZE) {
		output_size = MAX_OUTPUT_SIZE;
	}

	do {
		std::vector<char> output(output_size);

		Bytef* dst = reinterpret_cast<Bytef*>(&output[0]);
		uLongf dst_len = output.size();
		const Bytef* src = reinterpret_cast<const Bytef*>(&data[0]);

		const int result = uncompress(dst, &dst_len, src, data.size());
		if(result == Z_OK) {
			output.resize(dst_len);
			return output;
		}

		output_size *= 2;
	} while(output_size < MAX_OUTPUT_SIZE);

	ASSERT_LOG(false, "COULD NOT DECOMPRESS " << data.size() << " BYTE BUFFER\n");
}

std::vector<char> decompress_known_size(const std::vector<char>& data, int size)
{
	std::vector<char> output(size);

	Bytef* dst = reinterpret_cast<Bytef*>(&output[0]);
	uLongf dst_len = output.size();
	const Bytef* src = reinterpret_cast<const Bytef*>(&data[0]);

	const int result = uncompress(dst, &dst_len, src, data.size());
	ASSERT_LOG(result != Z_MEM_ERROR, "Decompression out of memory");
	ASSERT_LOG(result != Z_BUF_ERROR, "Insufficient space in output buffer");
	ASSERT_LOG(result != Z_DATA_ERROR, "Compression data corrupt");
	ASSERT_LOG(result == Z_OK && dst_len == output.size(), "FAILED TO DECOMPRESS " << data.size() << " BYTES OF DATA TO EXPECTED " << output.size() << " BYTES: " << " result = " << result << " (Z_OK = " << Z_OK << ") OUTPUT " << dst_len);
	return output;
}

}

UNIT_TEST(compression_test)
{
	std::vector<char> data(100000);
	for(int n = 0; n != data.size(); ++n) {
		data[n] = 'A' + rand()%26;
	}

	std::vector<char> compressed = zip::compress(data);
	std::vector<char> uncompressed = zip::decompress(compressed);
	CHECK_EQ(uncompressed.size(), data.size());
	for(int n = 0; n != data.size(); ++n) {
		CHECK_EQ(data[n], uncompressed[n]);
	}
}
