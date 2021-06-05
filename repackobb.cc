/*
 *	Copyright © 2016 Flamewing <flamewing.sonic@gmail.com>
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fileentry.hh"
#include "jsont.hh"
#include "prettyJson.hh"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/filter/aggregate.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/vector.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using std::allocator;
using std::array;
using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::flush;
using std::ios;
using std::istream;
using std::numeric_limits;
using std::ostream;
using std::regex;
using std::regex_match;
using std::streamsize;
using std::string;
using std::string_view;
using std::stringstream;
using std::tuple;
using std::unordered_map;
using std::vector;

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

using boost::archive::text_iarchive;
using boost::filesystem::ifstream;
using boost::filesystem::ofstream;
using boost::filesystem::path;
using boost::iostreams::aggregate_filter;
using boost::iostreams::filtering_ostream;
using boost::iostreams::zlib_compressor;
namespace zlib = boost::iostreams::zlib;

using ibufferstream = boost::interprocess::basic_ibufferstream<char>;

// Redefine assert macro to avoid clang-tidy noise.
#ifndef __MINGW64__
#    undef assert
#    define assert(expr)                           \
        ((expr) ? void(0)                          \
                : __assert_fail(                   \
                        #expr, __FILE__, __LINE__, \
                        __ASSERT_FUNCTION))    // NOLINT
#endif

// Sorcery! JSON stitch filter for boost::filtering_ostream
template <typename Ch, typename Alloc = std::allocator<Ch>>
class basic_json_unstitch_filter : public aggregate_filter<Ch, Alloc> {
private:
    using base_type   = aggregate_filter<Ch, Alloc>;
    using vector_type = typename base_type::vector_type;

public:
    using char_type = typename base_type::char_type;
    using category  = typename base_type::category;

    explicit basic_json_unstitch_filter(
            filtering_ostream& _inkContent, string _inkFileName)
            : inkContent(_inkContent), inkFileName(std::move(_inkFileName)) {}

private:
    auto printValueRaw(ostream& sint, jsont::Tokenizer& reader)
            -> decltype(auto) {
        return sint << reader.dataValue();
    }

    auto printValueObject(ostream& sint, jsont::Tokenizer& reader)
            -> decltype(auto) {
        return sint << reader.dataValue() << ':';
    }

    void handleObjectOrStitch(vectorstream& sint, jsont::Tokenizer& reader) {
        if (reader.dataValue() != R"("stitches")"sv) {
            printValueObject(sint, reader);
            return;
        }
        // Temporary storage for stitches, so we can get the offset and length
        // for the story file.
        stringstream stitches(ios::in | ios::out | ios::binary);
        auto         curr_position = stitches.tellp();
        sint << R"("indexed-content":)"sv;
        jsont::Token tok = reader.next();
        assert(tok == jsont::ObjectStart);
        printValueRaw(sint, reader);
        sint << R"("filename":")"sv << inkFileName << R"(","ranges":{)";
        tok = reader.next();
        while (tok != jsont::ObjectEnd) {
            assert(tok == jsont::FieldName);
            curr_position = stitches.tellp();
            printValueObject(sint, reader)
                    << '"' << uint32_t(curr_position) << ' ';
            tok = reader.next();
            assert(tok == jsont::ObjectStart);
            // Handle "content" arrays seperately.
            tok = reader.next();
            if (tok == jsont::FieldName
                && reader.dataValue() == R"("content")"sv) {
                tok = reader.next();
                assert(tok == jsont::ArrayStart);
                printJSON(reader, stitches, eNO_WHITESPACE, ~0U);
                tok = reader.current();
                assert(tok == jsont::ObjectEnd);
                stitches << '\n';
            } else {
                // We have an object on the ink file.
                // Need to print the open curly brace.
                stitches << '{';
                printJSON(reader, stitches, eNO_WHITESPACE, ~0U);
                tok = reader.current();
                assert(tok == jsont::ObjectEnd);
                stitches << "}\n";
            }
            auto end_position = stitches.tellp();
            sint << uint32_t(end_position - curr_position) << '"';
            tok = reader.next();
            if (tok == jsont::Comma) {
                printValueRaw(sint, reader);
                tok = reader.next();
            }
        }
        // This closes the "ranges" object.
        assert(tok == jsont::ObjectEnd);
        printValueRaw(sint, reader);
        // This closes the "indexed-content" object.
        printValueRaw(sint, reader);
        // Write out inkcontent file.
        stitches.seekg(0);
        inkContent << stitches.rdbuf();
    }

    void do_filter(vector_type const& src, vector_type& dest) final {
        vectorstream sint(ios::in | ios::out | ios::binary);
        sint.reserve(src.size() * 3 / 2);
        jsont::Tokenizer reader(src.data(), src.size());
        jsont::Token     tok = reader.current();
        while (true) {
            if (tok == jsont::FieldName) {
                handleObjectOrStitch(sint, reader);
            } else if (tok == jsont::Error || tok == jsont::End) {
                if (tok == jsont::Error) {
                    cerr << reader.errorMessage() << endl;
                }
                sint.swap_vector(dest);
                return;
            } else {
                printValueRaw(sint, reader);
            }
            tok = reader.next();
        }
        __builtin_unreachable();
    }
    filtering_ostream& inkContent;
    string             inkFileName;
};
// NOLINTNEXTLINE(modernize-use-trailing-return-type)
BOOST_IOSTREAMS_PIPABLE(basic_json_unstitch_filter, 2)

using json_unstitch_filter  = basic_json_unstitch_filter<char>;
using wjson_unstitch_filter = basic_json_unstitch_filter<wchar_t>;

enum ErrorCodes {
    eOK,
    eWRONG_ARGC,
    eOBB_NOT_FILE,
    eOBB_NO_ACCESS,
    eINPUT_NOT_DIR,
    eINPUT_NO_ACCESS,
    eINPUT_NO_FILE_TABLE,
    eINPUT_FILES_MISSING,
    eINPUT_FILES_NOT_VALID
};

[[nodiscard]] auto openObbFile(path const& obbfile) {
    if (exists(obbfile)) {
        if (!is_regular_file(obbfile)) {
            cerr << "Path "sv << obbfile
                 << " already exists, and is not a file!"sv << endl
                 << endl;
            throw ErrorCodes{eOBB_NOT_FILE};
        }
        if (!remove(obbfile)) {
            cerr << "Could not delete pre-existing file "sv << obbfile << "!"sv
                 << endl
                 << endl;
            throw ErrorCodes{eOBB_NO_ACCESS};
        }
    }

    std::unique_ptr fout
            = std::make_unique<ofstream>(obbfile, ios::out | ios::binary);
    if (!fout->good()) {
        cerr << "Could not open output file "sv << obbfile << "!"sv << endl
             << endl;
        throw ErrorCodes{eOBB_NO_ACCESS};
    }

    (*fout) << "AP_Pack!"sv;
    return fout;
}

void checkFile(path const& fpath) {
    if (!exists(fpath)) {
        cerr << "Input file "sv << fpath
             << " does not exist! Please re-dump the OBB."sv << endl
             << endl;
        throw ErrorCodes{eINPUT_FILES_MISSING};
    }
    if (!is_regular_file(fpath)) {
        cerr << "Input "sv << fpath
             << " is not a file! Please re-dump the OBB."sv << endl
             << endl;
        throw ErrorCodes{eINPUT_FILES_NOT_VALID};
    }
    ifstream fin(fpath, ios::in | ios::binary);
    if (!fin.good()) {
        cerr << "Input file "sv << fpath
             << " is not readable! Please re-dump the OBB."sv << endl
             << endl;
        throw ErrorCodes{eINPUT_NO_ACCESS};
    }
}

[[nodiscard]] auto readInputDir(path const& indir)
        -> tuple<vector<RFile_entry>, string, string, string> {
    if (!exists(indir)) {
        cerr << "Input path "sv << indir << " does not exist!"sv << endl
             << endl;
        throw ErrorCodes{eINPUT_NO_ACCESS};
    }
    if (!is_directory(indir)) {
        cerr << "Path "sv << indir << " must be a directory!"sv << endl << endl;
        throw ErrorCodes{eINPUT_NOT_DIR};
    }
    // Read file list.
    path const filetable(indir / "FileTable.ser");
    checkFile(filetable);
    vector<RFile_entry> entries;
    {
        ifstream      file_table(indir / "FileTable.ser");
        text_iarchive ia(file_table);
        ia >> entries;
    }

    // TODO: Main json file should be found from Info.plist file:
    //  main json filename = dict["StoryFilename"sv] + ".json"
    static regex const mainJsonRegex(R"regex(Sorcery\d\.(min)?json)regex"s);
    // TODO: inkcontent filename should be found from main json:
    // inkcontent filename = indexed-content/filename
    static regex const inkContentRegex(R"regex(Sorcery\d\.inkcontent)regex"s);

    string referenceFileName;
    string mainJsonFileName;
    string inkContentFileName;

    for (auto& entry : entries) {
        checkFile(indir / entry.name());
        string_view fname = entry.name();
        if (regex_match(fname.cbegin(), fname.cend(), mainJsonRegex)) {
            referenceFileName = fname.substr(0, "SorceryN"sv.size());
            referenceFileName += "-Reference.json"s;
            mainJsonFileName = fname;
            checkFile(indir / referenceFileName);
        } else if (regex_match(fname.cbegin(), fname.cend(), inkContentRegex)) {
            inkContentFileName = fname;
        }
    }
    return {entries, referenceFileName, mainJsonFileName, inkContentFileName};
}

inline auto roundUp(uint32_t numToRound, uint32_t multiple) -> uint32_t {
    assert(multiple);
    return ((numToRound + multiple - 1) / multiple) * multiple;
}

auto encodeFile(ofstream& obbContents, path const& infile, bool compressed)
        -> tuple<uint32_t, uint32_t, uint32_t> {
    path const parentdir(infile.parent_path());
    // Sanity check; if someone else is modifying the input directory as we
    // process the files, we should stop.
    assert(exists(infile));

    size_t     fulllength = file_size(infile);
    bool const isJson     = infile.extension() == ".json"s
                        || infile.extension() == ".inkcontent"s;

    stringstream sint(ios::in | ios::out | ios::binary);

    {
        ifstream fin(infile, ios::in | ios::binary);
        // Sanity check; if someone else is modifying the input directory as we
        // process the files, we should stop.
        assert(fin.good());

        {
            filtering_ostream fsout;
            if (isJson) {
                fsout.push(json_filter(eNO_WHITESPACE, &fulllength));
            }
            if (compressed) {
                fsout.push(zlib_compressor(
                        zlib::best_compression, 1 * 1024 * 1024));
            }
            fsout.push(sint);
            fsout << fin.rdbuf();
        }
    }

    sint.seekg(0);
    obbContents << sint.rdbuf();

    sint.seekg(0);
    sint.ignore(numeric_limits<streamsize>::max());
    uint32_t const complength = sint.gcount();

    uint32_t const padding = roundUp(complength, 16U) - complength;
    constexpr static const array<char, 16U> nullPadding{};
    obbContents.write(nullPadding.data(), padding);

    return {fulllength, complength, padding};
}

auto writeJSON(
        path const& fpath, ofstream& fout, filtering_ostream& fsout,
        json_unstitch_filter* fsink) {
    fout.open(fpath, ios::in | ios::binary);
    if (!fout.good()) {
        cerr << "Could not create file "sv << fpath << endl << endl;
        throw ErrorCodes{eINPUT_NO_ACCESS};
    }
    if (fsink != nullptr) {
        fsout.push(*fsink);
    }
    fsout.push(json_filter(ePRETTY));
    fsout.push(fout);
}

void unpackReferenceFile(
        path const& indir, string const& referenceFile,
        string const& mainJsonFile, string const& inkcontentFile) {
    cout << "\33[2K\rRe-generating "sv << inkcontentFile << " and "sv
         << mainJsonFile << " from reference file "sv << referenceFile
         << "... "sv << flush;
    ofstream          inkfile;
    filtering_ostream fsinkfile;
    writeJSON(indir / inkcontentFile, inkfile, fsinkfile, nullptr);

    ofstream             mainfile;
    filtering_ostream    fsmainfile;
    json_unstitch_filter filter(fsinkfile, inkcontentFile);
    writeJSON(indir / mainJsonFile, mainfile, fsmainfile, &filter);

    ifstream reffile(indir / referenceFile, ios::in | ios::binary);

    fsmainfile << reffile.rdbuf();
    cout << "done."sv << flush;
}

extern "C" auto main(int argc, char* argv[]) -> int;

auto main(int argc, char* argv[]) -> int {
    try {
        if (argc != 3) {
            cerr << "Usage: "sv << argv[0] << " inputdir outputfile"sv << endl
                 << endl;
            return eWRONG_ARGC;
        }

        path const indir(argv[1]);
        auto [entries, referenceFile, mainJsonFile, inkcontentFile]
                = readInputDir(indir);

        path const obbfile(argv[2]);
        auto       obbptr      = openObbFile(obbfile);
        auto&      obbcontents = *obbptr;

        uint32_t curr_offset = 8;
        auto     curr_pos    = obbcontents.tellp();
        Write4(obbcontents, 0U);    // Placeholder for file size
        Write4(obbcontents, 0U);    // Placeholder for file table position
        curr_offset += 8;

        unpackReferenceFile(indir, referenceFile, mainJsonFile, inkcontentFile);

        for (auto& elem : entries) {
            cout << "\33[2K\rPacking file "sv << elem.name() << flush;
            path infile(indir / elem.name());
            auto [file_fulllength, file_complength, file_padding]
                    = encodeFile(obbcontents, infile, elem.compressed);
            elem.fdata = {curr_offset, file_fulllength, file_complength};
            curr_offset += file_complength + file_padding;
        }

        cout << endl;
        cout << "\33[2K\rCreating name table... "sv << flush;
        unordered_map<string, uint32_t> nameOffsets;
        for (auto& elem : entries) {
            string const& fname = elem.fname;
            nameOffsets[fname]  = curr_offset;
            curr_offset += fname.size();
            obbcontents.write(
                    fname.data(), static_cast<uint32_t>(fname.size()));
        }
        cout << "done."sv << endl;

        uint32_t const padding = roundUp(curr_offset, 16U) - curr_offset;
        constexpr static const array<char, 16U> nullPadding{};
        obbcontents.write(nullPadding.data(), padding);
        curr_offset += padding;

        // File table is sorted by name.
        sort(entries.begin(), entries.end(), [](auto& lhs, auto& rhs) {
            return lhs.name() < rhs.name();
        });

        cout << "\33[2K\rCreating file table... "sv << flush;
        uint32_t file_table_pos = curr_offset;
        for (auto& elem : entries) {
            string const& fname = elem.fname;
            Write4(obbcontents, nameOffsets[fname]);
            Write4(obbcontents, fname.size());
            File_data const& fdata = elem.fdata;
            Write4(obbcontents, fdata.offset);
            Write4(obbcontents, fdata.complength);
            Write4(obbcontents, fdata.fulllength);
            curr_offset += 20;
        }

        // Fill in the file size and file table offset
        obbcontents.seekp(curr_pos);
        Write4(obbcontents, curr_offset);
        Write4(obbcontents, file_table_pos);
        cout << "done."sv << endl;
    } catch (exception const& except) {
        cerr << except.what() << endl;
    } catch (ErrorCodes err) {
        return err;
    }
    return eOK;
}
