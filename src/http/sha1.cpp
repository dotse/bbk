// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Based on public domain code.
// Modified by Göran Andersson <initgoran@gmail.com>

#include <sstream>
#include <iomanip>
#include <fstream>
#include <string.h>

#include "sha1.h"

static char base64(unsigned int value_in) {
    static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (value_in > 63)
        return '=';
    return encoding[static_cast<int>(value_in)];
}

// encode len bytes starting at src, write to dst.
// number of bytes written will be 4 times the smallest integer >= len/3.
void base64_encode(const unsigned char *src, size_t len, char *destination) {
    char *p = destination;
    while (len >= 3) {
        len -= 3;
        *p++ = base64(src[0] >> 2);
        *p++ = base64(static_cast<unsigned char>((src[0] & 0x3) << 4) + (src[1] >> 4));
        *p++ = base64(static_cast<unsigned char>((src[1] & 0xf) << 2) + (src[2] >> 6));
        *p++ = base64(src[2] & 0x3f);
        src += 3;
    }
    switch (len) {
    case 2:
        *p++ = base64(src[0] >> 2);
        *p++ = base64(static_cast<unsigned char>((src[0] & 0x3) << 4) + (src[1] >> 4));
        *p++ = base64(static_cast<unsigned char>((src[1] & 0xf) << 2));
        break;
    case 1:
        *p++ = base64(src[0] >> 2);
        *p++ = base64(static_cast<unsigned char>((src[0] & 0x3) << 4));
        break;
    }
    switch ((p-destination)%4) {
    case 3:
        *p++ = '=';
#ifdef __clang__
        [[clang::fallthrough]];
#elif defined __GNUC__
#if __GNUC__ > 6
        [[gnu::fallthrough]];
#endif
        // -Wimplicit-fallthrough=0
#endif
    case 2:
        *p++ = '=';
#ifdef __clang__
        [[clang::fallthrough]];
#elif defined __GNUC__
#if __GNUC__ > 6
        [[gnu::fallthrough]];
#endif
        // -Wimplicit-fallthrough=0
#endif
    case 1:
        *p++ = '=';
        break;
    }
}

namespace {
    int _one = 1;
}

bool SHA1::is_big_endian = (*(reinterpret_cast<int8_t *>(&_one)) == 0);

/* Help macros */
#define SHA1_ROL(value, bits) (((value) << (bits)) | (((value) & 0xffffffff) >> (32 - (bits))))
#define SHA1_BLK(i) (block[i&15] = SHA1_ROL(block[(i+13)&15] ^ block[(i+8)&15] ^ block[(i+2)&15] ^ block[i&15],1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define SHA1_R0(v,w,x,y,z,i) z += ((w&(x^y))^y)     + block[i]    + 0x5a827999 + SHA1_ROL(v,5); w=SHA1_ROL(w,30)
#define SHA1_R1(v,w,x,y,z,i) z += ((w&(x^y))^y)     + SHA1_BLK(i) + 0x5a827999 + SHA1_ROL(v,5); w=SHA1_ROL(w,30)
#define SHA1_R2(v,w,x,y,z,i) z += (w^x^y)           + SHA1_BLK(i) + 0x6ed9eba1 + SHA1_ROL(v,5); w=SHA1_ROL(w,30)
#define SHA1_R3(v,w,x,y,z,i) z += (((w|x)&y)|(w&x)) + SHA1_BLK(i) + 0x8f1bbcdc + SHA1_ROL(v,5); w=SHA1_ROL(w,30)
#define SHA1_R4(v,w,x,y,z,i) z += (w^x^y)           + SHA1_BLK(i) + 0xca62c1d6 + SHA1_ROL(v,5); w=SHA1_ROL(w,30)

SHA1::SHA1(char *buf) :
    the_res(buf) {
}

