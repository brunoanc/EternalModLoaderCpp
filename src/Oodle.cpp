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

#include "Oodle.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#define SAFE_SPACE 64

bool Oodle::Init(const std::string& basePath)
{
#ifdef _WIN32
    // Load ooz dll
    std::string oozPath = basePath + "ooz.dll";
    HMODULE ooz = LoadLibraryA(oozPath.c_str());

    if (!ooz) {
        throw std::exception();
    }

    // Get oodle compression functions
    KrakenCompress = static_cast<Kraken_Compress*>(GetProcAddress(ooz, "Kraken_Compress"));
    KrakenDecompress = static_cast<Kraken_Decompress*>(GetProcAddress(ooz, "Kraken_Decompress"));
#else
    // Load linoodle library
    std::string oozPath = basePath + "libooz.so";
    void *ooz = dlopen(oozPath.c_str(), RTLD_LAZY);

    if (!ooz) {
        return false;
    }

    // Get oodle compression functions
    KrakenCompress = reinterpret_cast<Kraken_Compress*>(dlsym(ooz, "Kraken_Compress"));
    KrakenDecompress = reinterpret_cast<Kraken_Decompress*>(dlsym(ooz, "Kraken_Decompress"));
#endif

    if (!KrakenCompress || !KrakenDecompress) {
        return false;
    }

    return true;
}

std::vector<std::byte> Oodle::Decompress(std::vector<std::byte>& compressedData, const size_t decompressedSize)
{
    // Decompress data with oodle
    std::vector<std::byte> decompressedData(decompressedSize + SAFE_SPACE);

    if (KrakenDecompress(reinterpret_cast<uint8_t*>(compressedData.data()), compressedData.size(),
    reinterpret_cast<uint8_t*>(decompressedData.data()), decompressedSize) == 0) {
        decompressedData.resize(0);
    }

    return decompressedData;
}

std::vector<std::byte> Oodle::Compress(std::vector<std::byte>& decompressedData)
{
    // Get compressed buffer using formula to get size
    unsigned int compressedBufferSize = decompressedData.size() + 274 * ((decompressedData.size() + 0x3FFFF) / 0x40000);
    std::vector<std::byte> compressedData(compressedBufferSize);

    // Compress data with oodle
    int compressedSize = KrakenCompress(reinterpret_cast<uint8_t*>(decompressedData.data()),
        decompressedData.size(), reinterpret_cast<uint8_t*>(compressedData.data()), 4);

    if (compressedSize <= 0) {
        compressedData.resize(0);
    }
    else {
        compressedData.resize(compressedSize);
    }

    return compressedData;
}
