#ifndef DFU_LOG_H
#define DFU_LOG_H

#include "dfu/dec.h"
#include <cctype>
#include <cstdio>

namespace dfu {
namespace log {
inline constexpr auto rst = "\u001b[0m";
inline constexpr auto blk = "\u001b[30m";
inline constexpr auto red = "\u001b[31m";
inline constexpr auto grn = "\u001b[32m";
inline constexpr auto yel = "\u001b[33m";
inline constexpr auto blu = "\u001b[34m";
inline constexpr auto mag = "\u001b[35m";
inline constexpr auto cyn = "\u001b[36m";
inline constexpr auto wht = "\u001b[37m";
}

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
        printf("%sRAW [%9lu] \n", log::grn, obj.size);
        if (obj.size)
            log_hex(obj.raw, obj.size);
    break;
    case type_rep:
        printf("%sREP [%9lu] byte 0x%02x \n", log::cyn, obj.size, obj.rep);
    break;
    case type_arr:
        printf("%sARR [%9lu] reps %lu \n", log::blu, obj.size, obj.arr.reps);
        log_hex(obj.arr.data, obj.size);
    break;
    case type_off:
        printf("%sOLD [%9lu] offs %+d \n", log::mag, obj.size, obj.off);
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
    printf("%s+-----------HEX-----------+\n", log::rst);
    log_hex(s.data(), s.size());
    printf("%s+--------DIAGNOSTIC-------+\n", log::rst);
    int i = 0; 
    for (auto it : s) {
        printf("%s| %d) ", log::rst, ++i);
        log_obj(it);
    }
    printf("%s+-------------------------+\n", log::rst);
}

}

#endif 