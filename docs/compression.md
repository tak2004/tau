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
lz4hc | S | can be small if a subset is used, very fast comp/decomp, ok compression ratio

# Modules/Packages

We expect following:
* high speed disk(ssd/nvme)
* download only once and reuse
* multi core
* hot and cold cache

lz4hc is perfect for the modules if the code base can be shrunk to an acceptable state. It takes more time for compress/write compared to the other but outplays all competitors by far with read/decompress speed and the compress ratio is ok.
Brotli, lz4 and zstd are better on very slow read sources like lte, old wifi but already struggle on 100mbit ethernet and lose with slow hdd.

lz4hc can improve the read performance of content in a module by factor 4-5. The speedup increase will get even better with increasing read-speed of the storage device.

As the cold cache only happens once on rebuild/live build and continuing access reads from hot cache the compression ratio/decompression factor is important. Because if the raw memory read speed is higher then the decompressed read then no compression would benefit most of the hot cache. But lz4hc is nearly the same speed on hot cache for some benchmarks. It would be better to use the uncompressed data with mapping in this case.

But bigger projects need several modules/packages and it's more and more likely that not all files are on hot cache. With the compression we can expect 3-4 times more files in the same cache size. A single cold cache hit would compensate for 100 hot cache hits.

On rebuild/live build jobs an additional cache of the uncompressed modules/packages should be stored in the build process for full control but this don't impact the decission for the modules/packages compression.