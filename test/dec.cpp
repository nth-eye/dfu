#include <gtest/gtest.h>
#include "dfu/dec.h"
#include "dfu/log.h"

using namespace dfu;

class Decode : public ::testing::Test {
protected:
    void TearDown() override 
    {
        ASSERT_EQ(ptr, end);
    }
    void check(pointer exp_ptr, chunk_type exp_type, size_t exp_size)
    {
        std::tie(c, e, ptr) = decode(ptr, end);
        ASSERT_EQ(e, err_ok);
        ASSERT_EQ(ptr, exp_ptr);
        ASSERT_EQ(c.type, exp_type);
        ASSERT_EQ(c.size, exp_size);
    }
    void check_raw(pointer exp_ptr, pointer exp_raw, size_t exp_size) 
    {
        check(exp_ptr, type_raw, exp_size);
        ASSERT_EQ(c.raw, exp_raw);
    }
    void check_rep(pointer exp_ptr, byte exp_rep, size_t exp_size) 
    {
        check(exp_ptr, type_rep, exp_size);
        ASSERT_EQ(c.rep, exp_rep);
    }
    void check_arr(pointer exp_ptr, pointer exp_data, size_t exp_size, size_t exp_reps) 
    {
        check(exp_ptr, type_arr, exp_size);
        ASSERT_EQ(c.arr.reps, exp_reps);
        ASSERT_EQ(c.arr.data, exp_data);
    }
    void check_off(pointer exp_ptr, size_t exp_size, int32_t exp_off) 
    {
        check(exp_ptr, type_off, exp_size);
        ASSERT_EQ(c.off, exp_off);
    }
protected:
    err e;
    chunk c;
    pointer ptr;
    pointer end;
};

TEST_F(Decode, DefaultChunk)
{
    static_assert(chunk{}.type == type_invalid);
    static_assert(chunk{}.valid() == false);
    
    ASSERT_EQ(chunk{}.type, type_invalid);
    ASSERT_EQ(chunk{}.valid(), false);
}

TEST_F(Decode, Raw)
{
    const byte test[] = { 
        0x00, 0x00, // RAW[1]
        0x20, 0x00, 0x11, 0x22, // RAW[3]
        0xf0, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, // RAW[16]
        0x00, 0x42, // RAW[1]
        0x04, 0x01, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // RAW[17]
    };
    ptr = test;
    end = test + sizeof(test);

    check_raw(test + 2, test + 1, 1);
    check_raw(test + 6, test + 3, 3);
    check_raw(test + 23, test + 7, 16);
    check_raw(test + 25, test + 24, 1);
    check_raw(test + 44, test + 27, 17);
}

TEST_F(Decode, RepeatedByte)
{
    const byte test[] = {
        0x01, 0x2a, // REP[1]
        0x21, 0x11, // REP[3]
        0xf1, 0xff, // REP[16]
        0x05, 0x01, 0x55, // REP[17]
        0xf5, 0xff, 0x66, // REP[4096]
        0x09, 0x00, 0x01, 0x00, // REP[4097]
        0xf9, 0xff, 0xff, 0x44, // REP[1048576]
        0x0d, 0x00, 0x00, 0x01, 0x69, // REP[1048577]
        0xfd, 0xff, 0xff, 0xff, 0x77, // REP[268435456]
    };
    ptr = test;
    end = test + sizeof(test);

    check_rep(test + 2, 0x2a, 1);
    check_rep(test + 4, 0x11, 3);
    check_rep(test + 6, 0xff, 16);
    check_rep(test + 9, 0x55, 17);
    check_rep(test + 12, 0x66, 4096);
    check_rep(test + 16, 0x00, 4097);
    check_rep(test + 20, 0x44, 1048576);
    check_rep(test + 25, 0x69, 1048577);
    check_rep(test + 30, 0x77, 268435456);
}

TEST_F(Decode, RepeatedArray)
{
    const byte test[] = {
        0x02, 0x00, 0x00, // ARR[1]
        0x02, 0x02, 0x00, // ARR[1]
        0x22, 0x00, 0x00, 0x11, 0x22, // ARR[3]
        0x22, 0x01, 0x00, 0x11, 0x22, // ARR[3]
        0x02, 0x0f, 0x42, // ARR[1]
        0x02, 0xfe, 0x42, // ARR[1]
        0x06, 0x01, 0x10, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ARR[17]
        0x06, 0x01, 0xff, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ARR[17]
    };
    ptr = test;
    end = test + sizeof(test);

    check_arr(test + 3, test + 2, 1, 1);
    check_arr(test + 6, test + 5, 1, 3);
    check_arr(test + 11, test + 8, 3, 1);
    check_arr(test + 16, test + 13, 3, 2);
    check_arr(test + 19, test + 18, 1, 16);
    check_arr(test + 22, test + 21, 1, 255);
    check_arr(test + 42, test + 25, 17, 17);
    check_arr(test + 62, test + 45, 17, 256);
}

