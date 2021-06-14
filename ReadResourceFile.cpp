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

#include "EternalModLoader.hpp"

void ReadResource(std::byte *&mem, ResourceContainer &resourceContainer)
{
    int32_t fileCount;
    std::copy(mem + 0x20, mem + 0x24, (std::byte*)&fileCount);

    int32_t unknownCount;
    std::copy(mem + 0x24, mem + 0x28, (std::byte*)&unknownCount);

    int32_t dummy2Num;
    std::copy(mem + 0x28, mem + 0x32, (std::byte*)&dummy2Num);

    int32_t stringsSize;
    std::copy(mem + 0x38, mem + 0x42, (std::byte*)&stringsSize);

    int64_t namesOffset;
    std::copy(mem + 0x40, mem + 0x48, (std::byte*)&namesOffset);

    int64_t namesEnd;
    std::copy(mem + 0x48, mem + 0x56, (std::byte*)&namesEnd);

    int64_t infoOffset;
    std::copy(mem + 0x50, mem + 0x58, (std::byte*)&infoOffset);

    int64_t dummy7OffOrg;
    std::copy(mem + 0x60, mem + 0x68, (std::byte*)&dummy7OffOrg);

    int64_t dataOff;
    std::copy(mem + 0x68, mem + 0x76, (std::byte*)&dataOff);

    int64_t idclOff;
    std::copy(mem + 0x74, mem + 0x82, (std::byte*)&idclOff);

    int64_t namesNum;
    std::copy(mem + namesOffset, mem + namesOffset + 8, (std::byte*)&namesNum);

    int64_t namesOffsetEnd = namesOffset + (namesNum + 1) * 8;
    int64_t namesSize = namesEnd - namesOffsetEnd;

    std::vector<ResourceName> namesList;
    std::vector<char> currentNameBytes;
    char currentByte;
    
    for (int32_t i = 0; i < namesSize; i++) {
        currentByte = (char)mem[namesOffsetEnd+ i];

        if (currentByte == 0 || i == namesSize - 1) {
            if (currentNameBytes.empty())
                continue;

            std::string fullFileName(currentNameBytes.data(), currentNameBytes.size());
            std::string normalizedFileName = NormalizeResourceFilename(fullFileName);

            ResourceName resourceName(fullFileName, normalizedFileName);
            namesList.push_back(resourceName);
            
            currentNameBytes.clear();
            continue;
        }

        currentNameBytes.push_back(currentByte);
    }

    resourceContainer.FileCount = fileCount;
    resourceContainer.TypeCount = dummy2Num;
    resourceContainer.StringsSize = stringsSize;
    resourceContainer.NamesOffset = namesOffset;
    resourceContainer.InfoOffset = infoOffset;
    resourceContainer.Dummy7Offset = dummy7OffOrg;
    resourceContainer.DataOffset = dataOff;
    resourceContainer.IdclOffset = idclOff;
    resourceContainer.UnknownCount = unknownCount;
    resourceContainer.FileCount2 = fileCount * 2;
    resourceContainer.NamesOffsetEnd = namesOffsetEnd;
    resourceContainer.UnknownOffset = namesEnd;
    resourceContainer.UnknownOffset2 = namesEnd;
    resourceContainer.NamesList = namesList;

    ReadChunkInfo(mem, resourceContainer);
}