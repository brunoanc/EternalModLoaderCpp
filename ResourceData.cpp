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

#include "EternalModLoader.hpp"

std::map<unsigned long, ResourceDataEntry> ParseResourceData(std::string &fileName)
{
    std::map<unsigned long, ResourceDataEntry> resourceData;

    long filesize = std::filesystem::file_size(fileName);
    long decompressedSize;
    std::vector<std::byte> compressedData(filesize - 8);

    FILE *resourceDataFile = fopen(fileName.c_str(), "rb");

    if (!resourceDataFile)
        return resourceData;
    
    if (fread(&decompressedSize, 8, 1, resourceDataFile) != 1)
        return resourceData;

    if (fread(compressedData.data(), 1, compressedData.size(), resourceDataFile) != compressedData.size())
        return resourceData;

    std::vector<std::byte> decompressedData;

    try {
        decompressedData = OodleDecompress(compressedData, decompressedSize);

        if (decompressedData.empty())
            throw std::exception();
    }
    catch (...) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to decompress " << fileName << std::endl;
        return resourceData;
    }

    if (decompressedData.empty())
        return resourceData;

    long pos = 0;

    unsigned long amount;
    std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 8, (std::byte*)&amount);
    pos += 8;

    for (unsigned long i = 0; i < amount; i++) {
        ResourceDataEntry resourceDataEntry;
        
        unsigned long fileNameHash;
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

        unsigned short resourceTypeSize;
        std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 2, (std::byte*)&resourceTypeSize);
        pos += 2;

        resourceDataEntry.ResourceType = std::string((char*)decompressedData.data() + pos, resourceTypeSize);
        pos += resourceTypeSize;

        unsigned short mapResourceTypeSize;
        std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 2, (std::byte*)&mapResourceTypeSize);
        pos += 2;

        resourceDataEntry.MapResourceType = resourceDataEntry.ResourceType;
        resourceDataEntry.MapResourceName = "";

        if (mapResourceTypeSize > 0) {
            resourceDataEntry.MapResourceType = std::string((char*)decompressedData.data() + pos, mapResourceTypeSize);
            pos += mapResourceTypeSize;

            unsigned short mapResourceNameSize;
            std::copy(decompressedData.begin() + pos, decompressedData.begin() + pos + 2, (std::byte*)&mapResourceNameSize);
            pos += 2;

            resourceDataEntry.MapResourceName = std::string((char*)decompressedData.data() + pos, mapResourceNameSize);
            pos += mapResourceNameSize;
        }

        resourceData[fileNameHash] = resourceDataEntry;
    }

    return resourceData;
}

unsigned long CalculateResourceFileNameHash(std::string &input)
{
    unsigned long hashedValue = 3074457345618258791;

    for (int i = 0; i < input.size(); i++) {
        hashedValue += input[i];
        hashedValue *= 3074457345618258799;
    }

    return hashedValue;
}