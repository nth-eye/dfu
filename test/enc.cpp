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
    static const uint8_t test_1[1]  = {0x42};
    static const uint8_t test_2[17] = {0xde, 0xad, 0xbe, 0xef};

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
    dfu::log_seq(codec);

    check(codec, {});
}

TEST_F(Encode, RepeatedArray)
{
    check(codec, {});
}

TEST_F(Encode, OldOffset)
{
    check(codec, {});
}

TEST_F(Encode, Mixed)
{
    codec.encode_raw({0x55, 0x66, 0x77});
    codec.encode_rep(0x42, 100);
    codec.encode_rep(0x66, 1);
    codec.encode_arr({0x01, 0x02, 0x03}, 1);
    codec.encode_off(-8191, 1024);

    // dfu::log_seq(codec);

    // check(codec, {

    // });
}

TEST_F(Encode, Failures)
{
    check(codec, {});
}