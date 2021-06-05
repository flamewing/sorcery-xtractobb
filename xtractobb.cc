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
#include <string>
#include <string_view>
#include <vector>

using std::allocator;
using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::flush;
using std::ios;
using std::istream;
using std::ostream;
using std::regex;
using std::regex_match;
using std::string;
using std::string_view;
using std::vector;

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

using boost::archive::text_oarchive;
using boost::filesystem::ifstream;
using boost::filesystem::ofstream;
using boost::filesystem::path;
using boost::iostreams::aggregate_filter;
using boost::iostreams::filtering_ostream;
using boost::iostreams::mapped_file_source;
using boost::iostreams::zlib_decompressor;
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
template <typename Ch, typename Alloc = allocator<Ch>>
class basic_json_stitch_filter : public aggregate_filter<Ch, Alloc> {
private:
    using base_type   = aggregate_filter<Ch, Alloc>;
    using vector_type = typename base_type::vector_type;

public:
    using char_type = typename base_type::char_type;
    using category  = typename base_type::category;

    // TODO: Filter should receive output directory instead.
    explicit basic_json_stitch_filter(string_view const _inkContent)
            : inkContent(_inkContent) {}

private:
    auto printValueRaw(vectorstream& sint, jsont::Tokenizer& reader)
            -> decltype(auto) {
        return sint << reader.dataValue();
    }

