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

#include "fileentry.hh"
#include "jsont.hh"
#include "prettyJson.hh"

using std::allocator;
using std::array;
using std::cerr;
using std::cout;
using std::endl;
using std::flush;
using std::ios;
using std::istream;
using std::ostream;
using std::regex;
using std::regex_match;
using std::string;
using std::string_view;
using std::stringstream;
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
#    define assert(expr)                                                       \
        ((expr) ? void(0)                                                      \
                : __assert_fail(                                               \
                      #expr, __FILE__, __LINE__, __ASSERT_FUNCTION)) // NOLINT
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

    // TODO: Filter should receive output directory instead.
    explicit basic_json_unstitch_filter(string_view const _inkContent)
        : inkContent(_inkContent) {}

private:
    decltype(auto) printValueRaw(vectorstream& sint, jsont::Tokenizer& reader) {
        return sint << reader.dataValue();
    }

    decltype(auto)
    printValueObject(vectorstream& sint, jsont::Tokenizer& reader) {
        return sint << reader.dataValue() << ':';
    }

    void handleObjectOrStitch(vectorstream& sint, jsont::Tokenizer& reader) {
        if (reader.dataValue() != R"("indexed-content")"sv) {
            printValueObject(sint, reader);
            return;
        }
        sint << R"("stitches":)"sv;
        jsont::Token tok = reader.next();
        assert(tok == jsont::ObjectStart);
        printValueRaw(sint, reader);
        tok = reader.next();
        while (tok != jsont::ObjectEnd) {
            assert(tok == jsont::FieldName);
            if (reader.dataValue() == R"("filename")"sv) {
                // TODO: instead of being discarded, this should be used with
                // output directoty to open stitch source file
                tok = reader.next(); // Fetch filename...
                assert(tok == jsont::String);
                tok = reader.next(); // ... and discard it
                assert(tok == jsont::Comma);
                tok = reader.next(); // Discard comma after it as well
            } else if (reader.dataValue() == R"("ranges")"sv) {
                // The meat.
                tok = reader.next();
                assert(tok == jsont::ObjectStart);
                tok = reader.next();
                while (tok != jsont::ObjectEnd) {
                    assert(tok == jsont::FieldName);
                    printValueObject(sint, reader);
                    tok = reader.next();
                    assert(tok == jsont::String);
                    string_view slice = reader.dataValue();
                    // Remove starting double-quotes
                    slice.remove_prefix(1);
                    ibufferstream sptr(
                        slice.data(), slice.length(), ios::in | ios::binary);
                    unsigned offset;
                    unsigned length;
                    sptr >> offset >> length;
                    string_view stitch(inkContent.substr(offset, length));

                    if (stitch[0] == '[') {
                        sint << R"({"content":)"sv << stitch << '}';
                    } else {
                        sint << stitch;
                    }
                    tok = reader.next();
                    if (tok == jsont::Comma) {
                        printValueRaw(sint, reader);
                        tok = reader.next();
                    }
                }
                assert(tok == jsont::ObjectEnd);
                tok = reader.next();
            }
        }
        assert(tok == jsont::ObjectEnd);
        printValueRaw(sint, reader);
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
    string_view const inkContent;
};
// NOLINTNEXTLINE(modernize-use-trailing-return-type)
BOOST_IOSTREAMS_PIPABLE(basic_json_unstitch_filter, 2)

using json_unstitch_filter  = basic_json_unstitch_filter<char>;
using wjson_unstitch_filter = basic_json_unstitch_filter<wchar_t>;

enum ErrorCodes {
    eOK,
    eWRONG_ARGC,
    eOBB_NOT_FOUND,
    eOBB_NOT_FILE,
    eOBB_NO_ACCESS,
    eOBB_INVALID,
    eOBB_CORRUPT,
    eINPUT_NOT_DIR,
    eINPUT_NO_ACCESS,
    eINPUT_NO_FILE_TABLE,
    eINPUT_FILES_MISSING,
    eINPUT_FILES_NOT_VALID
};

auto openObbFile(path const& obbfile) {
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

    std::unique_ptr fout =
        std::make_unique<ofstream>(obbfile, ios::out | ios::binary);
    if (!fout->good()) {
        cerr << "Could not open output file "sv << obbfile << "!"sv << endl
             << endl;
        throw ErrorCodes{eOBB_NO_ACCESS};
    }

    (*fout) << "AP_Pack!"sv;
    return fout;
}

[[nodiscard]] auto readInputDir(path const& indir) {
    if (exists(indir)) {
        if (!is_directory(indir)) {
            cerr << "Path "sv << indir << " must be a directory!"sv << endl
                 << endl;
            throw ErrorCodes{eINPUT_NOT_DIR};
        }
    } else {
        cerr << "Input path "sv << indir << " does not exist!"sv << endl
             << endl;
        throw ErrorCodes{eINPUT_NO_ACCESS};
    }
    // Read file list.
    path const filetable(indir / "FileTable.ser");
    if (!exists(filetable)) {
        cerr << "Input path "sv << filetable
             << " does not exist! Please re-dump the OBB to create it."sv
             << endl
             << endl;
        throw ErrorCodes{eINPUT_NO_FILE_TABLE};
    }
    vector<RFile_entry> entries;
    {
        ifstream      file_table(indir / "FileTable.ser");
        text_iarchive ia(file_table);
        ia >> entries;
    }
    for (auto& entry : entries) {
        path const fname = indir / entry.name();
        if (!exists(fname)) {
            cerr << "Input path "sv << filetable
                 << " does not exist! Please re-dump the OBB."sv << endl
                 << endl;
            throw ErrorCodes{eINPUT_FILES_MISSING};
        }
        if (!is_regular_file(fname)) {
            cerr << "Input path "sv << filetable
                 << " is not a file! Please re-dump the OBB."sv << endl
                 << endl;
            throw ErrorCodes{eINPUT_FILES_NOT_VALID};
        }
    }
    return entries;
}

inline auto roundUp(uint32_t numToRound, uint32_t multiple) -> uint32_t {
    assert(multiple);
    return ((numToRound + multiple - 1) / multiple) * multiple;
}

auto encodeFile(
    ofstream& obbContents, path const& infile, bool compressed,
    bool isReference) -> std::tuple<uint32_t, uint32_t, uint32_t> {
    path const parentdir(infile.parent_path());

    if (!exists(infile)) {
        cout << "\33[2K\r"sv << flush;
        cerr << "File "sv << infile << " not found!"sv << endl;
        return {};
    }

    size_t     fulllength = file_size(infile);
    bool const isJson =
        infile.extension() == ".json"s || infile.extension() == ".inkcontent"s;

    stringstream sint(ios::in | ios::out | ios::binary);

    {
        ifstream fin(infile, ios::in | ios::binary);
        if (!fin.good()) {
            cout << "\33[2K\r"sv << flush;
            cerr << "Could not create file "sv << infile << "!"sv << endl;
            return {};
        }
        if (isReference) {
            cout << "\33[2K\rGenerating story and inkcontent files..."sv
                 << flush;
        }

        {
            filtering_ostream fsout;
            if (isReference) {
                // TODO: Filter should receive OBB wrapper class and read
                // inkcontent filename = indexed-content/filename
                // TODO: use unstritch filter
                // fsout.push(json_unstitch_filter(inkData));
            }
            if (isJson) {
                fsout.push(json_filter(eNO_WHITESPACE, &fulllength));
            }
            if (compressed) {
                fsout.push(
                    zlib_compressor(zlib::best_compression, 1 * 1024 * 1024));
            }
            fsout.push(sint);
            fsout << fin.rdbuf();
        }
    }
    sint.seekg(0);
    if (!obbContents.good()) {
        cout << '\t' << obbContents.rdstate() << endl;
    }
    obbContents << sint.rdbuf();
    sint.seekg(0);
    sint.ignore(std::numeric_limits<std::streamsize>::max());
    uint32_t const complength = sint.gcount();
    uint32_t const padding    = roundUp(complength, 16U) - complength;
    constexpr static const array<char, 16U> nullPadding{};
    obbContents.write(nullPadding.data(), padding);

    if (isReference) {
        cout << "done."sv << flush;
    }
    return {fulllength, complength, padding};
}

extern "C" auto main(int argc, char* argv[]) -> int;

auto main(int argc, char* argv[]) -> int {
    try {
        if (argc != 3) {
            cerr << "Usage: "sv << argv[0] << " inputdir outputfile"sv << endl
                 << endl;
            return eWRONG_ARGC;
        }

        path const          indir(argv[1]);
        vector<RFile_entry> entries = readInputDir(indir);

        path const obbfile(argv[2]);
        auto       obbptr      = openObbFile(obbfile);
        auto&      obbcontents = *obbptr;

        uint32_t curr_offset = 8;
        auto     curr_pos    = obbcontents.tellp();
        Write4(obbcontents, 0U); // Placeholder for file size
        Write4(obbcontents, 0U); // Placeholder for file table position
        curr_offset += 8;

        // TODO: Main json file should be found from Info.plist file:
        //  main json filename = dict["StoryFilename"sv]
        regex const mainJsonRegex(R"regex(Sorcery\d\.(min)?json)regex"s);
        // TODO: inkcontent filename should be found from main json: inkcontent
        // filename = indexed-content/filename
        regex const inkContentRegex(R"regex(Sorcery\d\.inkcontent)regex"s);

        RFile_entry inkContent;

        for (auto& elem : entries) {
            // Ignore inkcontent file, we will regenerate it later while
            // processing main story file.
            string_view fname = entries.back().name();
#if 1
            if (regex_match(fname.cbegin(), fname.cend(), inkContentRegex)) {
                inkContent = elem;
                continue;
            }
            bool isReference = false;
#else
            bool isReference =
                regex_match(fname.cbegin(), fname.cend(), mainJsonRegex);
#endif
            if (isReference) {
                // Time to process both inkcontent and story file.
                cout << "\33[2K\rFound main json : "sv << fname << endl;
            } else {
                cout << "\33[2K\rPacking file "sv << elem.name() << flush;

                path infile(indir / elem.name());
                auto [file_fulllength, file_complength, file_padding] =
                    encodeFile(
                        obbcontents, infile, elem.compressed, isReference);
                elem.fdata = {curr_offset, file_fulllength, file_complength};
                curr_offset += file_complength + file_padding;
            }
        }

#if 0
        if (!mainJson.file().empty() && !inkContent.file().empty()) {
            string const fname =
                mainJson.name().substr(0, "SorceryN"sv.size()) +
                "-Reference.json"s;
            path const outfile(indir / fname);
            encodeFile(
                outfile, mainJson.file(), inkContent.file(),
                mainJson.compressed, true);
        }
#endif
        cout << endl;
        cout << "\33[2K\rCreating name table... "sv << flush;
        unordered_map<std::string, uint32_t> nameOffsets;
        for (auto& elem : entries) {
            std::string const& fname = elem.fname;
            nameOffsets[fname]       = curr_offset;
            curr_offset += fname.size();
            obbcontents.write(
                fname.data(), static_cast<uint32_t>(fname.size()));
        }
        cout << "done."sv << endl;
        uint32_t const padding = roundUp(curr_offset, 16U) - curr_offset;
        constexpr static const array<char, 16U> nullPadding{};
        obbcontents.write(nullPadding.data(), padding);
        curr_offset += padding;
        cout << "\33[2K\rCreating file table... "sv << flush;
        uint32_t file_table_pos = curr_offset;
        for (auto& elem : entries) {
            std::string const& fname = elem.fname;
            Write4(obbcontents, nameOffsets[fname]);
            Write4(obbcontents, fname.size());
            File_data const& fdata = elem.fdata;
            Write4(obbcontents, fdata.offset);
            Write4(obbcontents, fdata.fulllength);
            Write4(obbcontents, fdata.complength);
            curr_offset += 20;
        }
        obbcontents.seekp(curr_pos);
        Write4(obbcontents, curr_offset);
        Write4(obbcontents, file_table_pos);
        cout << "done."sv << endl;
    } catch (std::exception const& except) {
        cerr << except.what() << endl;
    } catch (ErrorCodes err) {
        return err;
    }
    return eOK;
}
