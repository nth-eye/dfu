#ifndef DFU_DEC_H
#define DFU_DEC_H

#include <cstdint>
#include <cstddef>
#include <span>
#include <tuple>

namespace dfu {

using byte = uint8_t;
using span = std::span<const byte>;
using pointer = const byte*;

enum error {
    error_ok,
    error_out_of_bounds,
    error_unreachable,
};

enum chunk_type {
    type_raw,
    type_rep_byte,
    type_rep_array,
    type_old_offset,
    type_invalid,
};

struct repeated_array {
    byte    reps;
    pointer data;
};

struct chunk {
    constexpr chunk(chunk_type t = type_invalid) : type{t} {}
    constexpr bool valid() const { return type != type_invalid; }
    chunk_type type;
    size_t size; 
    union {
        pointer raw;
        byte rep;
        repeated_array arr;
        int32_t off;
    };
};

constexpr std::tuple<chunk, error, pointer> decode(pointer p, const pointer end)
{
    if (p >= end)
        return {{}, error_out_of_bounds, end};

    chunk cnk = chunk_type(*p & 0b0000'0011);
    auto extr =           (*p & 0b0000'1100) >> 2;
    auto size =           (*p & 0b1111'0000) >> 4;

    if (++p + extr > end)
        return {{}, error_out_of_bounds, p};

    for (int i = 4; i < extr * 8 + 4; i += 8)
        size |= int(*p++) << i;
    
    cnk.size = ++size;

    switch (cnk.type) 
    {
    case type_raw:
        if (p + size > end)
            return {{}, error_out_of_bounds, p};
        cnk.raw = p;
        p += size;
    break;
    case type_rep_byte:
        if (p > end)
            return {{}, error_out_of_bounds, p};
        cnk.rep = *p++;
    break;
    case type_rep_array:
        if (p + size >= end) // NOTE: >= because + 1 byte for reps
            return {{}, error_out_of_bounds, p};
        cnk.arr.reps = *p++ + 1;
        cnk.arr.data = p;
        p += size;
    break;
    case type_old_offset:
        extr    =  *p & 0b0000'0011;
        cnk.off = (*p & 0b1111'1100) >> 2;
        if (++p + extr > end)
            return {{}, error_out_of_bounds, p};
        for (int i = 6; i < extr * 8 + 6; i += 8)
            cnk.off |= int(*p++) << i;
    break;
    default:
        return {{}, error_unreachable, p};
    }
    return {cnk, error_ok, p};
}

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

struct seq : span {
    using span::span;
    constexpr seq_iter begin() const    { return {data(), data() + size()}; }
    constexpr seq_iter end() const      { return {}; }
};

}

#endif