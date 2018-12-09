// Copyright (c) 2017 IIS (The Internet Foundation in Sweden)
// Based on public domain code.
// Modified by Göran Andersson <goran@init.se>

#pragma once

#include <iostream>
#include <string>

void base64_encode(const unsigned char *src, size_t len, char *destination);

class SHA1 {
public:
    // You provide a buffer, at least 27 bytes large.
    SHA1(char *buf);

    // Result (base64 encoded) will be stored in buf, no nul termination.
    void update(const char *key);

private:
    static bool is_big_endian;
    char *the_res;

    static const uint32_t DIGEST_INTS = 5;  // number of 32bit integers per SHA1 digest
    static const unsigned int BLOCK_INTS = 16;  // number of 32bit integers per SHA1 block
    static const unsigned int BLOCK_BYTES = BLOCK_INTS * 4;

    uint32_t digest[DIGEST_INTS];
    uint64_t transforms;

    void transform(uint32_t block[BLOCK_BYTES]);

    static void buffer_to_block(const std::string &buffer, uint32_t block[BLOCK_BYTES]);
    static void read(std::istream &is, std::string &s, int max);
};
