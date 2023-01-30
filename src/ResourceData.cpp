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

#include <filesystem>
#include <algorithm>
#include "Oodle.hpp"
#include "ResourceData.hpp"

namespace fs = std::filesystem;

std::map<uint64_t, ResourceDataEntry> ParseResourceData(const std::string& fileName)
{
    std::map<uint64_t, ResourceDataEntry> resourceDataMap;

    // The data should be compressed, read the whole file into memory first and decompress it
    // Compressed with Oodle Kraken, level 4
    size_t filesize = fs::file_size(fileName);
    size_t decompressedSize;
    std::vector<std::byte> compressedData(filesize - 8);

    FILE *resourceDataFile = fopen(fileName.c_str(), "rb");

    if (!resourceDataFile) {
        return resourceDataMap;
    }

    if (fread(&decompressedSize, 8, 1, resourceDataFile) != 1) {
        return resourceDataMap;
    }

    if (fread(compressedData.data(), 1, compressedData.size(), resourceDataFile) != compressedData.size()) {
        return resourceDataMap;
    }

    fclose(resourceDataFile);

    // Decompress data
    std::vector<std::byte> decompressedData;

    try {
        decompressedData = Oodle::Decompress(compressedData, decompressedSize);

        if (decompressedData.empty()) {
            throw std::exception();
        }
    }
    catch (...) {
        return resourceDataMap;
    }

    // Parse the binary data now
    size_t pos = 0;

    // Amount of entries
    uint64_t amount;
    std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 8, reinterpret_cast<std::byte*>(&amount));
    pos += 8;

    // Read each entry
    for (size_t i = 0; i < amount; i++) {
        ResourceDataEntry resourceDataEntry;

        uint64_t fileNameHash;
        std::copy(decompressedData.begin() + pos,decompressedData.begin() + pos + 8, reinterpret_cast<std::byte*>(&fileNameHash));
        pos += 8;

        std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 8, reinterpret_cast<std::byte*>(&resourceDataEntry.StreamDbHash));
        pos += 8;

        resourceDataEntry.Version = decompressedData[pos];
        pos += 1;

        resourceDataEntry.SpecialByte1 = decompressedData[pos];
        pos += 1;

        resourceDataEntry.SpecialByte2 = decompressedData[pos];
        pos += 1;

        resourceDataEntry.SpecialByte3 = decompressedData[pos];
        pos += 1;

        unsigned short resourceTypeSize;
        std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 2, reinterpret_cast<std::byte*>(&resourceTypeSize));
        pos += 2;

        resourceDataEntry.ResourceType = std::string(reinterpret_cast<char*>(decompressedData.data()) + pos, resourceTypeSize);
        pos += resourceTypeSize;

        unsigned short mapResourceTypeSize;
        std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 2,
            reinterpret_cast<std::byte*>(&mapResourceTypeSize));
        pos += 2;

        resourceDataEntry.MapResourceType = resourceDataEntry.ResourceType;

        if (mapResourceTypeSize > 0) {
            resourceDataEntry.MapResourceType = std::string(reinterpret_cast<char*>(decompressedData.data()) + pos, mapResourceTypeSize);
            pos += mapResourceTypeSize;

            unsigned short mapResourceNameSize;
            std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 2, reinterpret_cast<std::byte*>(&mapResourceNameSize));
            pos += 2;

            resourceDataEntry.MapResourceName = std::string(reinterpret_cast<char*>(decompressedData.data()) + pos, mapResourceNameSize);
            pos += mapResourceNameSize;
        }

        resourceDataMap[fileNameHash] = resourceDataEntry;
    }

    return resourceDataMap;
}

uint64_t CalculateResourceFileNameHash(const std::string& input)
{
    uint64_t hashedValue = 3074457345618258791;

    for (size_t i = 0; i < input.size(); i++) {
        hashedValue += input[i];
        hashedValue *= 3074457345618258799;
    }

    return hashedValue;
}
