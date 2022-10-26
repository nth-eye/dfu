#include <iostream>
#include "dfu/log.h"
#include "dfu/enc.h"

int main(int, char**) 
{
    const uint8_t test[17] = {0xde, 0xad, 0xbe, 0xef};

    dfu::codec<99> codec;

    codec.encode_raw({0x55, 0x66, 0x77});
    codec.encode_rep(0x42, 100);
    codec.encode_rep(0x66, 1);
    codec.encode_arr({0x01, 0x02, 0x03}, 1);
    codec.encode_arr(test, 256);
    codec.encode_off(-8191, 1024);

    dfu::log_seq(codec);
}
