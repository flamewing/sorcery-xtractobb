#include <algorithm>
#include <memory>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <regex>
#include <string>
#include <sstream>
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

// After c++17, these should be swapped.
#if 0
#include <experimental/string_view>
#else
#include <boost/utility/string_ref.hpp>
	namespace std {
		using boost::string_ref;
	}
#define string_view string_ref
	inline constexpr std::string_view
	operator""sv(const char* __str, size_t __len) {
		return std::string_view{__str, __len};
	}
#endif

#include "prettyJson.h"
#include "jsont.h"

using std::allocator;
using std::copy;
using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::ws;
using std::istream;
using std::istream_iterator;
using std::ostream;
using std::ostream_iterator;
using std::regex;
using std::regex_match;
using std::string;
using std::stringstream;
using std::vector;
using std::string_view;

using namespace std::literals::string_literals;

using namespace boost::filesystem;
using namespace boost::iostreams;

#ifdef UNUSED
#elif defined(__GNUC__)
#	define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
#	define UNUSED(x) /*@unused@*/ x
#elif defined(__cplusplus)
#	define UNUSED(x)
#else
#	define UNUSED(x) x
#endif

inline size_t Read1(istream &in) {
	size_t c = static_cast<unsigned char>(in.get());
	return c;
}

inline size_t Read1(string_view &in) {
	size_t c = in[0];
	in.remove_prefix(1);
	return c;
}

template <typename T>
inline size_t Read1(T &in) {
	size_t c = static_cast<unsigned char>(*in++);
	return c;
}

inline void Write1(ostream &out, size_t c) {
	out.put(static_cast<char>(c & 0xff));
}

inline void Write1(char *&out, size_t c) {
	*out++ = static_cast<char>(c & 0xff);
}

inline void Write1(unsigned char *&out, size_t c) {
	*out++ = static_cast<char>(c & 0xff);
}

inline void Write1(string &out, size_t c) {
	out.push_back(static_cast<char>(c & 0xff));
}

template <typename T>
inline size_t Read2(T &in) {
	size_t c = Read1(in);
	c |= Read1(in) << 8;
	return c;
}

template <typename T>
inline size_t Read4(T &in) {
	size_t c = Read1(in);
	c |= Read1(in) << 8;
	c |= Read1(in) << 16;
	c |= Read1(in) << 24;
	return c;
}

template <typename T, int N>
inline size_t ReadN(T &in) {
	size_t c = 0;
	for (size_t i = 0; i < 8 * N; i += 8)
		c = c | (Read1(in) << i);
	return c;
}

template <typename T>
inline void Write2(T &out, size_t c) {
	Write1(out, c & 0xff);
	Write1(out, (c & 0xff00) >> 8);
}

template <typename T>
inline void Write4(T &out, size_t c) {
	Write1(out, (c & 0x000000ff));
	Write1(out, (c & 0x0000ff00) >> 8);
	Write1(out, (c & 0x00ff0000) >> 16);
	Write1(out, (c & 0xff000000) >> 24);
}

template <typename T, int N>
inline void WriteN(T &out, size_t c) {
	for (size_t i = 0; i < 8 * N; i += 8)
		Write1(out, (c >> i) & 0xff);
}

// JSON pretty-print filter for boost::filtering_ostream
template<typename Ch, typename Alloc = std::allocator<Ch>>
class basic_json_filter : public aggregate_filter<Ch, Alloc> {
private:
	typedef aggregate_filter<Ch, Alloc> base_type;
public:
	typedef typename base_type::char_type char_type;
	typedef typename base_type::category category;

	basic_json_filter(PrettyJSON _pretty) : pretty(_pretty) {
	}
private:
	typedef typename base_type::vector_type vector_type;
	void do_filter(const vector_type& src, vector_type& dest) {
		if (src.empty()) {
			return;
		}

		stringstream sint(ios::in|ios::out);
		printJSON(src, sint, pretty);
		string const &str = sint.str();
		dest.assign(str.begin(), str.end());
	}
	PrettyJSON const pretty;
};
BOOST_IOSTREAMS_PIPABLE(basic_json_filter, 2)

