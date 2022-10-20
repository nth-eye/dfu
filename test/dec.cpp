#include <gtest/gtest.h>
#include "dfu/dec.h"
#include "dfu/log.h"

TEST(Dfu, Test)
{
    const uint8_t test_1[] = { 
        dfu::type_raw        | (3 << 4), 0x01, 0x02, 0x03, 0x04,
        dfu::type_rep_byte   | (9 << 4), 0x42,
        dfu::type_rep_array  | (1 << 4), 0x02, 0x13, 0x37,
    };
    // const uint8_t test_2[] = { dfu::type_rep_byte   | (9 << 4), 0x42 };
    // const uint8_t test_3[] = { dfu::type_rep_array  | (1 << 4), 0x02, 0x13, 0x37 };

    dfu::log_seq(test_1);
    // dfu::log_seq(test_2);
    // dfu::log_seq(test_3);
}