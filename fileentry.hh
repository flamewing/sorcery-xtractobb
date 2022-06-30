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

#pragma once

#include "endianio.hh"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <string_view>

struct File_data {
    uint32_t offset     = 0U;
    uint32_t fulllength = 0U;
    uint32_t complength = 0U;
};

template <typename FileDataT>
struct Basic_File_entry {
    std::string fname;
    FileDataT   fdata;
    bool        compressed = false;

    static constexpr const size_t EntrySize = 20;

    [[nodiscard]] auto name() const noexcept -> std::string const& {
        return fname;
    }
    [[nodiscard]] auto file() const noexcept -> FileDataT {
        return fdata;
    }
    Basic_File_entry() noexcept = default;
    Basic_File_entry(
            std::string_view::const_iterator position,
            std::string_view                 oggview) noexcept
            : fname(getData(position, oggview)), fdata(getData(position, oggview)),
              compressed(fdata.size() != Read4(position)) {
    }

private:
    friend class boost::serialization::access;
    static auto getData(
            std::string_view::const_iterator& position,
            std::string_view oggview) noexcept -> std::string_view {
        uint32_t ptr = Read4(position);
        uint32_t len = Read4(position);
        return oggview.substr(ptr, len);
    }
    template <class Archive>
    void serialize(Archive& archive, unsigned int const) {
        // Do NOT want to save or read file data (fdata) here
        (archive & fname);
        (archive & compressed);
    }
};

using XFile_entry = Basic_File_entry<std::string_view>;
using RFile_entry = Basic_File_entry<File_data>;
