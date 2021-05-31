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
#include <dlfcn.h>

#include "EternalModLoader.hpp"

OodLZ_CompressFunc* OodLZ_Compress;
OodLZ_DecompressFunc* OodLZ_Decompress;

int OodleInit()
{
    std::string linoodlePath = BasePath + "liblinoodle.so";
    void *oodle = dlopen(linoodlePath.c_str(), RTLD_LAZY);

    if (!oodle)
        return -1;

    OodLZ_Compress = (OodLZ_CompressFunc*)dlsym(oodle, "OodleLZ_Compress");
    OodLZ_Decompress = (OodLZ_DecompressFunc*)dlsym(oodle, "OodleLZ_Decompress");

    if (!OodLZ_Compress || !OodLZ_Decompress)
        return -1;

    return 0;
}

std::vector<std::byte> OodleDecompress(std::vector<std::byte> &compressedData, long decompressedSize)
{
    if (!OodLZ_Decompress) {
        if (OodleInit() == -1)
            throw std::exception();
    }

    std::vector<std::byte> decompressedData(decompressedSize);

    if (OodLZ_Decompress((uint8_t*)compressedData.data(), compressedData.size(), (uint8_t*)decompressedData.data(), decompressedSize, 1, 1, 0, NULL, 0, NULL, NULL, NULL, 0, 0) == 0)
        decompressedData.resize(0);

    return decompressedData;
}

std::vector<std::byte> OodleCompress(std::vector<std::byte> &decompressedData, OodleFormat format, OodleCompressionLevel compressionLevel)
{
    if (!OodLZ_Compress) {
        if (OodleInit() == -1)
            throw;
    }

    unsigned int compressedBufferSize = decompressedData.size() + 274 * ((decompressedData.size() + 0x3FFFF) / 0x40000);
    std::vector<std::byte> compressedData(compressedBufferSize);

    int compressedSize = OodLZ_Compress(format, (uint8_t*)decompressedData.data(), decompressedData.size(), (uint8_t*)compressedData.data(), compressionLevel, NULL, 0, 0, NULL, 0);

    if (compressedSize <= 0) {
        compressedData.resize(0);
    }
    else {
        compressedData.resize(compressedSize);
    }

    return compressedData;
}