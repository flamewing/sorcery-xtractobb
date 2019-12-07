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

BOOST_CLASS_IMPLEMENTATION(std::string_view, boost::serialization::primitive_type)
#ifndef BOOST_NO_STD_WSTRING
BOOST_CLASS_IMPLEMENTATION(std::wstring_view, boost::serialization::primitive_type)
#endif

struct File_entry {
    std::string_view              fname;
    std::string_view              fdata;
    bool                          compressed = false;
    static constexpr const size_t EntrySize  = 20;

    [[nodiscard]] auto name() const noexcept -> std::string_view {
        return fname;
    }
    [[nodiscard]] auto file() const noexcept -> std::string_view {
        return fdata;
    }
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
    static auto getData(
        std::string_view::const_iterator& it, std::string_view oggview) noexcept
        -> std::string_view {
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