TEST_F(Decode, OldOffset)
{
    const byte test[] = {
        0x03, 0x00, // OLD[1] offs +0
        0xf3, 0x7c, // OLD[16] offs +31
        0xf7, 0xff, 0xfd, 0x7f, // OLD[4096] +8191
        0xfb, 0xff, 0xff, 0xfe, 0xff, 0x7f, // OLD[1048576] +2097151
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, // OLD[268435456] +536870911
        0xf3, 0x80, // OLD[16] offs -32
        0xf7, 0xff, 0x01, 0x80, // OLD[4096] -8192
        0xfb, 0xff, 0xff, 0x02, 0x00, 0x80, // OLD[1048576] -2097152
        0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x00, 0x80, // OLD[268435456] -536870912
    };
    ptr = test;
    end = test + sizeof(test);

    check_off(test + 2, 1, 0);
    check_off(test + 4, 16, 31);
    check_off(test + 8, 4096, 8191);
    check_off(test + 14, 1048576, 2097151);
    check_off(test + 22, 268435456, 536870911);
    check_off(test + 24, 16, -32);
    check_off(test + 28, 4096, -8192);
    check_off(test + 34, 1048576, -2097152);
    check_off(test + 42, 268435456, -536870912);
}

TEST_F(Decode, Mixed)
{
    const byte test[] = {
        0x20, 0x55, 0x66, 0x77,         // RAW[3] {55 66 77}
        0x35, 0x06, 0x42,               // REP[100] byte 0x42
        0x01, 0x66,                     // REP[1] byte 0x66 
        0x22, 0x00, 0x01, 0x02, 0x03,   // ARR[3] reps 1 {01 02 03}
        0x06, 0x01, 0xff, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ARR[17] reps 256 {...}
        0xf7, 0x3f, 0x05, 0x80,         // OLD[1024] offs -8191 
    };
    ptr = test;
    end = test + sizeof(test);

    check_raw(test + 4, test + 1, 3);
    check_rep(test + 7, 0x42, 100);
    check_rep(test + 9, 0x66, 1);
    check_arr(test + 14, test + 11, 3, 1);
    check_arr(test + 34, test + 17, 17, 256);
    check_off(test + 38, 1024, -8191);
}

TEST_F(Decode, Constexpr)
{
    static constexpr const byte test[] = {
        0x20, 0x55, 0x66, 0x77,         // RAW[3] {55 66 77}
        0x35, 0x06, 0x42,               // REP[100] byte 0x42
        0x01, 0x66,                     // REP[1] byte 0x66 
        0x22, 0x00, 0x01, 0x02, 0x03,   // ARR[3] reps 1 {01 02 03}
        0x06, 0x01, 0xff, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ARR[17] reps 256 {...}
        0xf7, 0x3f, 0x05, 0x80,         // OLD[1024] offs -8191 
    };
    static constexpr auto cnt = [&]()
    {
        int i = 0;
        for ([[maybe_unused]] auto it : seq{test}) {
            ++i;
        }
        return i;
    }();
    static_assert(6 == cnt);
}

TEST_F(Decode, Failures)
{
    std::array<byte, 1> test_1 = { 0x00 };
    std::array<byte, 3> test_2 = { 0x0c, 0x00, 0x01 };
    std::array<byte, 4> test_3 = { 0x04, 0x01, 0xde, 0xad };
    std::array<byte, 4> test_4 = { 0x0d, 0x00, 0x00, 0x01 };
    std::array<byte, 4> test_5 = { 0x22, 0x00, 0x00, 0x11 };
    std::array<byte, 2> test_6 = { 0x02, 0x00 };
    std::array<byte, 1> test_7 = { 0xf3 };
    std::array<byte, 4> test_8 = { 0xfd, 0xff, 0xff, 0xff };

    std::tie(c, e, ptr) = decode(test_1.begin(), test_1.begin());

    ASSERT_EQ(e, err_out_of_bounds);
    ASSERT_EQ(ptr, test_1.begin());
    ASSERT_EQ(c.type, type_invalid);

    std::tie(c, e, ptr) = decode(test_2.begin(), test_2.end());

    ASSERT_EQ(e, err_out_of_bounds);
    ASSERT_EQ(ptr, test_2.begin() + 1);
    ASSERT_EQ(c.type, type_invalid);

    std::tie(c, e, ptr) = decode(test_3.begin(), test_3.end());

    ASSERT_EQ(e, err_out_of_bounds);
    ASSERT_EQ(ptr, test_3.begin() + 2);
    ASSERT_EQ(c.type, type_invalid);

    std::tie(c, e, ptr) = decode(test_4.begin(), test_4.end());

    ASSERT_EQ(e, err_out_of_bounds);
    ASSERT_EQ(ptr, test_4.begin() + 1);
    ASSERT_EQ(c.type, type_invalid);

    std::tie(c, e, ptr) = decode(test_5.begin(), test_5.end());

    ASSERT_EQ(e, err_out_of_bounds);
    ASSERT_EQ(ptr, test_5.begin() + 1);
    ASSERT_EQ(c.type, type_invalid);

    std::tie(c, e, ptr) = decode(test_6.begin(), test_6.end());

    ASSERT_EQ(e, err_out_of_bounds);
    ASSERT_EQ(ptr, test_6.begin() + 1);
    ASSERT_EQ(c.type, type_invalid);

    std::tie(c, e, ptr) = decode(test_7.begin(), test_7.end());

    ASSERT_EQ(e, err_out_of_bounds);
    ASSERT_EQ(ptr, test_7.begin() + 1);
    ASSERT_EQ(c.type, type_invalid);

    std::tie(c, e, ptr) = decode(test_8.begin(), test_8.end());

    ASSERT_EQ(e, err_out_of_bounds);
    ASSERT_EQ(ptr, test_8.begin() + 1);
    ASSERT_EQ(c.type, type_invalid);

    ptr = end = nullptr;
}