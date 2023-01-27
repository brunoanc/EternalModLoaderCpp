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

// Kraken compression function types
typedef int Kraken_Compress(uint8_t* src, size_t src_len, uint8_t* dst, int level);
typedef int Kraken_Decompress(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len);

/**
 * @brief Oodle compression class
 * 
 */
class Oodle {
public:
    std::vector<std::byte> Decompress(const std::vector<std::byte> &compressedData, const size_t decompressedSize);
    std::vector<std::byte> Compress(const std::vector<std::byte> &compressedData);

    Oodle(const std::string &basePath);
    Oodle() { }
private:
    // Oodle compression function pointers
    Kraken_Compress *KrakenCompress = nullptr;
    Kraken_Decompress *KrakenDecompress = nullptr;

    // Path to get oodle dll from
    std::string BasePath;
};

// Oodle instance
extern Oodle OodleInstance;

#endif