void SHA1::update(const char *key) {
    std::istringstream is(key);

    /* SHA1 initialization constants */
    digest[0] = 0x67452301;
    digest[1] = 0xefcdab89;
    digest[2] = 0x98badcfe;
    digest[3] = 0x10325476;
    digest[4] = 0xc3d2e1f0;

    /* Reset counters */
    transforms = 0;
    std::string buffer;

    std::string rest_of_buffer;
    read(is, rest_of_buffer, static_cast<int>(BLOCK_BYTES) - static_cast<int>(buffer.size()));
    buffer += rest_of_buffer;

    while (is) {
        uint32_t block[BLOCK_INTS];
        buffer_to_block(buffer, block);
        transform(block);
        read(is, buffer, BLOCK_BYTES);
    }

    /* Total number of hashed bits */
    uint64_t total_bits = (transforms*BLOCK_BYTES + buffer.size()) * 8;

    /* Padding */
    buffer += static_cast<char>(0x80);
    unsigned int orig_size = static_cast<unsigned int>(buffer.size());
    while (buffer.size() < BLOCK_BYTES) {
        buffer += static_cast<char>(0x00);
    }

    uint32_t block[BLOCK_INTS];
    buffer_to_block(buffer, block);

    if (orig_size > BLOCK_BYTES - 8) {
        transform(block);
        for (unsigned int i = 0; i < BLOCK_INTS - 2; i++) {
            block[i] = 0;
        }
    }

    /* Append total_bits, split this uint64_t into two uint32_t */
    block[BLOCK_INTS - 1] = static_cast<uint32_t>(total_bits);
    block[BLOCK_INTS - 2] = (total_bits >> 32);
    transform(block);

    /* Base64 */
    unsigned char *pp = reinterpret_cast<unsigned char *>(digest);
    unsigned int pos = 0;
    if (is_big_endian) {
        base64_encode(pp, 20, the_res);
        return;
    }

    // Byte order is 3, 2, 1, 0, 7, 6, 5, 4, 11, ...

    the_res[pos++] = base64(pp[3] >> 2);
    the_res[pos++] = base64(static_cast<unsigned char>((pp[3] & 0x3) << 4) + (pp[2] >> 4));
    the_res[pos++] = base64(static_cast<unsigned char>((pp[2] & 0xf) << 2) + (pp[1] >> 6));
    the_res[pos++] = base64(pp[1] & 0x3f);

    the_res[pos++] = base64(pp[0] >> 2);
    the_res[pos++] = base64(static_cast<unsigned char>((pp[0] & 0x3) << 4) + (pp[7] >> 4));
    the_res[pos++] = base64(static_cast<unsigned char>((pp[7] & 0xf) << 2) + (pp[6] >> 6));
    the_res[pos++] = base64(pp[6] & 0x3f);

    the_res[pos++] = base64(pp[5] >> 2);
    the_res[pos++] = base64(static_cast<unsigned char>((pp[5] & 0x3) << 4) + (pp[4] >> 4));
    the_res[pos++] = base64(static_cast<unsigned char>((pp[4] & 0xf) << 2) + (pp[11] >> 6));
    the_res[pos++] = base64(pp[11] & 0x3f);

    the_res[pos++] = base64(pp[10] >> 2);
    the_res[pos++] = base64(static_cast<unsigned char>((pp[10] & 0x3) << 4) + (pp[9] >> 4));
    the_res[pos++] = base64(static_cast<unsigned char>((pp[9] & 0xf) << 2) + (pp[8] >> 6));
    the_res[pos++] = base64(pp[8] & 0x3f);

    the_res[pos++] = base64(pp[15] >> 2);
    the_res[pos++] = base64(static_cast<unsigned char>((pp[15] & 0x3) << 4) + (pp[14] >> 4));
    the_res[pos++] = base64(static_cast<unsigned char>((pp[14] & 0xf) << 2) + (pp[13] >> 6));
    the_res[pos++] = base64(pp[13] & 0x3f);

    the_res[pos++] = base64(pp[12] >> 2);
    the_res[pos++] = base64(static_cast<unsigned char>((pp[12] & 0x3) << 4) + (pp[19] >> 4));
    the_res[pos++] = base64(static_cast<unsigned char>((pp[19] & 0xf) << 2) + (pp[18] >> 6));
    the_res[pos++] = base64(pp[18] & 0x3f);

    the_res[pos++] = base64(pp[17] >> 2);
    the_res[pos++] = base64(static_cast<unsigned char>((pp[17] & 0x3) << 4) + (pp[16] >> 4));
    the_res[pos++] = base64(static_cast<unsigned char>((pp[16] & 0xf) << 2));
}

/*
 * Hash a single 512-bit block. This is the core of the algorithm.
 */

