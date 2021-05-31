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

#include <iostream>
#include <vector>

#ifndef OODLE_HPP
#define OODLE_HPP

typedef int OodLZ_CompressFunc(
    int codec, uint8_t *src_buf, size_t src_len, uint8_t *dst_buf, int level,
    void *opts, size_t offs, size_t unused, void *scratch, size_t scratch_size);
 
typedef int OodLZ_DecompressFunc(uint8_t *src_buf, int src_len, uint8_t *dst, size_t dst_size,
    int fuzz, int crc, int verbose,
    uint8_t *dst_base, size_t e, void *cb, void *cb_ctx, void *scratch, size_t scratch_size, int threadPhase);

enum OodleCompressionLevel
{
    NoCompression,
    SuperFast,
    VeryFast,
    Fast,
    Normal,
    Optimal1,
    Optimal2,
    Optimal3,
    Optimal4,
    Optimal5
};

enum OodleFormat
{
    LZH,
    LZHLW,
    LZNIB,
    NoFormat,
    LZB16,
    LZBLW,
    LZA,
    LZNA,
    Kraken,
    Mermaid,
    BitKnit,
    Selkie,
    Akkorokamui
};

std::vector<std::byte> OodleDecompress(std::vector<std::byte> &compressedData, long decompressedSize);
std::vector<std::byte> OodleCompress(std::vector<std::byte> &compressedData, OodleFormat format, OodleCompressionLevel compressionLevel);

#endif