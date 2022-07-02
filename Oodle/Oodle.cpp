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
#include "Oodle/Oodle.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

// Oodle instance
Oodle OodleInstance;

/**
 * @brief Oodle class constructor, inits the Oodle functions from the dll
 */
Oodle::Oodle(const std::string &basePath)
{
    // Assign base path
    BasePath = basePath;

#ifdef _WIN32
    // Load oodle dll
    std::string oo2corePath = BasePath + "..\\oo2core_8_win64.dll";
    HMODULE oodle = LoadLibraryA(oo2corePath.c_str());

    if (!oodle) {
        throw std::exception();
    }

    // Get oodle compression functions
    OodLZ_Compress = (OodLZ_CompressFunc*)GetProcAddress(oodle, "OodleLZ_Compress");
    OodLZ_Decompress = (OodLZ_DecompressFunc*)GetProcAddress(oodle, "OodleLZ_Decompress");
#else
    // Load linoodle library
    std::string linoodlePath = BasePath + "liblinoodle.so";
    void *oodle = dlopen(linoodlePath.c_str(), RTLD_LAZY);

    if (!oodle) {
        throw std::exception();
    }

    // Get oodle compression functions
    OodLZ_Compress = (OodLZ_CompressFunc*)dlsym(oodle, "OodleLZ_Compress");
    OodLZ_Decompress = (OodLZ_DecompressFunc*)dlsym(oodle, "OodleLZ_Decompress");
#endif

    if (!OodLZ_Compress || !OodLZ_Decompress) {
        throw std::exception();
    }
}

/**
 * @brief Decompress the given data with the Oodle function
 * 
 * @param compressedData Byte vector containing the data to decompress
 * @param decompressedSize Size of the data to decompress
 * @return A byte vector containing the decompressed data, or an empty byte vector on failure
 */
std::vector<std::byte> Oodle::Decompress(const std::vector<std::byte> &compressedData, const size_t decompressedSize)
{
    // Init oodle if needed
    if (!OodLZ_Decompress) {
        throw std::exception();
    }

    // Decompress data with oodle
    std::vector<std::byte> decompressedData(decompressedSize);

    if (OodLZ_Decompress((uint8_t*)compressedData.data(), compressedData.size(), (uint8_t*)decompressedData.data(), decompressedSize, 1, 1, 0, nullptr, 0, nullptr, nullptr, nullptr, 0, 0) == 0) {
        decompressedData.resize(0);
    }

    return decompressedData;
}

/**
 * @brief Compress the given data with the Oodle function
 * 
 * @param decompressedData Byte vector containing the data to compress
 * @param format Oodle format to use for compression
 * @param compressionLevel Oodle compression level to use for compression
 * @return A byte vector containing the compressed data, or an empty byte vector on failure
 */
std::vector<std::byte> Oodle::Compress(const std::vector<std::byte> &decompressedData, const OodleFormat format, const OodleCompressionLevel compressionLevel)
{
    // Init oodle if needed
    if (!OodLZ_Compress) {
        throw std::exception();
    }

    // Get compressed buffer using formula to get size
    uint32_t compressedBufferSize = decompressedData.size() + 274 * ((decompressedData.size() + 0x3FFFF) / 0x40000);
    std::vector<std::byte> compressedData(compressedBufferSize);

    // Compress data with oodle
    int32_t compressedSize = OodLZ_Compress(std::underlying_type<OodleFormat>::type(format), (uint8_t*)decompressedData.data(), decompressedData.size(),
        (uint8_t*)compressedData.data(), std::underlying_type<OodleCompressionLevel>::type(compressionLevel), nullptr, 0, 0, nullptr, 0);

    if (compressedSize <= 0) {
        compressedData.resize(0);
    }
    else {
        compressedData.resize(compressedSize);
    }

    return compressedData;
}