    auto printValueObject(vectorstream& sint, jsont::Tokenizer& reader)
            -> decltype(auto) {
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
                tok = reader.next();    // Fetch filename...
                assert(tok == jsont::String);
                tok = reader.next();    // ... and discard it
                assert(tok == jsont::Comma);
                tok = reader.next();    // Discard comma after it as well
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
                            slice.data(), slice.length(),
                            ios::in | ios::binary);
                    unsigned offset = 0;
                    unsigned length = 0;
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
BOOST_IOSTREAMS_PIPABLE(basic_json_stitch_filter, 2)

using json_stitch_filter  = basic_json_stitch_filter<char>;
using wjson_stitch_filter = basic_json_stitch_filter<wchar_t>;

enum ErrorCodes {
    eOK,
    eWRONG_ARGC,
    eOBB_NOT_FOUND,
    eOBB_NOT_FILE,
    eOBB_NO_ACCESS,
    eOBB_INVALID,
    eOBB_CORRUPT,
    eOUTPUT_NOT_DIR,
    eOUTPUT_NO_ACCESS
};

[[nodiscard]] auto readObbFile(path const& obbfile) -> mapped_file_source {
    if (!exists(obbfile)) {
        cerr << "File "sv << obbfile << " does not exist!"sv << endl << endl;
        throw ErrorCodes{eOBB_NOT_FOUND};
    }
    if (!is_regular_file(obbfile)) {
        cerr << "Path "sv << obbfile << " must be a file!"sv << endl << endl;
        throw ErrorCodes{eOBB_NOT_FILE};
    }

    mapped_file_source fin(obbfile);
    if (!fin.is_open()) {
        cerr << "Could not open input file "sv << obbfile << "!"sv << endl
             << endl;
        throw ErrorCodes{eOBB_NO_ACCESS};
    }

    string_view const oggview(fin.data(), fin.size());
    if (oggview.substr(0, 8) != "AP_Pack!"sv) {
        cerr << "Input file missing signature!"sv << endl << endl;
        throw ErrorCodes{eOBB_INVALID};
    }
    return fin;
}

void createOutputDir(path const& outdir) {
    if (exists(outdir)) {
        if (!is_directory(outdir)) {
            cerr << "Path "sv << outdir << " must be a directory!"sv << endl
                 << endl;
            throw ErrorCodes{eOUTPUT_NOT_DIR};
        }
    } else {
        create_directories(outdir);
        if (!is_directory(outdir)) {
            cerr << "Could not create output directory "sv << outdir << "!"sv
                 << endl
                 << endl;
            throw ErrorCodes{eOUTPUT_NO_ACCESS};
        }
    }
}

void decodeFile(
        zlib_decompressor& unzip, path outfile, string_view fdata,
        string_view inkData, bool compressed, bool isReference) {
    path const parentdir(outfile.parent_path());

    if (!exists(parentdir) && !create_directories(parentdir)) {
        cout << "\33[2K\r"sv << flush;
        cerr << "Could not create directory "sv << parentdir << " for file "sv
             << outfile << "!"sv << endl;
        return;
    }
    if (outfile.extension() == ".minjson"s) {
        outfile.replace_extension(".json"s);
    }
    ofstream fout(outfile, ios::out | ios::binary);
    if (!fout.good()) {
        cout << "\33[2K\r"sv << flush;
        cerr << "Could not create file "sv << outfile << "!"sv << endl;
        return;
    }
    if (isReference) {
        cout << "\33[2K\rCreating reference file "sv << outfile << "... "sv
             << flush;
    }
    filtering_ostream fsout;
    if (compressed) {
        fsout.push(unzip);
    }
    if (isReference) {
        // TODO: Filter should receive OBB wrapper class and read
        // inkcontent filename = indexed-content/filename
        fsout.push(json_stitch_filter(inkData));
    }
    if (outfile.extension() == ".json"s
        || outfile.extension() == ".inkcontent"s) {
        fsout.push(json_filter(ePRETTY));
    }
    fsout.push(fout);
    fsout << fdata;
    if (isReference) {
        cout << "done."sv << flush;
    }
}

extern "C" auto main(int argc, char* argv[]) -> int;

auto main(int argc, char* argv[]) -> int {
    try {
        if (argc != 3) {
            cerr << "Usage: "sv << argv[0] << " inputfile outputdir"sv << endl
                 << endl;
            return eWRONG_ARGC;
        }

        path const         obbfile(argv[1]);
        mapped_file_source obbcontents = readObbFile(obbfile);

        path const outdir(argv[2]);
        createOutputDir(outdir);

        string_view const oggview(obbcontents.data(), obbcontents.size());
        uint32_t const    hlen = Read4(oggview.cbegin() + 8);
        uint32_t const    htbl = Read4(oggview.cbegin() + 12);
        if (obbcontents.size() != hlen) {
            cerr << "Incorrect length in header!"sv << endl << endl;
            return eOBB_CORRUPT;
        }

        // TODO: Main json file should be found from Info.plist file:
        //  main json filename = dict["StoryFilename"sv] + ".json"
        regex const mainJsonRegex(R"regex(Sorcery\d\.(min)?json)regex"s);
        // TODO: inkcontent filename should be found from main json:
        // inkcontent filename = indexed-content/filename
        regex const inkContentRegex(R"regex(Sorcery\d\.inkcontent)regex"s);

        XFile_entry         mainJson;
        XFile_entry         inkContent;
        vector<XFile_entry> entries;
        entries.reserve((oggview.size() - htbl) / XFile_entry::EntrySize);

        for (const auto* it = oggview.cbegin() + htbl; it != oggview.cend();
             it += XFile_entry::EntrySize) {
            entries.emplace_back(it, oggview);
            // TODO: These should be obtained by name from OBB wrapper when
            // class is implemented.
            string_view fname = entries.back().name();
            if (regex_match(fname.cbegin(), fname.cend(), mainJsonRegex)) {
                mainJson = entries.back();
                cout << "\33[2K\rFound main json : "sv << fname << endl;
            } else if (regex_match(
                               fname.cbegin(), fname.cend(), inkContentRegex)) {
                inkContent = entries.back();
                cout << "\33[2K\rFound inkcontent: "sv << fname << endl;
            }
        }

        // Sort by data order in file, to improve OS prefetching.
        sort(entries.begin(), entries.end(), [](auto& lhs, auto& rhs) {
            return lhs.file().data() < rhs.file().data();
        });
        {
            // Save file table for future reference.
            ofstream      file_table(outdir / "FileTable.ser");
            text_oarchive oa(file_table);
            oa << entries;
        }

        zlib_decompressor unzip(zlib::default_window_bits, 1 * 1024 * 1024);

        for (auto& elem : entries) {
            cout << "\33[2K\rExtracting file "sv << elem.name() << flush;

            path outfile(outdir / elem.name());
            decodeFile(
                    unzip, outfile, elem.file(), inkContent.file(),
                    elem.compressed, false);
        }

        if (!mainJson.file().empty() && !inkContent.file().empty()) {
            string const fname = mainJson.name().substr(0, "SorceryN"sv.size())
                                 + "-Reference.json"s;
            path const outfile(outdir / fname);
            decodeFile(
                    unzip, outfile, mainJson.file(), inkContent.file(),
                    mainJson.compressed, true);
        }
        cout << endl;
    } catch (exception const& except) {
        cerr << except.what() << endl;
    } catch (ErrorCodes err) {
        return err;
    }
    return eOK;
}
