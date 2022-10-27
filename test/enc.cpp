#include <gtest/gtest.h>
#include "dfu/enc.h"
#include "dfu/log.h"

static void check(dfu::cref res, std::initializer_list<uint8_t> exp)
{
    ASSERT_EQ(res.size(), exp.size());
    for (size_t i = 0; auto it : exp)
        ASSERT_EQ(res[i++], it) << "at index " << i;
}

class Encode : public ::testing::Test {
protected:
    void SetUp() override 
    {

    }
    void TearDown() override 
    {
        codec.clear();
    }
    dfu::codec<77> codec;
};

TEST_F(Encode, Raw)
{
    const uint8_t test_1[1]  = {0x42};
    const uint8_t test_2[17] = {0xde, 0xad, 0xbe, 0xef};

    codec.encode_raw({0x00});
    codec.encode_raw({0x00, 0x11, 0x22});
    codec.encode_raw({0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff});
    codec.encode_raw(test_1);
    codec.encode_raw(test_2);

    check(codec, {
        0x00, 0x00, // RAW[1]
        0x20, 0x00, 0x11, 0x22, // RAW[3]
        0xf0, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, // RAW[16]
        0x00, 0x42, // RAW[1]
        0x04, 0x01, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // RAW[17]
    });
}

TEST_F(Encode, RepeatedByte)
{
    codec.encode_rep(0x2a, 1);
    codec.encode_rep(0x11, 3);
    codec.encode_rep(0xff, 16);
    codec.encode_rep(0x55, 17);
    codec.encode_rep(0x66, 4096);
    codec.encode_rep(0x00, 4097);
    codec.encode_rep(0x44, 1048576);
    codec.encode_rep(0x69, 1048577);
    codec.encode_rep(0x77, 268435456);

    check(codec, {
        0x01, 0x2a, // REP[1]
        0x21, 0x11, // REP[3]
        0xf1, 0xff, // REP[16]
        0x05, 0x01, 0x55, // REP[17]
        0xf5, 0xff, 0x66, // REP[4096]
        0x09, 0x00, 0x01, 0x00, // REP[4097]
        0xf9, 0xff, 0xff, 0x44, // REP[1048576]
        0x0d, 0x00, 0x00, 0x01, 0x69, // REP[1048577]
        0xfd, 0xff, 0xff, 0xff, 0x77, // REP[268435456]
    });
}

TEST_F(Encode, RepeatedArray)
{
    const uint8_t test_1[1]  = {0x42};
    const uint8_t test_2[17] = {0xde, 0xad, 0xbe, 0xef};

    codec.encode_arr({0x00}, 1);
    codec.encode_arr({0x00}, 3);
    codec.encode_arr({0x00, 0x11, 0x22}, 1);
    codec.encode_arr({0x00, 0x11, 0x22}, 2);
    codec.encode_arr(test_1, 16);
    codec.encode_arr(test_1, 255);
    codec.encode_arr(test_2, 17);
    codec.encode_arr(test_2, 256);

    check(codec, {
        0x02, 0x00, 0x00, // ARR[1]
        0x02, 0x02, 0x00, // ARR[1]
        0x22, 0x00, 0x00, 0x11, 0x22, // ARR[3]
        0x22, 0x01, 0x00, 0x11, 0x22, // ARR[3]
        0x02, 0x0f, 0x42, // ARR[1]
        0x02, 0xfe, 0x42, // ARR[1]
        0x06, 0x01, 0x10, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ARR[17]
        0x06, 0x01, 0xff, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ARR[17]
    });
}

TEST_F(Encode, OldOffset)
{
    codec.encode_off(0, 1);
    codec.encode_off(0x1f, 0x10);
    codec.encode_off(0x1fff, 0x1000);
    codec.encode_off(0x1fffff, 0x100000);
    codec.encode_off(0x1fffffff, 0x10000000);
    codec.encode_off(-0x20, 0x10);
    codec.encode_off(-0x2000, 0x1000);
    codec.encode_off(-0x200000, 0x100000);
    codec.encode_off(-0x20000000, 0x10000000);

    check(codec, {
        0x03, 0x00, // OLD[1] offs +0
        0xf3, 0x7c, // OLD[16] offs +31
        0xf7, 0xff, 0xfd, 0x7f, // OLD[4096] +8191
        0xfb, 0xff, 0xff, 0xfe, 0xff, 0x7f, // OLD[1048576] +2097151
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, // OLD[268435456] +536870911
        0xf3, 0x80, // OLD[16] offs -32
        0xf7, 0xff, 0x01, 0x80, // OLD[4096] -8192
        0xfb, 0xff, 0xff, 0x02, 0x00, 0x80, // OLD[1048576] -2097152
        0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x00, 0x80, // OLD[268435456] -536870912
    });
}

TEST_F(Encode, Failures)
{
    const uint8_t test[76] = {};

    EXPECT_EQ(codec.encode_raw(test),           dfu::err_no_memory);
    ASSERT_EQ(codec.encode_raw({}),             dfu::err_invalid_size);
    ASSERT_EQ(codec.encode_rep(0x42, 0),        dfu::err_invalid_size);
    ASSERT_EQ(codec.encode_arr({}, 1),          dfu::err_invalid_size);
    ASSERT_EQ(codec.encode_arr({0x00}, 0),      dfu::err_invalid_size);
    ASSERT_EQ(codec.encode_arr({0x00}, 257),    dfu::err_invalid_size);
    
    check(codec, {});
}

TEST_F(Encode, ConstexprMixed)
{
    static constexpr auto ce_codec = []()
    {
        const uint8_t test[17] = {0xde, 0xad, 0xbe, 0xef};
        dfu::codec<99> codec;
        codec.encode_raw({0x55, 0x66, 0x77});
        codec.encode_rep(0x42, 100);
        codec.encode_rep(0x66, 1);
        codec.encode_arr({0x01, 0x02, 0x03}, 1);
        codec.encode_arr(test, 256);
        codec.encode_off(-8191, 1024);
        return codec;
    }();

    check(ce_codec, {
        0x20, 0x55, 0x66, 0x77,         // RAW[3] {55 66 77}
        0x35, 0x06, 0x42,               // REP[100] byte 0x42
        0x01, 0x66,                     // REP[1] byte 0x66 
        0x22, 0x00, 0x01, 0x02, 0x03,   // ARR[3] reps 1 {01 02 03}
        0x06, 0x01, 0xff, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ARR[17] reps 256 {...}
        0xf7, 0x3f, 0x05, 0x80,         // OLD[1024] offs -8191 
    });
}
