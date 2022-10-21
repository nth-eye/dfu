#ifndef DFU_DEC_H
#define DFU_DEC_H

#include <cstdint>
#include <cstddef>
#include <span>
#include <tuple>

namespace dfu {

using byte = uint8_t;
using span = std::span<const byte>;
using list = std::initializer_list<byte>;
using pointer = const byte*;

/**
 * @brief General dfu:: codec err.
 * 
 */
enum err {
    err_ok,
    err_out_of_bounds,
    err_unreachable,
    err_no_memory,
    err_invalid_size,
};

/**
 * @brief Chunk type, 2 bits in header.
 * 
 */
enum chunk_type {
    type_raw,
    type_rep_byte,
    type_rep_array,
    type_old_offset,
    type_invalid,
};

/**
 * @brief Chunk data for repeated sections.
 * 
 */
struct array {
    byte    reps;
    pointer data;
};

/**
 * @brief General decoded chunk with type and data.
 * 
 */
struct chunk {
    constexpr chunk(chunk_type t = type_invalid) : type{t} {}
    constexpr bool valid() const { return type != type_invalid; }
    chunk_type type;
    size_t size; 
    union {
        pointer raw;
        byte rep;
        array arr;
        int32_t off;
    };
};

/**
 * @brief Decode next adjacent chunk in a given range. Includes bounds checks.
 * 
 * @param p Begin pointer, must be valid
 * @param end End pointer, must be valid
 * @return Tuple with decoded chunk, err status and pointer past last byte interpreted
 */
constexpr std::tuple<chunk, err, pointer> decode(pointer p, const pointer end)
{
    if (p >= end)
        return {{}, err_out_of_bounds, end};

    chunk cnk = chunk_type(*p & 0b0000'0011);
    auto extr =           (*p & 0b0000'1100) >> 2;
    auto size =           (*p & 0b1111'0000) >> 4;

    if (++p + extr > end)
        return {{}, err_out_of_bounds, p};

    for (int i = 4; i < extr * 8 + 4; i += 8)
        size |= int(*p++) << i;
    
    cnk.size = ++size;

    switch (cnk.type) 
    {
    case type_raw:
        if (p + size > end)
            return {{}, err_out_of_bounds, p};
        cnk.raw = p;
        p += size;
    break;
    case type_rep_byte:
        if (p > end)
            return {{}, err_out_of_bounds, p};
        cnk.rep = *p++;
    break;
    case type_rep_array:
        if (p + size >= end) // NOTE: >= because + 1 byte for reps
            return {{}, err_out_of_bounds, p};
        cnk.arr.reps = *p++ + 1;
        cnk.arr.data = p;
        p += size;
    break;
    case type_old_offset:
        extr    =  *p & 0b0000'0011;
        cnk.off = (*p & 0b1111'1100) >> 2;
        if (++p + extr > end)
            return {{}, err_out_of_bounds, p};
        for (int i = 6; i < extr * 8 + 6; i += 8)
            cnk.off |= int(*p++) << i;
        if (cnk.off >> (extr * 8 + 5))
            cnk.off -= 1 << (extr * 8 + 6);
    break;
    default:
        return {{}, err_unreachable, p};
    }
    return {cnk, err_ok, p};
}

/**
 * @brief Sequence iterator which holds range (begin and end pointers). 
 * Allows to decode adjacent chunks one by one till it reaches end. 
 * 
 */
struct seq_iter {
    constexpr seq_iter() = default;
    constexpr seq_iter(pointer head, pointer tail) : head{head}, tail{tail}
    {
        step(val);
    }
    constexpr bool operator!=(const seq_iter&) const    { return val.valid(); }
    constexpr auto& operator*() const                   { return val; }
    constexpr auto& operator++()
    {
        step(val);
        return *this;
    }
    constexpr auto operator++(int) 
    { 
        auto tmp = *this; 
        ++(*this); 
        return tmp; 
    }
private:
    constexpr void step(chunk& o) 
    {
        std::tie(o, std::ignore, head) = decode(head, tail); 
    }
private:
    pointer head = nullptr;
    pointer tail = nullptr;
    chunk val;
};

/**
 * @brief Read-only span wrapper for traversal on-the-fly using dfu::seq_iter.
 * 
 */
struct seq : span {
    using span::span;
    constexpr seq_iter begin() const    { return {data(), data() + size()}; }
    constexpr seq_iter end() const      { return {}; }
};

}

#endif