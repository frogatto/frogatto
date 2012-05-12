#include "asserts.hpp"
#include "base64.hpp"
#include "unit_test.hpp"

namespace base64 {

static const char _base64chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const unsigned char _base64inv[] = 
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00>\x00\x00\x00?456789:"
    ";<=\x00\x00\x00\x00\x00\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07"
    "\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17"
    "\x18\x19\x00\x00\x00\x00\x00\x00\x1a\x1b\x1c\x1d\x1e\x1f !\"#$%&'"
    "()*+,-./0123";

/*! Calculates the amount of buffer space required to encode a string 
    of length n.
    \param n Number of characters to be encoded.
    \return Amount of buffer space required to encode the number of
        characters specified.                                               */
static int encode_buffer_req(int n, int output_line_len)
{
    // Allow for 3 characters expand to 4-characters
    n = ( ( ( n + 2 ) / 3 ) * 4 );
    // Allow for <cr> every #OUTPUT_LINE_LENGTH characters.
	if(output_line_len > 0) {
		n += ( n / output_line_len );
	}
    return n;
}

static void encode_block(std::vector<char>::const_iterator& in_it, std::vector<char>::iterator& out, int len) {
	unsigned char in[3];
	for(int i = 0; i < 3; i++) {
		in[i] = len > i ? *in_it++ : 0;
	}
    *out++ = _base64chars[ in[ 0 ] >> 2 ];
    *out++ = _base64chars[ ( ( in[ 0 ] & 3 ) << 4 ) | ( ( in[ 1 ] & 0xf0 ) >> 4 ) ];
    *out++ = ( len > 1 ? _base64chars[ ( ( in[ 1 ] & 0x0f ) << 2 )
        |( ( in[ 2 ] & 0xc0 ) >> 6 ) ] : '=' );
    *out++ = ( len > 2 ? _base64chars[ in[ 2 ] & 0x3f ] : '=' );
}

std::vector<char> b64encode(const std::vector<char>& data, int output_line_length) {
	std::vector<char> dest;
	dest.resize(encode_buffer_req(data.size(), output_line_length));
	std::vector<char>::const_iterator in_pos = data.begin();
	std::vector<char>::iterator out_pos = dest.begin();
	int line_cnt = 0;
	for(unsigned i = 0; i < data.size(); i += 3) {
		encode_block( in_pos, out_pos, ( data.size() - i ) > 2 ? 3 : data.size() - i );
		line_cnt += 4;
		if(line_cnt >= output_line_length) {
			line_cnt = 0;
			*out_pos++ = '\n';
		}
	}
	if(line_cnt >= output_line_length) {
		*out_pos++ = '\n';
	}
	return dest;
}

/*! \brief Calculate a quick estimate of the amount of bytes required to 
    store the decoded base64 data.  This is only an estimate as it assumes
    that there are no ignore characters in the data stream.
    \param n Number of encoded base64 characters.
    \return An estimate of the number buffer space required for the decode.
*/
static int decode_buffer_req(int n) {
    return ((n + 3) / 4) * 3;
}

/*! Test a character to determine if it is a valid base64 encode character.
    \param ch Character to test if it is in the expected base64 character set.
    \return Non-zero if the character is a valid base64 character.  Zero if the
        character isn't valid.
*/
static bool is_base64_char(int ch) {
    if(ch >= 'A' && ch <= 'Z') return true;
    if(ch >= 'a' && ch <= 'z') return true;
    if(ch >= '0' && ch <= '9') return true;
    if(ch == '+' || ch == '/') return true;
    return false;
}

/*! \brief Takes a block of 4 base64 encoded characters and decodes them.
    \param in The block of base64 encoded characters, must come from the
        expected character set.
    \param out Pointer to place the 3 plaintext characters.
*/
static void decodeblock(const char* in, std::vector<char>::iterator& out) {
    unsigned long nn = (_base64inv[in[0]] << 18) | (_base64inv[in[1]] << 12) 
        | (_base64inv[in[2]] << 6) | (_base64inv[in[3]]);
    *out++ = (char)(nn >> 16);
    *out++ = (char)(nn >> 8);
    *out++ = (char)nn;
}

/*! \brief Decode a block of base64 encoded data.
    \param data Base64 encoded block of data.  Data may contain 
        other characters such as line-terminators and whitespace.
	\return A block of unencoded data, if unencoding was possible.
*/
std::vector<char> b64decode(const std::vector<char>& data) {
	std::vector<char> dest;
	dest.resize(decode_buffer_req(data.size()));
	std::vector<char>::const_iterator in_pos = data.begin();
	std::vector<char>::iterator out_pos = dest.begin();

    char decode_buffer[ 4 ];
    int ndx = 0;
    int cnt = 0;
    int padding = 0;
    for(unsigned i = 0; i < data.size(); i++) {
        int ch = *in_pos++;
        if( ch == '=' ) {
            ch = 'A';
            padding++;
        }
        if(is_base64_char(ch)) {
            decode_buffer[ndx] = ch;
            if(++ndx >= 4) {
                ndx = 0;
                decodeblock(decode_buffer, out_pos);
                cnt += 3 - padding;
                padding = 0;
            }
        }
    }
	dest.resize(cnt);
	return dest;
}

}

UNIT_TEST(base64_encode) {
	std::vector<char> in;
	std::string ress("YQ==");
	in.push_back('a');
	std::vector<char> resv = base64::b64encode(in);
	std::string s(resv.begin(), resv.end());
	CHECK_EQ(s,ress)
}

UNIT_TEST(base64_encode_bin) {
	std::vector<char> in;
	for(int i = 0; i < 256; i++) {
		in.push_back(i);
	}
	std::string ress = 
		"AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v\n"
		"MDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5f\n"
		"YGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6P\n"
		"kJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/\n"
		"wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v\n"
		"8PHy8/T19vf4+fr7/P3+/w==";
	std::vector<char> resv = base64::b64encode(in);
	std::string s(resv.begin(), resv.end());
	CHECK_EQ(s,ress)
}

UNIT_TEST(base64_decode) {
	std::string s = "The quick brown fox jumps over the lazy dog";
	std::vector<char> resv = base64::b64decode(base64::b64encode(std::vector<char>(s.begin(), s.end())));
	std::string res(resv.begin(), resv.end());
	CHECK_EQ(s, res);
}