typedef basic_json_filter<char>    json_filter;
typedef basic_json_filter<wchar_t> wjson_filter;

// Sorcery! JSON stitch filter for boost::filtering_ostream
template<typename Ch, typename Alloc = std::allocator<Ch>>
class basic_json_stitch_filter : public aggregate_filter<Ch, Alloc> {
private:
	typedef aggregate_filter<Ch, Alloc> base_type;
	typedef typename base_type::vector_type vector_type;
public:
	typedef typename base_type::char_type char_type;
	typedef typename base_type::category category;

	// TODO: Filter should receive output directory instead.
	basic_json_stitch_filter(string_view const& _inkContent) : inkContent(_inkContent) {
	}
private:
	void do_filter(const vector_type& src, vector_type& dest) {
		if (src.empty()) {
			return;
		}

		stringstream sint(ios::in|ios::out);
		jsont::Tokenizer reader(src.data(), src.size(), jsont::UTF8TextEncoding);
		size_t indent = 0u;
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
				sint << reader.intValue();
				break;
			case jsont::Float:
				sint << reader.floatValue();
				break;
			case jsont::String:
				sint << '"' << reader.stringValue() << '"';
				break;
			case jsont::FieldName:
				if (reader.stringValue() == "indexed-content"sv) {
					sint << "\"stitches\":"sv;
					tok = reader.next();
					assert(tok == jsont::ObjectStart);
					sint << '{';
					indent++;
					tok = reader.next();
					while (tok != jsont::ObjectEnd) {
						assert(tok == jsont::FieldName);
						if (reader.stringValue() == "filename"sv) {
							// TODO: instead of being discarded, this should be used with output directoty to open stitch source file
							tok = reader.next();	// Fetch filename...
							assert(tok == jsont::String);
							tok = reader.next();	// ... and discard it
						} else if (reader.stringValue() == "ranges"sv) {
							// The meat.
							tok = reader.next();
							assert(tok == jsont::ObjectStart);
							tok = reader.next();
							while (tok != jsont::ObjectEnd) {
								assert(tok == jsont::FieldName);
								sint << '"' << reader.stringValue() << "\":"sv;
								tok = reader.next();
								assert(tok == jsont::String);
								stringstream sptr(reader.stringValue());
								unsigned offset, length;
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
					sint << '"' << reader.stringValue() << '"' << ':';
				}
				tok = reader.next();
				continue;
			default:
				break;
			}
			tok = reader.next();
			if (indent > 0 && tok != jsont::ObjectEnd && tok != jsont::ArrayEnd && tok != jsont::End) {
				sint << ',';
			}
		}
		string const &str = sint.str();
		dest.assign(str.begin(), str.end());
	}
	string_view const &inkContent;
};
BOOST_IOSTREAMS_PIPABLE(basic_json_stitch_filter, 2)

typedef basic_json_stitch_filter<char>    json_stitch_filter;
typedef basic_json_stitch_filter<wchar_t> wjson_stitch_filter;

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
	} else if (!create_directories(outdir)) {
		cerr << "Could not create output directory "sv << outdir << "!"sv << endl << endl;
		return eOUTPUT_NO_ACCESS;
	}

	ifstream fin(obbfile, ios::in|ios::binary);
	if (!fin.good()) {
		cerr << "Could not open input file "sv << obbfile << "!"sv << endl << endl;
		return eOBB_NO_ACCESS;
	}

	unsigned const len = file_size(obbfile);
	string obbcontents;
	obbcontents.resize(len);
	fin.read(&obbcontents[0], len);
	fin.close();

	string_view const oggview(obbcontents);
	if (oggview.substr(0, 8) != "AP_Pack!"sv) {
		cerr << "Input file missing signature!"sv << endl << endl;
		return eOBB_INVALID;
	}

	string_view::const_iterator it = oggview.cbegin() + 8;
	unsigned const hlen = Read4(it), htbl = Read4(it);
	if (len != hlen) {
		cerr << "Incorrect length in header!"sv << endl << endl;
		return eOBB_CORRUPT;
	}

	// TODO: Main json file should be found from Info.plist file: main json filename = dict["StoryFilename"sv] + (dict["SorceryPartNumber"sv] == "3"sv ? ".json"sv : ".minjson"sv)
	regex const mainJsonRegex("Sorcery\\d\\.(min)?json"s);
	// TODO: inkcontent filename should be found from main json: inkcontent filename = indexed-content/filename
	regex const inkContentRegex("Sorcery\\d\\.inkcontent"s);

	string_view mainJsonName;
	string_view mainJsonData, inkContentData;
	bool mainJsonCompressed = false;

	it = oggview.cbegin() + htbl;
	while (it != oggview.cend()) {
		unsigned fnameptr = Read4(it), fnamelen = Read4(it),
		         fdataptr = Read4(it), fdatalen = Read4(it);
		string_view fname(oggview.substr(fnameptr, fnamelen));
		string_view fdata(oggview.substr(fdataptr, fdatalen));
		bool const compressed = fdata.size() != Read4(it);

		// TODO: These should be obtained by name from OBB wrapper when class is implemented.
		if (regex_match(fname.cbegin(), fname.cend(), mainJsonRegex)) {
			mainJsonName = fname;
			mainJsonData = fdata;
			mainJsonCompressed = compressed;
			cout << "Found main json : "sv << fname << endl;
		} else if (regex_match(fname.cbegin(), fname.cend(), inkContentRegex)) {
			inkContentData = fdata;
			cout << "Found inkcontent: "sv << fname << endl;
		}

		path outfile(outdir / fname.to_string());
		path const parentdir(outfile.parent_path());

		if (!exists(parentdir) && !create_directories(parentdir)) {
			cerr << "Could not create directory "sv << parentdir << " for file "sv << outfile << "!"sv << endl;
		} else {
			if (outfile.extension() == ".minjson"s) {
				outfile.replace_extension(".json"s);
			}
			ofstream fout(outfile, ios::out|ios::binary);
			if (!fout.good()) {
				cerr << "Could not create file "sv << outfile << "!"sv << endl;
			} else {
				filtering_ostream fsout;
				if (compressed) {
					fsout.push(zlib_decompressor());
				}
				if (outfile.extension() == ".json"s || outfile.extension() == ".inkcontent"s) {
					fsout.push(json_filter(ePRETTY));
				}
				fsout.push(fout);
				fsout.write(fdata.data(), fdata.size());
			}
		}
	}

	if (mainJsonData.size() != 0u && inkContentData.size() != 0u) {
		string const fname = mainJsonName.substr(0, "SorceryN"sv.size()).to_string() + "-Reference.json"s;
		path const outfile(outdir / fname);
		path const parentdir(outfile.parent_path());

		if (!exists(parentdir) && !create_directories(parentdir)) {
			cerr << "Could not create directory "sv << parentdir << " for file "sv << outfile << "!"sv << endl;
		} else {
			ofstream fout(outfile, ios::out|ios::binary);
			if (!fout.good()) {
				cerr << "Could not create file "sv << outfile << "!"sv << endl;
			} else {
				cout << "Creating reference file "sv << outfile << "."sv << endl;
				filtering_ostream fsout;
				if (mainJsonCompressed) {
					fsout.push(zlib_decompressor());
				}
				// TODO: Filter should receive OBB wrapper class and read inkcontent filename = indexed-content/filename
				fsout.push(json_stitch_filter(inkContentData));
				fsout.push(json_filter(ePRETTY));
				fsout.push(fout);
				fsout.write(mainJsonData.data(), mainJsonData.size());
			}
		}
	}

	return eOK;
}
