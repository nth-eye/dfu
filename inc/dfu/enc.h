#ifndef DFU_ENC_H
#define DFU_ENC_H

#include "dfu/dec.h"
#include <algorithm>

namespace dfu {
namespace enc {

template<class T>
struct interface {

    // ANCHOR Helpers

    constexpr operator seq() const                      { return {buf(), buf() + idx()}; }
    constexpr seq_iter begin() const                    { return {buf(), buf() + idx()}; }
    constexpr seq_iter end() const                      { return {}; }
    constexpr const byte& operator[](size_t i) const    { return buf()[i]; }
    constexpr byte& operator[](size_t i)                { return buf()[i]; }
    constexpr const byte* data() const                  { return buf(); }
    constexpr byte* data()                              { return buf(); }
    constexpr size_t capacity() const                   { return max(); }
    constexpr size_t size() const                       { return idx(); }
    constexpr size_t resize(size_t len)                 { return len <= max() ? idx() = len : idx(); }    
    constexpr void clear()                              { idx() = 0; }

    // ANCHOR: Explicit interface

    constexpr err encode_raw(span val)
    {
        return encode_general(type_raw, val.size(), val.data(), val.size());
    }
    constexpr err encode_raw(list val)
    {
        return encode_general(type_raw, val.size(), val.begin(), val.size());
    }
    constexpr err encode_rep(byte val, size_t rep)
    {
        return encode_general(type_rep_byte, rep, &val, 1);
    }
    constexpr err encode_arr(span val, size_t rep)
    {
        if (val.empty() || !rep || rep > 0x100)
            return err_invalid_size;

        err e = encode_head(type_rep_array, val.size() - 1, val.size() + 1);
        if (e == err_ok) {
            byte nr_reps = rep - 1;
            std::copy_n(&nr_reps, 1, buf() + idx());
            idx() += 1;
            std::copy_n(val.data(), val.size(), buf() + idx());
            idx() += val.size();
            return e;
        }
        return e;
    }
    constexpr err encode_arr(list val, size_t rep)
    {
        return encode_arr(span{val.begin(), val.end()}, rep);
    }
    constexpr err encode_off(int32_t val, size_t len)
    {
        byte ai;
        if (val >= -0x20 && val < 0x20)
            ai = 0;
        else if (val >= -0x2000 && val < 0x2000)
            ai = 1;
        else if (val >= -0x200000 && val < 0x200000)
            ai = 2;
        else
            ai = 3;
        byte tmp[4] = {
            byte(((val & 0x3f) << 2) | ai),
            byte(  val >>   6),
            byte(  val >>  14),
            byte(  val >>  22),
        };
        if (val < 0)
            tmp[ai] |= 0x80;
        return encode_general(type_old_offset, len, tmp, ai + 1);
    }
private:
    constexpr err encode_head(chunk_type ct, size_t cs, size_t add_len = 0) // NOTE: add_len is only to check if enoug capacity
    {
        byte ai;

        if (cs <= 0xf)
            ai = 0;
        else if (cs <= 0xfff)
            ai = 1;
        else if (cs <= 0xfffff)
            ai = 2;
        else
            ai = 3;

        if (idx() + ai + add_len + 1 > max())
            return err_no_memory;

        buf()[idx()++] = ct | (ai << 2) | ((cs & 0xf) << 4);

        cs >>= 4;

        for (int i = 0; i < ai * 8; i += 8)
            buf()[idx()++] = cs >> i;
    
        return err_ok;
    }
    constexpr err encode_general(chunk_type ct, size_t cs, pointer data, size_t len)
    {
        if (!cs--)
            return err_invalid_size;
        err e = encode_head(ct, cs, len);
        if (e == err_ok) {
            std::copy_n(data, len, buf() + idx());
            idx() += len;
        }
        return e;
    }
private:
    constexpr auto buf() const  { return static_cast<const T*>(this)->buf; }
    constexpr auto buf()        { return static_cast<T*>(this)->buf; }
    constexpr auto max() const  { return static_cast<const T*>(this)->max; }
    constexpr auto& idx()       { return static_cast<T*>(this)->idx; }
    constexpr auto& idx() const { return static_cast<const T*>(this)->idx; }
};

/**
 * @brief Same as dfu::ref but const.
 * 
 */
struct cref : enc::interface<cref> {
    friend enc::interface<cref>;
    constexpr cref() = delete;
    constexpr cref(std::span<const byte> buf, const size_t& len) : idx{len}, max{buf.size()}, buf{buf.data()} {}
private:
    const size_t& idx;
    const size_t max;
    const byte* const buf;
};

}

/**
 * @brief Reference to DFU codec with actual storage, either dfu::view 
 * or dfu::codec<>. Use it to pass those around to common non-templated
 * functions to read and/or modify source. Unlike dfu::view, stores a 
 * reference to both memory and current size of a storage.
 * 
 */
struct ref : enc::interface<ref> {
    friend enc::interface<ref>;
    constexpr ref() = delete;
    constexpr ref(std::span<byte> buf, size_t& len) : idx{len}, max{buf.size()}, buf{buf.data()} {}
private:
    size_t& idx;
    const size_t max;
    byte* const buf;
};

/**
 * @brief Same as dfu::ref but const.
 * 
 */
using cref = const enc::cref;

/**
 * @brief DFU codec with external storage. Basically a view which allows 
 * to use writable referenced memory with codec interface. Unlike dfu::ref, 
 * stores reference only to memory, while current size is part of an instance. 
 * So passing it by value into a function and modifying there, will modify 
 * memory content but leave size unchanged.
 * 
 */
struct view : enc::interface<view> {
    friend enc::interface<view>;
    constexpr operator ref()        { return {{buf, max}, idx}; }
    constexpr operator cref() const { return {{buf, max}, idx}; }
    constexpr view() = delete;
    constexpr view(std::span<byte> buf, size_t len = 0) : idx{len}, max{buf.size()}, buf{buf.data()} {}
private:
    size_t idx;
    const size_t max;
    byte* const buf;
};

/**
 * @brief DFU codec with internal storage. Similar to dfu::view, but 
 * doesn't have overhead of an additional pointer to external storage, 
 * while still uses same CRTP codec interface. Like dfu::view, it can 
 * be cheaply passed around as dfu::ref. 
 * 
 * @tparam N Buffer size in bytes
 */
template<size_t N>
struct codec : enc::interface<codec<N>> {
    friend enc::interface<codec<N>>;
    constexpr operator ref()            { return {{buf}, idx}; }
    constexpr operator cref() const     { return {{buf}, idx}; }
    static constexpr size_t capacity()  { return N; }
private:
    size_t idx = 0;
    static constexpr size_t max = N;
    byte buf[N]{};
};

}

#endif