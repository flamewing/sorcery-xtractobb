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
#include <memory>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <regex>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filter/aggregate.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include <boost/interprocess/streams/vectorstream.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>

#include <string_view>
using namespace std::literals::string_view_literals;

#include "prettyJson.h"
#include "jsont.h"

using std::allocator;
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::ios;
using std::istream;
using std::ostream;
using std::regex;
using std::regex_match;
using std::string;
using std::vector;
using std::string_view;

using namespace std::literals::string_literals;

using boost::filesystem::path;
using boost::filesystem::ifstream;
using boost::filesystem::ofstream;
using boost::iostreams::aggregate_filter;
using boost::iostreams::filtering_ostream;
using boost::iostreams::zlib_decompressor;
namespace zlib = boost::iostreams::zlib;

using vectorstream = boost::interprocess::basic_vectorstream<std::vector<char> >;
using ibufferstream = boost::interprocess::basic_ibufferstream<char>;

// Utility to convert "fancy pointer" to pointer (ported from C++20).
template <typename T>
__attribute__((always_inline,const)) inline constexpr T* to_address(T* in) noexcept {
	return in;
}

template <typename Iter>
__attribute__((always_inline,pure)) inline constexpr auto to_address(Iter in) {
	return to_address(in.operator->());
}

template <typename T>
inline size_t Read1(T &in) {
	size_t c = static_cast<unsigned char>(*in++);
	return c;
}

template <typename T>
inline uint32_t Read4(T &in) {
	auto ptr{to_address(in)};
	alignas(alignof(uint32_t)) uint8_t buf[sizeof(uint32_t)];
	uint32_t val;
	std::memcpy(&buf[0], ptr, sizeof(uint32_t));
	std::memcpy(&val, &buf[0], sizeof(uint32_t));
	std::advance(in, sizeof(uint32_t));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return val;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return __builtin_bswap32(val);
#else
	#error "Byte order is neither little endian nor big endian. Do not know how to proceed."
	return val;
#endif
}

// JSON pretty-print filter for boost::filtering_ostream
template<typename Ch, typename Alloc = std::allocator<Ch>>
class basic_json_filter : public aggregate_filter<Ch, Alloc> {
private:
	using base_type = aggregate_filter<Ch, Alloc>;
	static vectorstream sint;
public:
	using char_type = typename base_type::char_type;
	using category = typename base_type::category;

	explicit basic_json_filter(PrettyJSON _pretty) : pretty(_pretty) {
	}
private:
	using vector_type = typename base_type::vector_type;
	void do_filter(vector_type const& src, vector_type& dest) final {
		if (src.empty()) {
			return;
		}

		sint.reserve(src.size()*3/2);
		printJSON(src, sint, pretty);
		sint.swap_vector(dest);
	}
	PrettyJSON const pretty;
};
BOOST_IOSTREAMS_PIPABLE(basic_json_filter, 2)

using json_filter = basic_json_filter<char>;
using wjson_filter = basic_json_filter<wchar_t>;

template <typename Ch, typename Alloc>
vectorstream basic_json_filter<Ch, Alloc>::sint;

// Sorcery! JSON stitch filter for boost::filtering_ostream
template<typename Ch, typename Alloc = std::allocator<Ch>>
class basic_json_stitch_filter : public aggregate_filter<Ch, Alloc> {
private:
	using base_type = aggregate_filter<Ch, Alloc>;
	using vector_type = typename base_type::vector_type;
	static vectorstream sint;

public:
	using char_type = typename base_type::char_type;
	using category = typename base_type::category;

