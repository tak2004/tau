# Package format

The findings of the [compression](compression.md), [design](design.md) and [module](module.md) documents are deciding the following package format.

The package contains the module, meta and other files. Each file can be compressed with lz4hc or stay uncompressed. The format splits the file content and the file tree information to allow a quick processing of the content without touching the file data.

## File header

The header use 8 byte alignment to support zero copy on 64bit integer processing.

Offset | Bytes | Content | Description
-|-|-|-
0 | 4 | FourCC | The value is 'taup'.
4 | 4 | Checksum | A u32 checksum of the **Package header**.
8 | 8 | Bytes | Bytes size of the **Package header** and the **Payload**.

## Package header

Offset | Bytes | Content | Description
-|-|-|-
16 | 4 | Payloads | counts the amount of payload header and payload data chunks.
20 | 12 | Reserved | This is reserved to store the payload header at byte 32.

## Payload header

The payload header is a set of arrays and the first entry is 32 byte aligned to support SIMD instruction, cacheline optimization and fixed length string support.

The data are designed as hash lookup table. The first array contains the hashes and the second list contains the values related to the hash. You can search for a specific hash and then jump to the right data entry on the value list.

## Package header

Offset | Bytes | Content | Description
-|-|-|-
32 | N*4 | Hash list | As the hashes are packed as one list it can be used with SIMD to find all potential candidates very quickly. If there are more then one hit then you need to double check the candidates in the **Payload identifier list**.
32+N*4 | N*64 | Payload infos | The payload info list contains for each hash entry one set of payload meta data. It's content is 32 byte aligned.

### Payload info list

Offset | Bytes | Content | Description
-|-|-|-
X | 32 | Name | The name of the payload which is described in detail below.
X+32 | 8 | Offset | Points to the first byte of the payload data.
X+40 | 8 | Bytes | Specifies the byte size of the payload.
X+48 | 8 | Uncompressed bytes | Specifies the byte size of the decompressed payload. If this is 0 then the file don't use compression else it contains the necessary size of the buffer for the decompression.
X+56 | 4 | Checksum | The checksum of the specified payload data.
X+60 | 4 | Reserved | Reserved for future use and providing the right alignment. It's likely that the compression algorithm would be stored here. 

#### Name

Each name is using 32 bytes in total. The first 31 bytes are used for the utf8 data and the last byte is used as a counter for unused bytes. If you use less then 31 bytes then you need to set the trailing unused bytes zero. This allows c ansi/ascii/utf8 compatible string support. As the utf8 data are stored at the beginning the SIMD support works too. 

The amount of 32 bytes is choosen because it's enough space to distinguish many files but is still small enough to fit 2 names into most CPUs cache line. Which is demanded to support string operations(e.g. comparing) on a single cache line.

If you use 31 bytes as a name then the last byte contains zero because no bytes are left and in this case it fullfill the 0 termination rule.

## Payload data

The payload is stored with 64 byte alignment to support zero copy with up to 512Bit SIMD instruction sets support.