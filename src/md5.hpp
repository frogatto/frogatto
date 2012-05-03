#ifndef MD5_H
#define MD5_H

#include <stdint.h>
#include <iostream>
#include <sstream>
#include <vector>

namespace md5 {
struct MD5Context {
        uint32_t buf[4];
        uint32_t bits[2];
        uint8_t in[64];
};

extern void MD5Init(struct MD5Context *ctx);
extern void MD5Update(struct MD5Context *ctx, unsigned char *buf, unsigned len);
extern void MD5Final(uint8_t digest[16], struct MD5Context *ctx);
extern void MD5Transform(uint32_t buf[4], uint32_t in[16]);

std::string sum(const std::string& data);
}

class MD5
{
public:
	MD5() {
	}
	virtual ~MD5() { 
	}

	static std::string calc(const std::string& s) {
		std::vector<uint8_t> v(s.begin(),s.end());
		std::vector<uint8_t> result = calc(v);
		std::string ss(result.begin(), result.end());
		return ss;
	}

	static std::vector<uint8_t> calc(std::vector<uint8_t> v) {
		struct md5::MD5Context ctx;
		md5::MD5Init(&ctx);
		md5::MD5Update(&ctx, &v[0], v.size());
		std::vector<uint8_t> result(16);
		md5::MD5Final(&result[0], &ctx);
		return result;
	}
};

#endif /* !MD5_H */