	// TODO: Filter should receive output directory instead.
	explicit basic_json_stitch_filter(string_view const& _inkContent) : inkContent(_inkContent) {
	}

private:
	void do_filter(vector_type const& src, vector_type& dest) final {
		if (src.empty()) {
			return;
		}

		sint.reserve(src.size()*3/2);
		jsont::Tokenizer reader(src.data(), src.size());
		size_t indent = 0;
		jsont::Token tok = reader.current();
		while (tok != jsont::End) {
			switch (tok) {
			case jsont::ObjectStart:
				sint << '{';
				tok = reader.next();
				if (tok == jsont::ObjectEnd) {
					sint << '}';
					break;
				} else {
					indent++;
					continue;
				}
			case jsont::ObjectEnd:
				--indent;
				sint << '}';
				break;
			case jsont::ArrayStart:
				sint << '[';
				tok = reader.next();
				if (tok == jsont::ArrayEnd) {
					sint << ']';
					break;
				} else {
					indent++;
					continue;
				}
			case jsont::ArrayEnd:
				--indent;
				sint << ']';
				break;
			case jsont::True:
				sint << "true"sv;
				break;
			case jsont::False:
				sint << "false"sv;
				break;
			case jsont::Null:
				sint << "null"sv;
				break;
			case jsont::Integer:
				sint << reader.dataValue();
				break;
			case jsont::Float:
				sint << reader.dataValue();
				break;
			case jsont::String:
				sint << '"' << reader.dataValue() << '"';
				break;
			case jsont::FieldName:
				if (reader.dataValue() == "indexed-content"sv) {
					sint << "\"stitches\":"sv;
					tok = reader.next();
					assert(tok == jsont::ObjectStart);
					sint << '{';
					indent++;
					tok = reader.next();
					while (tok != jsont::ObjectEnd) {
						assert(tok == jsont::FieldName);
						if (reader.dataValue() == "filename"sv) {
							// TODO: instead of being discarded, this should be used with output directoty to open stitch source file
							tok = reader.next();	// Fetch filename...
							assert(tok == jsont::String);
							tok = reader.next();	// ... and discard it
						} else if (reader.dataValue() == "ranges"sv) {
							// The meat.
							tok = reader.next();
							assert(tok == jsont::ObjectStart);
							tok = reader.next();
							while (tok != jsont::ObjectEnd) {
								assert(tok == jsont::FieldName);
								sint << '"' << reader.dataValue() << "\":"sv;
								tok = reader.next();
								assert(tok == jsont::String);
								string_view slice = reader.dataValue();
								ibufferstream sptr(slice.data(), slice.length());
								unsigned offset;
								unsigned length;
								sptr >> offset >> length;
								string_view stitch(inkContent.substr(offset, length));

								if (stitch[0] == '[') {
									sint << "{\"content\":"sv << stitch  << '}';
								} else {
									sint << stitch;
								}
								tok = reader.next();
								if (indent > 0 && tok != jsont::ObjectEnd && tok != jsont::ArrayEnd && tok != jsont::End) {
									sint << ',';
								}
							}
							tok = reader.next();
						}
					}
					--indent;
					sint << '}';
				} else {
					sint << '"' << reader.dataValue() << '"' << ':';
				}
				tok = reader.next();
				continue;
			case jsont::Error:
				cerr << reader.errorMessage() << endl;
				break;
			case jsont::_Comma:
			case jsont::End:
				break;
			}
			tok = reader.next();
			if (indent > 0 && tok != jsont::ObjectEnd && tok != jsont::ArrayEnd && tok != jsont::End) {
				sint << ',';
			}
		}
		sint.swap_vector(dest);
	}
	string_view const &inkContent;
};
BOOST_IOSTREAMS_PIPABLE(basic_json_stitch_filter, 2)

using json_stitch_filter = basic_json_stitch_filter<char>;
using wjson_stitch_filter = basic_json_stitch_filter<wchar_t>;

template <typename Ch, typename Alloc>
vectorstream basic_json_stitch_filter<Ch, Alloc>::sint;

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

