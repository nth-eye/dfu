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

TEST_F(Encode, Test)
{
    codec.encode_raw({0x55, 0x66, 0x77});
    codec.encode_rep(0x42, 100);
    codec.encode_rep(0x66, 1);
    codec.encode_arr({0x01, 0x02, 0x03}, 1);
    codec.encode_off(-8191, 1024);

    dfu::log_seq(codec);
}