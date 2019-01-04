[![Build Status](https://travis-ci.org/flamewing/sorcery-xtractobb.svg?branch=master)](https://travis-ci.org/flamewing/sorcery-xtractobb)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/13717/badge.svg)](https://scan.coverity.com/projects/13717)
[![CodeFactor](https://www.codefactor.io/repository/github/flamewing/sorcery-xtractobb/badge)](https://www.codefactor.io/repository/github/flamewing/sorcery-xtractobb)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/flamewing/sorcery-xtractobb.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/flamewing/sorcery-xtractobb/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/flamewing/sorcery-xtractobb.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/flamewing/sorcery-xtractobb/context:cpp)

Inkle Sorcery! Extractor
========================
This software is capable of extracting the Android OBB files used in the Inkle Sorcery! series. It is possible that other Inkle games can also be extracted with this tool, but I did not test it.

To compile this tool you need a C++17-compatible compiler (GCC 7 is enough), as well as Boost. When you meet the requirements, run "make" and the "xtractobb" executable will be created. Its usage is:

    xtractobb <obbfile> <outputdir>

The tool will scan all files packed into the OBB and extract them into the output directory. It will also create a "SorceryN-Reference.json" file that stitches together "SorceryN.json" with the contents of "SorceryN.inkcontent".

Also provided is a "xtract_all_obbs.sh" which will extract all Sorcery! OBBs and link all JSON files for easier browsing.

TODO
====
- [ ] Create a OBB directory abstraction layer;
- [ ] Determine main story filename using "StoryFilename" and "[StoryFilename]PartNumber" properties from "Info.plist" file instead of hard-coding;
- [ ] Use "indexed-content/filename" attribute in story file to determine inkcontent file instead of hard-coding;
- [ ] Support for other Inkle games;
- [ ] Support for generating new OBB files;
- [ ] Decompile the reference file into [Ink script](https://github.com/inkle/ink);
- [ ] Use a different JSON tokenizer.
- [ ] Get Coverity working again once it supports GCC8/c++17
