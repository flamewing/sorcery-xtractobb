/*
 *	Copyright © 2019 Flamewing <flamewing.sonic@gmail.com>
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

#ifndef FILEENTRY_HH
#define FILEENTRY_HH

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "endianio.hh"
#include <string_view>

struct File_entry {
    std::string_view              fname;
    std::string_view              fdata;
    bool                          compressed = false;
    static constexpr const size_t EntrySize  = 20;
    [[nodiscard]] std::string_view name() const noexcept { return fname; }
    [[nodiscard]] std::string_view file() const noexcept { return fdata; }
    File_entry() noexcept = default;
    File_entry(
        std::string_view::const_iterator it,
        std::string_view                 oggview) noexcept {
        fname      = getData(it, oggview);
        fdata      = getData(it, oggview);
        compressed = fdata.size() != Read4(it);
    }

private:
    friend class boost::serialization::access;
    std::string_view getData(
        std::string_view::const_iterator& it, std::string_view oggview) const
        noexcept {
        unsigned ptr = Read4(it);
        unsigned len = Read4(it);
        return oggview.substr(ptr, len);
    }
    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        // Do NOT want to save or read file data (fdata) here
        (ar & fname);
        (ar & compressed);
    }
};

#endif
