Header
======

| Offset | Length | Content           |
|--------|--------|-------------------|
| 0      | 8      | AP_Pack!          |
| 8      | 4      | Length of ogg     |
| 12     | 4      | File table offset |

File table
==========

The file table consists of several 20-byte long entries. The format of these entries is as follows:

| Relative offset | Length | Content                |
|-----------------|--------|------------------------|
| N+0             | 4      | File name offset       |
| N+4             | 4      | Length of file name    |
| N+8             | 4      | File data offset       |
| N+12            | 4      | Compressed file size   |
| N+16            | 4      | Uncompressed file size |

Compressed files are raw zlib-compressed binary files.

