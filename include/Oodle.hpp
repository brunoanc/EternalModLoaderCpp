/*
* This file is part of EternalModLoaderCpp (https://github.com/PowerBall253/EternalModLoaderCpp).
* Copyright (C) 2021 PowerBall253
*
* EternalModLoaderCpp is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* EternalModLoaderCpp is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with EternalModLoaderCpp. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef OODLE_HPP
#define OODLE_HPP

#include <string>
#include <vector>
#include <cstdint>

// Kraken compression functions
extern "C" {
    int Kraken_Compress(uint8_t* src, size_t src_len, uint8_t* dst, int level);
    int Kraken_Decompress(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len);
}

class Oodle
{
public:
    static std::vector<std::byte> Decompress(std::vector<std::byte>& compressedData, const size_t decompressedSize);
    static std::vector<std::byte> Compress(std::vector<std::byte>& compressedData);
};

#endif