int main(int argc, char *argv[]) {
	try {
		if (argc != 3) {
			cerr << "Usage: "sv << argv[0] << " inputfile outputdir"sv << endl << endl;
			return eWRONG_ARGC;
		}

		path const obbfile(argv[1]);
		if (!exists(obbfile)) {
			cerr << "File "sv << obbfile << " does not exist!"sv << endl << endl;
			return eOBB_NOT_FOUND;
		} else if (!is_regular_file(obbfile)) {
			cerr << "Path "sv << obbfile << " must be a file!"sv << endl << endl;
			return eOBB_NOT_FILE;
		}

		path const outdir(argv[2]);
		if (exists(outdir)) {
			if (!is_directory(outdir)) {
				cerr << "Path "sv << outdir << " must be a directory!"sv << endl << endl;
				return eOUTPUT_NOT_DIR;
			}
		} else {
			create_directories(outdir);
			if (!is_directory(outdir)) {
				cerr << "Could not create output directory "sv << outdir << "!"sv << endl << endl;
				return eOUTPUT_NO_ACCESS;
			}
		}

		ifstream fin(obbfile, ios::in|ios::binary);
		if (!fin.good()) {
			cerr << "Could not open input file "sv << obbfile << "!"sv << endl << endl;
			return eOBB_NO_ACCESS;
		}

		size_t const len = file_size(obbfile);
		string obbcontents;
		obbcontents.resize(len);
		fin.read(&obbcontents[0], static_cast<std::streamsize>(len));
		fin.close();

		string_view const oggview(obbcontents);
		if (oggview.substr(0, 8) != "AP_Pack!"sv) {
			cerr << "Input file missing signature!"sv << endl << endl;
			return eOBB_INVALID;
		}

		string_view::const_iterator it = oggview.cbegin() + 8;
		unsigned const hlen = Read4(it);
		unsigned const htbl = Read4(it);
		if (len != hlen) {
			cerr << "Incorrect length in header!"sv << endl << endl;
			return eOBB_CORRUPT;
		}

		// TODO: Main json file should be found from Info.plist file: main json filename = dict["StoryFilename"sv] + (dict["SorceryPartNumber"sv] == "3"sv ? ".json"sv : ".minjson"sv)
		regex const mainJsonRegex(R"regex(Sorcery\d\.(min)?json)regex"s);
		// TODO: inkcontent filename should be found from main json: inkcontent filename = indexed-content/filename
		regex const inkContentRegex(R"regex(Sorcery\d\.inkcontent)regex"s);

		string_view mainJsonName;
		string_view mainJsonData;
		string_view inkContentData;
		bool mainJsonCompressed = false;
		zlib_decompressor unzip(zlib::default_window_bits, 1*1024*1024);

		it = oggview.cbegin() + htbl;
		while (it != oggview.cend()) {
			unsigned fnameptr = Read4(it);
			unsigned fnamelen = Read4(it);
			unsigned fdataptr = Read4(it);
			unsigned fdatalen = Read4(it);
			string_view fname(oggview.substr(fnameptr, fnamelen));
			string_view fdata(oggview.substr(fdataptr, fdatalen));
			bool const compressed = fdata.size() != Read4(it);

			// TODO: These should be obtained by name from OBB wrapper when class is implemented.
			if (regex_match(fname.cbegin(), fname.cend(), mainJsonRegex)) {
				mainJsonName = fname;
				mainJsonData = fdata;
				mainJsonCompressed = compressed;
				cout << "\33[2K\rFound main json : "sv << fname << endl;
			} else if (regex_match(fname.cbegin(), fname.cend(), inkContentRegex)) {
				inkContentData = fdata;
				cout << "\33[2K\rFound inkcontent: "sv << fname << endl;
			}
			cout << "\33[2K\rExtracting file "sv << fname << flush;

			path outfile(outdir / string(fname));
			path const parentdir(outfile.parent_path());

			if (!exists(parentdir) && !create_directories(parentdir)) {
				cout << "\33[2K\r"sv << flush;
				cerr << "Could not create directory "sv << parentdir << " for file "sv << outfile << "!"sv << endl;
			} else {
				if (outfile.extension() == ".minjson"s) {
					outfile.replace_extension(".json"s);
				}
				ofstream fout(outfile, ios::out|ios::binary);
				if (!fout.good()) {
					cout << "\33[2K\r"sv << flush;
					cerr << "Could not create file "sv << outfile << "!"sv << endl;
				} else {
					filtering_ostream fsout;
					if (compressed) {
						fsout.push(unzip);
					}
					if (outfile.extension() == ".json"s || outfile.extension() == ".inkcontent"s) {
						fsout.push(json_filter(ePRETTY));
					}
					fsout.push(fout);
					fsout << fdata;
				}
			}
		}

		if (!mainJsonData.empty() && !inkContentData.empty()) {
			string const fname = string(mainJsonName.substr(0, "SorceryN"sv.size())) + "-Reference.json"s;
			path const outfile(outdir / fname);
			path const parentdir(outfile.parent_path());

			if (!exists(parentdir) && !create_directories(parentdir)) {
				cout << "\33[2K\r"sv << flush;
				cerr << "Could not create directory "sv << parentdir << " for file "sv << outfile << "!"sv << endl;
			} else {
				ofstream fout(outfile, ios::out|ios::binary);
				if (!fout.good()) {
					cout << "\33[2K\r"sv << flush;
					cerr << "Could not create file "sv << outfile << "!"sv << endl;
				} else {
					cout << "\33[2K\rCreating reference file "sv << outfile << "... "sv << flush;
					filtering_ostream fsout;
					if (mainJsonCompressed) {
						fsout.push(unzip);
					}
					// TODO: Filter should receive OBB wrapper class and read inkcontent filename = indexed-content/filename
					fsout.push(json_stitch_filter(inkContentData));
					fsout.push(json_filter(ePRETTY));
					fsout.push(fout);
					fsout << mainJsonData;
					cout << "done."sv << flush;
				}
			}
		}
		cout << endl;
	} catch (std::exception const& except) {
		cerr << except.what() << endl;
	}
	return eOK;
}
