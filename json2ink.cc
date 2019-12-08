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

#include "driver.hh"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>
#include <string_view>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::ios;
using std::ostream;
using std::string_view;

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

using boost::filesystem::ofstream;
using boost::filesystem::path;

enum ErrorCodes { eOK, eWRONG_ARGC, eFILE_ERROR };

void usage(ostream& out, string_view const program) {
    out << "Usage: "sv << program
        << "input-reference"sv
           "Where:\n"sv
           "\tinput-reference \tThis is the SorceryN-reference.json file.\n\n"sv;
}

extern "C" auto main(int argc, char* argv[]) -> int;

auto main(int argc, char* argv[]) -> int {
    string_view const program(argv[0]);
    if (argc != 2) {
        usage(cerr, program);
        return eWRONG_ARGC;
    }

    path const jsonfile(argv[1]);
    if (!exists(jsonfile)) {
        cerr << "JSON reference file "sv << jsonfile << " does not exist!"sv
             << endl
             << endl;
        return eFILE_ERROR;
    }

    if (!is_regular_file(jsonfile)) {
        cerr << "Path "sv << jsonfile << " must be a file!"sv << endl << endl;
        return eFILE_ERROR;
    }

    path inkfile(jsonfile);
    inkfile.replace_extension(".ink"s);
    ofstream fout(inkfile, ios::out | ios::binary);
    if (!fout.good()) {
        cout << endl;
        cerr << "Could not create file "sv << inkfile << "!"sv << endl;
        return eFILE_ERROR;
    }

    cout << "\nCreating ink file "sv << inkfile << "... "sv << endl;
    driver drv(fout);
    drv.parse(jsonfile.string());
    cout << "done."sv << endl;

    return 0;
}
