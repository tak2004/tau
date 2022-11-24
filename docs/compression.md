# Compare the options
The goal is to find a decompressor which is as small as possible, compress very good and decompress between 10-50% of memcpy speed.

Name | Rating | Notes
-|-|-
brotli | A | to large decompressor
zstd | A | around 10% of memcpy speed
lz4 | S | can be small if a subset is used
aplib | B | tiny but very slow decompressor
lzsa | A | tiny, fast but bad compression ratio
fpaq2 | S | tiny, fast and good compression