void SHA1::transform(uint32_t block[BLOCK_BYTES])
{
    /* Copy digest[] to working vars */
    uint32_t a = digest[0];
    uint32_t b = digest[1];
    uint32_t c = digest[2];
    uint32_t d = digest[3];
    uint32_t e = digest[4];

    /* 4 rounds of 20 operations each. Loop unrolled. */
    SHA1_R0(a,b,c,d,e, 0);
    SHA1_R0(e,a,b,c,d, 1);
    SHA1_R0(d,e,a,b,c, 2);
    SHA1_R0(c,d,e,a,b, 3);
    SHA1_R0(b,c,d,e,a, 4);
    SHA1_R0(a,b,c,d,e, 5);
    SHA1_R0(e,a,b,c,d, 6);
    SHA1_R0(d,e,a,b,c, 7);
    SHA1_R0(c,d,e,a,b, 8);
    SHA1_R0(b,c,d,e,a, 9);
    SHA1_R0(a,b,c,d,e,10);
    SHA1_R0(e,a,b,c,d,11);
    SHA1_R0(d,e,a,b,c,12);
    SHA1_R0(c,d,e,a,b,13);
    SHA1_R0(b,c,d,e,a,14);
    SHA1_R0(a,b,c,d,e,15);
    SHA1_R1(e,a,b,c,d,16);
    SHA1_R1(d,e,a,b,c,17);
    SHA1_R1(c,d,e,a,b,18);
    SHA1_R1(b,c,d,e,a,19);
    SHA1_R2(a,b,c,d,e,20);
    SHA1_R2(e,a,b,c,d,21);
    SHA1_R2(d,e,a,b,c,22);
    SHA1_R2(c,d,e,a,b,23);
    SHA1_R2(b,c,d,e,a,24);
    SHA1_R2(a,b,c,d,e,25);
    SHA1_R2(e,a,b,c,d,26);
    SHA1_R2(d,e,a,b,c,27);
    SHA1_R2(c,d,e,a,b,28);
    SHA1_R2(b,c,d,e,a,29);
    SHA1_R2(a,b,c,d,e,30);
    SHA1_R2(e,a,b,c,d,31);
    SHA1_R2(d,e,a,b,c,32);
    SHA1_R2(c,d,e,a,b,33);
    SHA1_R2(b,c,d,e,a,34);
    SHA1_R2(a,b,c,d,e,35);
    SHA1_R2(e,a,b,c,d,36);
    SHA1_R2(d,e,a,b,c,37);
    SHA1_R2(c,d,e,a,b,38);
    SHA1_R2(b,c,d,e,a,39);
    SHA1_R3(a,b,c,d,e,40);
    SHA1_R3(e,a,b,c,d,41);
    SHA1_R3(d,e,a,b,c,42);
    SHA1_R3(c,d,e,a,b,43);
    SHA1_R3(b,c,d,e,a,44);
    SHA1_R3(a,b,c,d,e,45);
    SHA1_R3(e,a,b,c,d,46);
    SHA1_R3(d,e,a,b,c,47);
    SHA1_R3(c,d,e,a,b,48);
    SHA1_R3(b,c,d,e,a,49);
    SHA1_R3(a,b,c,d,e,50);
    SHA1_R3(e,a,b,c,d,51);
    SHA1_R3(d,e,a,b,c,52);
    SHA1_R3(c,d,e,a,b,53);
    SHA1_R3(b,c,d,e,a,54);
    SHA1_R3(a,b,c,d,e,55);
    SHA1_R3(e,a,b,c,d,56);
    SHA1_R3(d,e,a,b,c,57);
    SHA1_R3(c,d,e,a,b,58);
    SHA1_R3(b,c,d,e,a,59);
    SHA1_R4(a,b,c,d,e,60);
    SHA1_R4(e,a,b,c,d,61);
    SHA1_R4(d,e,a,b,c,62);
    SHA1_R4(c,d,e,a,b,63);
    SHA1_R4(b,c,d,e,a,64);
    SHA1_R4(a,b,c,d,e,65);
    SHA1_R4(e,a,b,c,d,66);
    SHA1_R4(d,e,a,b,c,67);
    SHA1_R4(c,d,e,a,b,68);
    SHA1_R4(b,c,d,e,a,69);
    SHA1_R4(a,b,c,d,e,70);
    SHA1_R4(e,a,b,c,d,71);
    SHA1_R4(d,e,a,b,c,72);
    SHA1_R4(c,d,e,a,b,73);
    SHA1_R4(b,c,d,e,a,74);
    SHA1_R4(a,b,c,d,e,75);
    SHA1_R4(e,a,b,c,d,76);
    SHA1_R4(d,e,a,b,c,77);
    SHA1_R4(c,d,e,a,b,78);
    SHA1_R4(b,c,d,e,a,79);

    /* Add the working vars back into digest[] */
    digest[0] += a;
    digest[1] += b;
    digest[2] += c;
    digest[3] += d;
    digest[4] += e;

    /* Count the number of transformations */
    transforms++;
}

void SHA1::buffer_to_block(const std::string &buffer, uint32_t block[BLOCK_BYTES])
{
    /* Convert the std::string (byte buffer) to a uint32_t array (MSB) */
    for (unsigned int i = 0; i < BLOCK_INTS; i++) {
        block[i] = (buffer[4*i+3] & 0xff)
            | static_cast<uint32_t>(buffer[4*i+2] & 0xff)<<8
            | static_cast<uint32_t>(buffer[4*i+1] & 0xff)<<16
            | static_cast<uint32_t>(buffer[4*i+0] & 0xff)<<24;
    }
}

void SHA1::read(std::istream &is, std::string &s, int max) {
    char sbuf[BLOCK_BYTES];
    is.read(sbuf, max);
    s.assign(sbuf, static_cast<size_t>(is.gcount()));
}
