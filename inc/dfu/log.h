#ifndef DFU_LOG_H
#define DFU_LOG_H

#include "dfu/dfu.h"
#include <cctype>
#include <cstdio>

namespace dfu {

/**
 * @brief Convert integer to hexadecimal character.
 * 
 * @param bin Byte value
 * @return Hexadecimal character 
 */
constexpr char bin_to_char(uint8_t bin)
{
    return "0123456789abcdef"[bin & 0xf];
}

/**
 * @brief Print hex nicely with relevant ASCII representation.
 * 
 * @param dat Data to print
 * @param len Length in bytes
 */
inline void log_hex(const void *dat, size_t len)
{
    if (!dat || !len)
        return;

    auto p = static_cast<const uint8_t*>(dat);

    for (size_t i = 0; i < len; ++i) {

        if (!(i & 15)) {
            putchar('|');
            putchar(' ');
        }
        putchar(bin_to_char(p[i] >> 4));
        putchar(bin_to_char(p[i] & 0xF));
        putchar(' ');
        
        if ((i & 7) == 7)
            putchar(' ');

        if ((i & 15) == 15) {
            putchar('|');
            for (int j = 15; j >= 0; --j) {
                char c = p[i - j];
                putchar(isprint(c) ? c : '.');
            }
            putchar('|');
            putchar('\n');
        }
    }
    int rem = len - ((len >> 4) << 4);
    if (rem) {
        for (int j = (16 - rem) * 3 + ((~rem & 8) >> 3); j >= 0; --j)
            putchar(' ');
        putchar('|');
        for (int j = rem; j; --j) {
            char c = p[len - j];
            putchar(isprint(c) ? c : '.');
        }
        for (int j = 0; j < 16 - rem; ++j)
            putchar('.');
        putchar('|');
        putchar('\n');
    }
}

inline void log_obj(const chunk& obj)
{
    switch (obj.type) 
    {
    case type_raw:
        printf("raw[%lu] \n", obj.size);
        if (obj.size)
            log_hex(obj.raw, obj.size);
    break;
    case type_rep_byte:
        printf("rep[%lu] byte 0x%02x \n", obj.size, obj.rep);
    break;
    case type_rep_array:
        printf("arr[%lu] reps %u \n", obj.size, obj.arr.reps);
        log_hex(obj.arr.data, obj.size);
    break;
    case type_old_offset:
        printf("old[%lu] offset %d \n", obj.size, obj.off);
    break;
    case type_invalid:
        printf("<invalid> \n");
    break;
    default: 
        printf("<unknown> \n");
    }
}

inline void log_seq(const seq& s)
{
    printf("+-----------HEX-----------+\n");
    log_hex(s.data(), s.size());
    printf("+--------DIAGNOSTIC-------+\n");
    int i = 0; 
    for (auto it : s) {
        printf("| %d) ", ++i);
        log_obj(it);
    }
    printf("+-------------------------+\n");
}

}

#endif 