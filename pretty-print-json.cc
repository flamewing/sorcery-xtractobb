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

#include <iostream>
#include <string_view>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/stream.hpp>

#include "prettyJson.hh"

using std::cerr;
using std::cout;
using std::endl;
using std::ios;
using std::istream;
using std::ostream;
using std::string_view;
using std::vector;

using namespace std::literals::string_view_literals;

using boost::filesystem::ifstream;
using boost::filesystem::ofstream;
using boost::filesystem::path;

enum ErrorCodes { eOK, eWRONG_ARGC, eINVALID_ARGS, eFILE_ERROR };

void usage(ostream& out, string_view const program) {
    out << "Usage: " << program
        << " -h\n"
           "Usage: "
        << program
        << " -p|-w|-c jsonfile [...]\n\n"
           "Where:\n"
           "\t-h\tDisplays this message.\n"
           "\t-p\tPretty prints the input JSON files.\n"
           "\t-w\tRemoves all whitespace from the input JSON files.\n"
           "\t-c\tLike w, but adds a single space after ':'.\n\n";
}

extern "C" int main(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    string_view const program(argv[0]);
    if (argc < 3) {
        usage(cerr, program);
        return eWRONG_ARGC;
    }

    PrettyJSON        pretty;
    string_view const type(argv[1]);
    if (type == "-h"sv) {
        usage(cout, program);
        return eOK;
    }
    if (type == "-p"sv) {
        pretty = ePRETTY;
    } else if (type == "-w"sv) {
        pretty = eNO_WHITESPACE;
    } else if (type == "-c"sv) {
        pretty = eCOMPACT;
    } else {
        cerr << "First parameter must be '-h', '-p', '-c' or '-w'!"sv << endl
             << endl;
        return eINVALID_ARGS;
    }

    unsigned num_errors = 0;
    for (int ii = 2; ii < argc; ii++) {
        path const jsonfile(argv[ii]);
        if (!exists(jsonfile)) {
            cerr << "File "sv << jsonfile << " does not exist!"sv << endl
                 << endl;
            num_errors++;
            continue;
        }
        if (!is_regular_file(jsonfile)) {
            cerr << "Path "sv << jsonfile << " must be a file!"sv << endl
                 << endl;
            num_errors++;
            continue;
        }

        size_t const len = file_size(jsonfile);
        ifstream     fin(jsonfile, ios::in | ios::binary);
        if (!fin.good()) {
            cerr << "Could not open input file "sv << jsonfile
                 << " for reading!"sv << endl
                 << endl;
            num_errors++;
            continue;
        }

        vector<char> buf(len);
        fin.read(&buf[0], static_cast<std::streamsize>(len));
        fin.close();

        ofstream fout(jsonfile, ios::out | ios::binary);
        if (!fout.good()) {
            cerr << "Could not open output file "sv << jsonfile
                 << " for writing!"sv << endl
                 << endl;
            num_errors++;
            continue;
        }
        printJSON(buf, fout, pretty);
        fout.close();
    }

    return (num_errors) > 0 ? eFILE_ERROR : eOK;
}
