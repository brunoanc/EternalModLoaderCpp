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
#include <filesystem>
#include <algorithm>
#include <map>

#include "Oodle/Oodle.hpp"
#include "ResourceData/ResourceData.hpp"

/**
 * @brief Parse the resource data file from disk
 * 
 * @param fileName Resource data filename
 * @return Map containing the parsed resource data
 */
std::map<uint64_t, ResourceDataEntry> ParseResourceData(std::string &fileName)
{
    std::map<uint64_t, ResourceDataEntry> resourceDataMap;
    int64_t filesize = std::filesystem::file_size(fileName);
    int64_t decompressedSize;
    std::vector<std::byte> compressedData(filesize - 8);

    FILE *resourceDataFile = fopen(fileName.c_str(), "rb");

    if (!resourceDataFile)
        return resourceDataMap;
    
    if (fread(&decompressedSize, 8, 1, resourceDataFile) != 1)
        return resourceDataMap;

    if (fread(compressedData.data(), 1, compressedData.size(), resourceDataFile) != compressedData.size())
        return resourceDataMap;

    fclose(resourceDataFile);

    std::vector<std::byte> decompressedData;

    try {
        decompressedData = OodleDecompress(compressedData, decompressedSize);

        if (decompressedData.empty())
            throw std::exception();
    }
    catch (...) {
        return resourceDataMap;
    }

    int64_t pos = 0;

    uint64_t amount;
    std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 8, (std::byte*)&amount);
    pos += 8;

    for (uint64_t i = 0; i < amount; i++) {
        ResourceDataEntry resourceDataEntry;
        
        uint64_t fileNameHash;
        std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 8, (std::byte*)&fileNameHash);
        pos += 8;

        std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 8, (std::byte*)&resourceDataEntry.StreamDbHash);
        pos += 8;

        resourceDataEntry.Version = decompressedData[pos];
        pos += 1;

        resourceDataEntry.SpecialByte1 = decompressedData[pos];
        pos += 1;

        resourceDataEntry.SpecialByte2 = decompressedData[pos];
        pos += 1;

        resourceDataEntry.SpecialByte3 = decompressedData[pos];
        pos += 1;

        uint16_t resourceTypeSize;
        std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 2, (std::byte*)&resourceTypeSize);
        pos += 2;

        resourceDataEntry.ResourceType = std::string((char*)decompressedData.data() + pos, resourceTypeSize);
        pos += resourceTypeSize;

        uint16_t mapResourceTypeSize;
        std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 2, (std::byte*)&mapResourceTypeSize);
        pos += 2;

        resourceDataEntry.MapResourceType = resourceDataEntry.ResourceType;

        if (mapResourceTypeSize > 0) {
            resourceDataEntry.MapResourceType = std::string((char*)decompressedData.data() + pos, mapResourceTypeSize);
            pos += mapResourceTypeSize;

            uint16_t mapResourceNameSize;
            std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 2, (std::byte*)&mapResourceNameSize);
            pos += 2;

            resourceDataEntry.MapResourceName = std::string((char*)decompressedData.data() + pos, mapResourceNameSize);
            pos += mapResourceNameSize;
        }

        resourceDataMap[fileNameHash] = resourceDataEntry;
    }

    return resourceDataMap;
}

/**
 * @brief Calculate the resource filename hash
 * 
 * @param input Filename to hash
 * @return Filename's resource hash
 */
uint64_t CalculateResourceFileNameHash(std::string &input)
{
    uint64_t hashedValue = 3074457345618258791;

    for (int32_t i = 0; i < input.size(); i++) {
        hashedValue += input[i];
        hashedValue *= 3074457345618258799;
    }

    return hashedValue;
}