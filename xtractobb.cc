#include <algorithm>
#include <memory>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <iterator>
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

#include <boost/regex.hpp>

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
using std::string;
using std::stringstream;
using std::vector;

using namespace boost::filesystem;
using namespace boost::iostreams;

using boost::regex;
using boost::regex_match;

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

inline size_t Read1(char const *& in) {
	size_t c = static_cast<unsigned char>(*in++);
	return c;
}

inline size_t Read1(unsigned char const *& in) {
	size_t c = *in++;
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
	PrettyJSON pretty;
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
	basic_json_stitch_filter(vector_type const &_inkContent) : inkContent(_inkContent) {
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
				sint << "true";
				break;
			case jsont::False:
				sint << "false";
				break;
			case jsont::Null:
				sint << "null";
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
				if (reader.stringValue() == "indexed-content") {
					sint << "\"stitches\":";
					tok = reader.next();
					assert(tok == jsont::ObjectStart);
					sint << '{';
					indent++;
					tok = reader.next();
					while (tok != jsont::ObjectEnd) {
						assert(tok == jsont::FieldName);
						if (reader.stringValue() == "filename") {
							// TODO: instead of being discarded, this should be used with output directoty to open stitch source file
							tok = reader.next();	// Fetch filename...
							assert(tok == jsont::String);
							tok = reader.next();	// ... and discard it
						} else if (reader.stringValue() == "ranges") {
							// The meat.
							tok = reader.next();
							assert(tok == jsont::ObjectStart);
							tok = reader.next();
							while (tok != jsont::ObjectEnd) {
								assert(tok == jsont::FieldName);
								sint << '"' << reader.stringValue() << "\":";
								tok = reader.next();
								assert(tok == jsont::String);
								stringstream sptr(reader.stringValue());
								unsigned offset, length;
								sptr >> offset >> length;
								typename vector_type::const_iterator first = inkContent.cbegin() + offset;
								typename vector_type::const_iterator last  = first + length;

								if (*first == '[') {
									sint << "{\"content\":";
									copy(first, last, ostream_iterator<Ch>(sint));
									sint << '}';
								} else {
									copy(first, last, ostream_iterator<Ch>(sint));
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
	vector_type const &inkContent;
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
		cerr << "Usage: " << argv[0] << " inputfile outputdir" << endl << endl;
		return eWRONG_ARGC;
	}

	path obbfile(argv[1]);
	if (!exists(obbfile)) {
		cerr << "File '" << obbfile << "' does not exist!" << endl << endl;
		return eOBB_NOT_FOUND;
	} else if (!is_regular_file(obbfile)) {
		cerr << "Path '" << obbfile << "' must be a file!" << endl << endl;
		return eOBB_NOT_FILE;
	}

	path outdir(argv[2]);
	if (exists(outdir)) {
		if (!is_directory(outdir)) {
			cerr << "Path '" << outdir << "' must be a directory!" << endl << endl;
			return eOUTPUT_NOT_DIR;
		}
	} else if (!create_directories(outdir)) {
		cerr << "Could not create output directory '" << outdir << "'!" << endl << endl;
		return eOUTPUT_NO_ACCESS;
	}

	ifstream fin(obbfile, ios::in|ios::binary);
	if (!fin.good()) {
		cerr << "Could not open input file '" << obbfile << "'!" << endl << endl;
		return eOBB_NO_ACCESS;
	}

	unsigned len = file_size(obbfile);

	char sigbuf[8];
	fin.read(sigbuf, sizeof(sigbuf));
	if (memcmp(sigbuf, "AP_Pack!", sizeof(sigbuf))) {
		cerr << "Input file missing signature!" << endl << endl;
		return eOBB_INVALID;
	}

	unsigned hlen = Read4(fin), htbl = Read4(fin);
	if (len != hlen) {
		cerr << "Incorrect length in header!" << endl << endl;
		return eOBB_CORRUPT;
	}

	// TODO: OBB wrapper class with file search.
	unsigned mainJsonPtr = 0u, inkContentPtr = 0u, referencePtr = 0u;
	// TODO: Main json file should be found from Info.plist file: main json filename = dict["StoryFilename"] + (dict["SorceryPartNumber"] == "3" ? ".json" : ".minjson")
	regex mainJsonRegex("Sorcery\\d\\.(min)?json");
	// TODO: inkcontent filename should be found from main json: inkcontent filename = indexed-content/filename
	regex inkContentRegex("Sorcery\\d\\.inkcontent");
	// TODO: Should not have any special significance.
	regex referenceRegex("Sorcery\\dReference\\.(min)?json");

	fin.seekg(htbl);
	while (fin.tellg() < hlen) {
		unsigned fentry = fin.tellg();
		unsigned fnameptr = Read4(fin), fnamelen = Read4(fin),
		         fdataptr = Read4(fin), fdatalen = Read4(fin), funclen = Read4(fin);
		unsigned nextloc = fin.tellg();

		fin.seekg(fnameptr);
		char *fnamebuf = new char[fnamelen+1];
		fin.read(fnamebuf, fnamelen);
		fnamebuf[fnamelen] = 0;
		// TODO: These should be obtained by name from OBB wrapper when class is implemented.
		if (regex_match(fnamebuf, mainJsonRegex)) {
			mainJsonPtr = fentry;
			cout << "Found main json : " << fnamebuf << "\t" << mainJsonPtr << endl;
		} else if (regex_match(fnamebuf, inkContentRegex)) {
			inkContentPtr = fentry;
			cout << "Found inkcontent: " << fnamebuf << "\t" << inkContentPtr << endl;
		} else if (regex_match(fnamebuf, referenceRegex)) {
			referencePtr = fentry;
			cout << "Found reference : " << fnamebuf << "\t" << referencePtr << endl;
		}

		path outfile(outdir / fnamebuf);
		delete [] fnamebuf;
		path parentdir(outfile.parent_path());

		if (!exists(parentdir) && !create_directories(parentdir)) {
			cerr << "Could not create directory '" << parentdir << "' for file '" << outfile << "'!" << endl;
		} else {
			if (outfile.extension() == ".minjson") {
				outfile.replace_extension(".json");
			}
			ofstream fout(outfile, ios::out|ios::binary);
			if (!fout.good()) {
				cerr << "Could not create file '" << outfile << "'!" << endl;
			} else {
				filtering_ostream fsout;
				if (fdatalen != funclen) {
					fsout.push(zlib_decompressor());
				}

				if (outfile.extension() == ".json" || outfile.extension() == ".inkcontent") {
					fsout.push(json_filter(ePRETTY));
				}
				fsout.push(fout);

				char *fdatabuf = new char[fdatalen];
				fin.seekg(fdataptr);
				fin.read(fdatabuf, fdatalen);
				fsout.write(fdatabuf, fdatalen);
				delete [] fdatabuf;
			}
		}

		fin.seekg(nextloc);
	}

	if (mainJsonPtr != 0u && inkContentPtr != 0u) {
		fin.seekg(mainJsonPtr);
		unsigned fnameptr = Read4(fin);
		fin.ignore(4);
		unsigned fdataptr = Read4(fin), fdatalen = Read4(fin), funclen = Read4(fin);
		// Want only the digit
		char fnamebuf[50];
		fin.seekg(fnameptr + sizeof("Sorcery") - 1);
		char cc = fin.get();
		snprintf(fnamebuf, sizeof(fnamebuf), "Sorcery%c-Reference.json", cc);

		// TODO: Filter should receive OBB wrapper class and read inkcontent filename = indexed-content/filename
		fin.seekg(inkContentPtr + 8);
		unsigned contentData = Read4(fin), contentLen = Read4(fin);
		fin.seekg(contentData);
		vector<char> inkContent;
		inkContent.resize(contentLen);
		fin.read(&inkContent[0], contentLen);

		path outfile(outdir / fnamebuf);
		path parentdir(outfile.parent_path());

		if (!exists(parentdir) && !create_directories(parentdir)) {
			cerr << "Could not create directory '" << parentdir << "' for file '" << outfile << "'!" << endl;
		} else {
			ofstream fout(outfile, ios::out|ios::binary);
			if (!fout.good()) {
				cerr << "Could not create file '" << outfile << "'!" << endl;
			} else {
				cout << "Creating reference file '" << outfile << "'." << endl;
				filtering_ostream fsout;
				if (fdatalen != funclen) {
					fsout.push(zlib_decompressor());
				}

				// TODO: Filter should receive OBB wrapper class and read inkcontent filename = indexed-content/filename
				fsout.push(json_stitch_filter(inkContent));
				fsout.push(json_filter(ePRETTY));
				fsout.push(fout);

				char *fdatabuf = new char[fdatalen];
				fin.seekg(fdataptr);
				fin.read(fdatabuf, fdatalen);
				fsout.write(fdatabuf, fdatalen);
				delete [] fdatabuf;
			}
		}
	}

	return eOK;
}

