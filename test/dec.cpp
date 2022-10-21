#include <gtest/gtest.h>
#include "dfu/dec.h"
#include "dfu/log.h"

TEST(Decode, Test)
{
    const uint8_t test[] = { 
        dfu::type_raw        | (3 << 4), 0x01, 0x02, 0x03, 0x04,
        dfu::type_rep_byte   | (9 << 4), 0x42,
        dfu::type_rep_array  | (1 << 4), 0x02, 0x13, 0x37,
        dfu::type_old_offset | (1 << 4), uint8_t((32 << 2) | 0x00),
    };
    dfu::log_seq(test);
}