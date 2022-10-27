# dfu

Delta Device Firmware Upgrade codec. In case of question about protocol contact [Alari][1], and about implementation [Ilya][2].

## Guide

`using namespace dfu`

Decoder toolset consists of `decode()`, which returns decoded `chunk`, status `err`, and pointer to next byte past last interpreted. For convenient use in range-based for loop there is `seq` wrapper, which decodes adjacent items in a sequence one by one. Range safely stops at anything invalid. Encoder can be created with memory provided by user as `view`, or self-contained template as `codec<>`. To pass either of those to handler functions use `ref` and `cref`. All these classes provide same functionality through CRTP base class, so no overhead of virtual function calls, and no unnecessary pointer to self-contained memory for `codec<>`. Both de/encoder are fully `constexpr`.

## Examples

### Encode 

```cpp
const uint8_t test[17] = {0xde, 0xad, 0xbe, 0xef};

dfu::codec<99> codec;

codec.encode_raw({0x55, 0x66, 0x77});
codec.encode_rep(0x42, 100);
codec.encode_rep(0x66, 1);
codec.encode_arr({0x01, 0x02, 0x03}, 1);
codec.encode_arr(test, 256);
codec.encode_off(-8191, 1024);

dfu::log_seq(codec);
```
> \>\>\>
```
+-----------HEX-----------+
| 20 55 66 77 35 06 42 01  66 22 00 01 02 03 06 01  | Ufw5.B.f"......|
| ff de ad be ef 00 00 00  00 00 00 00 00 00 00 00  |................|
| 00 00 f7 3f 05 80                                 |...?............|
+--------DIAGNOSTIC-------+
| 1) RAW [        3] 
| 55 66 77                                          |Ufw.............|
| 2) REP [      100] byte 0x42 
| 3) REP [        1] byte 0x66 
| 4) ARR [        3] reps 1 
| 01 02 03                                          |................|
| 5) ARR [       17] reps 256 
| de ad be ef 00 00 00 00  00 00 00 00 00 00 00 00  |................|
| 00                                                |................|
| 6) OLD [     1024] offs -8191 
+-------------------------+
```

### Decode 

```cpp
const uint8_t data[] = {...};
const size_t requested_address = 0x4000;

for (auto chunk : dfu::seq{data}) {
    switch (chunk.type)
    {
    case dfu::type_raw:
        flash_write(chunk.raw, chunk.size);
    break;
    case dfu::type_rep:
        for (size_t i = 0; i < chunk.size; ++i)
            flash_write(&chunk.rep, 1);
    break;
    case dfu::type_arr:
        for (size_t i = 0; i < chunk.arr.reps; ++i)
            flash_write(chunk.arr.data, chunk.size);
    break;
    case dfu::type_off: {
        static uint8_t tmp[512];
        size_t processed = 0;
        size_t base_addr = chunk.off + int(requested_address);
        while (processed < chunk.size) {
            auto len = MIN(chunk.size - processed, sizeof(tmp));
            flash_read_old_fw(base_addr + processed, tmp, len);
            flash_write(tmp, len);
            processed += len;
        }
    }
    break;
    default:
        puts("failure: invalid type");
        return;
    }
}
```

## TODO

- [x] source
    - [x] dec
    - [x] enc
- [x] tests
    - [x] dec
    - [x] enc
- [x] reamde
    - [x] intro
    - [x] guide
    - [x] examples

[1]: https://github.com/AlariOis
[2]: https://github.com/nth-eye