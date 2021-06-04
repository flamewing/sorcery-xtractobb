# OGG Format Description

## Header

| Offset | Length | Content           |
|--------|--------|-------------------|
| 0      | 8      | AP_Pack!          |
| 8      | 4      | Length of ogg     |
| 12     | 4      | File table offset |

## File table

The file table consists of several 20-byte long entries, and starts aligned to a 16-bye boundary. It extends until the end of the file; all other data comes before it. The format of a file entry is:

| Relative offset | Length | Content                |
|-----------------|--------|------------------------|
| N+0             | 4      | File name offset       |
| N+4             | 4      | Length of file name    |
| N+8             | 4      | File data offset       |
| N+12            | 4      | Compressed file size   |
| N+16            | 4      | Uncompressed file size |

Compressed files are raw zlib-compressed binary files. All files are stored aligned to a 16-byte boundary, with null byte padding.

## File names

File names are stored without null terminators in a contiguous block of data containing all names. This block of data starts aligned to a 16-byte boundary, with null byte padding. The list of file names is immediately before the file table.

## inkcontent, reference

The Sorcery`N`.inkcontent file is a sequence of LF-separated pieces of JSON with as little white-space as possible. The Sorcery`N`.json file, in the "indexed-content" object: this object is structured as follows:

```JSON
    "indexed-content": {
        "filename": "SorceryN.inkcontent",
        "ranges": {
            "<stitch-name>": "<start-offset> <length>",
```

where:

| Field                | Meaning                                                    |
|----------------------|------------------------------------------------------------|
| &lt;start-offset&gt; | Start of the stitch, as an offset into the inkcontent file |
| &lt;length&gt;       | Length of the stitch, including ending LF                  |

They can be conbined into the reference file by replacing the "indexed-content" object with the "stitches" object:

```JSON
    "stitches": {
    "<stitch-name1>": {
        "content": <stitch-inkcontent1>
        },
    "<stitch-name2>": {
        <stitch-inkcontent2>
        },
```

depending on whether the inkcontent is a JSON array (`stitch-inkcontent1`) or a JSON object (`stitch-inkcontent2`).
