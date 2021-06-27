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

typedef int32_t OodLZ_CompressFunc(
    int32_t codec, uint8_t *src_buf, size_t src_len, uint8_t *dst_buf, int32_t level,
    void *opts, size_t offs, size_t unused, void *scratch, size_t scratch_size);
 
typedef int32_t OodLZ_DecompressFunc(uint8_t *src_buf, int32_t src_len, uint8_t *dst, size_t dst_size,
    int32_t fuzz, int32_t crc, int32_t verbose,
    uint8_t *dst_base, size_t e, void *cb, void *cb_ctx, void *scratch, size_t scratch_size, int32_t threadPhase);

enum class OodleCompressionLevel
{
    None,
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

enum class OodleFormat
{
    LZH,
    LZHLW,
    LZNIB,
    None,
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

std::vector<std::byte> OodleDecompress(std::vector<std::byte> &compressedData, int64_t decompressedSize);
std::vector<std::byte> OodleCompress(std::vector<std::byte> &compressedData, OodleFormat format, OodleCompressionLevel compressionLevel);

#endif