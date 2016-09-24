#include <iostream>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/iostreams/stream.hpp>

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

using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::istream;
using std::ostream;
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

enum ErrorCodes {
	eOK,
	eWRONG_ARGC,
	eINVALID_ARGS,
	eFILE_ERROR
};

void usage(ostream &out, string_view const program) {
	out << "Usage: " << program << " -h\n"
	       "Usage: " << program << " -p|-w|-c jsonfile [...]\n\n"
	       "Where:\n"
	       "\t-h\tDisplays this message.\n"
	       "\t-p\tPretty prints the input JSON files.\n"
	       "\t-w\tRemoves all whitespace from the input JSON files.\n"
	       "\t-c\tLike w, but adds a single space after ':'.\n\n";
}

int main(int argc, char *argv[]) {
	string_view const program(argv[0]);
	if (argc < 3) {
		usage(cerr, program);
		return eWRONG_ARGC;
	}

	PrettyJSON pretty;
	string_view const type(argv[1]);
	if (type == "-h"sv) {
		usage(cout, program);
		return eOK;
	} else if (type == "-p"sv) {
		pretty = ePRETTY;
	} else if (type == "-w"sv) {
		pretty = eNO_WHITESPACE;
	} else if (type == "-c"sv) {
		pretty = eCOMPACT;
	} else {
		cerr << "First parameter must be '-h', '-p', '-c' or '-w'!"sv << endl << endl;
		return eINVALID_ARGS;
	}

	unsigned num_errors = 0;
	for (int ii = 2; ii < argc; ii++) {
		path const jsonfile(argv[ii]);
		if (!exists(jsonfile)) {
			cerr << "File "sv << jsonfile << " does not exist!"sv << endl << endl;
			num_errors++;
			continue;
		} else if (!is_regular_file(jsonfile)) {
			cerr << "Path "sv << jsonfile << " must be a file!"sv << endl << endl;
			num_errors++;
			continue;
		}

		unsigned const len = file_size(jsonfile);
		ifstream fin(jsonfile, ios::in);
		if (!fin.good()) {
			cerr << "Could not open input file "sv << jsonfile << " for reading!"sv << endl << endl;
			num_errors++;
			continue;
		}

		vector<char> buf(len);
		fin.read(&buf[0], len);
		fin.close();

		ofstream fout(jsonfile, ios::out);
		if (!fout.good()) {
			cerr << "Could not open output file "sv << jsonfile << " for writing!"sv << endl << endl;
			num_errors++;
			continue;
		}
		printJSON(buf, fout, pretty);
		fout.close();
	}

	return (num_errors) > 0 ? eFILE_ERROR : eOK;
}
