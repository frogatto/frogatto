//#define ZLIB_CONST

#include "asserts.hpp"
#include "compress.hpp"
#include "unit_test.hpp"
#include "zlib.h"

#define CHUNK 16384

namespace zip {

std::vector<char> compress(const std::vector<char>& data, int compression_level) {
	z_stream strm;
	int ret;
	int pos_in = 0;
	int pos_out = 0;
	int flush;
	std::vector<char> in(data); // <-- annoying work around for old versions of zlib that don't define z_stream.in as const, by defining ZLIB_CONST
	std::vector<char> out;

	ASSERT_LOG(compression_level >= -1 && compression_level <= 9, "Compression level must be between -1(default) and 9.");
	memset(&strm, 0, sizeof(z_stream));
    if(deflateInit(&strm, compression_level) != Z_OK) {
		CompressionException e = {"Unable to initialise deflation routines."};
        throw CompressionException(e);
	}
	do {
		strm.avail_in = (in.size() - pos_in) > CHUNK ? CHUNK : in.size() - pos_in;
        flush = (strm.avail_in + pos_in) == in.size() ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = reinterpret_cast<Bytef*>(&in[pos_in]);
		do {
			out.resize(pos_out - out.size() + CHUNK);
			strm.avail_out = CHUNK;
			strm.next_out = reinterpret_cast<Bytef*>(&out[pos_out]);
            ret = deflate(&strm, flush);    // no bad return value
			ASSERT_LOG(ret != Z_STREAM_ERROR, "Error in the compression stream");
            pos_out += CHUNK - strm.avail_out;
		} while (strm.avail_out == 0);
		ASSERT_LOG(strm.avail_in == 0, "zip::compress(): All input used");     // all input will be used
	} while(flush != Z_FINISH);
	ASSERT_LOG(ret == Z_STREAM_END, "zip::compress(): stream will be complete");
	deflateEnd(&strm);
	out.resize(pos_out);
	return out;
}

std::vector<char> decompress(const std::vector<char>& data) {
	int ret;
	z_stream strm;
	int pos_in = 0;
	int pos_out = 0;
	std::vector<char> in(data); // <-- annoying work around for old versions of zlib that don't define z_stream.in as const, by defining ZLIB_CONST
	std::vector<char> out;
	memset(&strm, 0, sizeof(z_stream));
	if(inflateInit(&strm) != Z_OK) {
		CompressionException e = {"Unable to initialise inflation routines."};
        throw CompressionException(e);
	}

	do {
		strm.avail_in = (in.size() - pos_in) > CHUNK ? CHUNK : in.size() - pos_in;
		if(strm.avail_in == 0){ break; }
		strm.next_in = reinterpret_cast<Bytef*>(&in[pos_in]);
		do {
			out.resize(pos_out - out.size() + CHUNK);
			strm.avail_out = CHUNK;
			strm.next_out = reinterpret_cast<Bytef*>(&out[pos_out]);
            ret = inflate(&strm, Z_NO_FLUSH);    // no bad return value
			ASSERT_LOG(ret != Z_STREAM_ERROR, "zip::decompress(): Error in the compression stream");
			if(ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
				CompressionException e = {"Error in decompression."};
				throw CompressionException(e);
			}
            pos_out += CHUNK - strm.avail_out;
		} while (strm.avail_out == 0);
	} while(ret != Z_STREAM_END);
	inflateEnd(&strm);
	out.resize(pos_out);
	return out;
}

}

UNIT_TEST(compress_function) {
	std::string in("The quick brown fox jumps over the lazy dog");
	std::string out("x\x9c\x0b\xc9HU(,\xcdL\xceVH*\xca/\xcfSH\xcb\xafP\xc8*\xcd-(V\xc8/K-R(\x01J\xe7$VU*\xa4\xe4\xa7\x03\x00[\xdc\x0f\xda", 50);
	std::vector<char> resv = zip::compress(std::vector<char>(in.begin(), in.end()));
	std::string ress(resv.begin(), resv.end());
	CHECK_EQ(out, ress);
}

UNIT_TEST(decompress_function) {
	std::string out("The quick brown fox jumps over the lazy dog");
	std::string in("x\x9c\x0b\xc9HU(,\xcdL\xceVH*\xca/\xcfSH\xcb\xafP\xc8*\xcd-(V\xc8/K-R(\x01J\xe7$VU*\xa4\xe4\xa7\x03\x00[\xdc\x0f\xda", 50);
	std::vector<char> resv = zip::decompress(std::vector<char>(in.begin(), in.end()));
	std::string ress(resv.begin(), resv.end());
	CHECK_EQ(out, ress);
}
