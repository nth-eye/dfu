# dfu

Delta Device Firmware Upgrade codec. In case of question about protocol contact [Alari][1], and about implementation [Ilya][2].

## Guide

`using namespace dfu`

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
    case dfu::type_rep_byte:
        for (size_t i = 0; i < chunk.size; ++i)
            flash_write(&chunk.rep, 1);
    break;
    case dfu::type_rep_array:
        for (size_t i = 0; i < chunk.arr.reps; ++i)
            flash_write(chunk.arr.data, chunk.size);
    break;
    case dfu::type_old_offset: {
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
- [ ] tests
    - [ ] dec
    - [x] enc
- [ ] reamde
    - [ ] description
    - [ ] guide
    - [x] examples

[1]: https://github.com/AlariOis
[2]: https://github.com/nth-eye