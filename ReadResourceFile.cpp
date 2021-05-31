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

void ReadResource(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, ResourceInfo &resourceInfo)
{
    int fileCount;
    std::copy(mem.begin() + 0x20, mem.begin() + 0x24, (std::byte*)&fileCount);

    int unknownCount;
    std::copy(mem.begin() + 0x24, mem.begin() + 0x28, (std::byte*)&unknownCount);

    int dummy2Num;
    std::copy(mem.begin() + 0x28, mem.begin() + 0x32, (std::byte*)&dummy2Num);

    int stringsSize;
    std::copy(mem.begin() + 0x38, mem.begin() + 0x42, (std::byte*)&stringsSize);

    long namesOffset;
    std::copy(mem.begin() + 0x40, mem.begin() + 0x48, (std::byte*)&namesOffset);

    long namesEnd;
    std::copy(mem.begin() + 0x48, mem.begin() + 0x56, (std::byte*)&namesEnd);

    long infoOffset;
    std::copy(mem.begin() + 0x50, mem.begin() + 0x58, (std::byte*)&infoOffset);

    long dummy7OffOrg;
    std::copy(mem.begin() + 0x60, mem.begin() + 0x68, (std::byte*)&dummy7OffOrg);

    long dataOff;
    std::copy(mem.begin() + 0x68, mem.begin() + 0x76, (std::byte*)&dataOff);

    long idclOff;
    std::copy(mem.begin() + 0x74, mem.begin() + 0x82, (std::byte*)&idclOff);

    long namesNum;
    std::copy(mem.begin() + namesOffset, mem.begin() + namesOffset + 8, (std::byte*)&namesNum);

    long namesOffsetEnd = namesOffset + (namesNum + 1) * 8;
    long namesSize = namesEnd - namesOffsetEnd;

    std::vector<ResourceName> namesList;
    std::vector<std::byte> currentNameBytes;
    std::byte currentByte;
    
    for (int i = 0; i < namesSize; i++) {
        currentByte = mem[namesOffsetEnd+ i];

        if (currentByte == (std::byte)0 || i == namesSize - 1) {
            if (currentNameBytes.empty())
                continue;

            std::string fullFileName((char*)currentNameBytes.data(), currentNameBytes.size());
            std::string normalizedFileName = NormalizeResourceFilename(fullFileName);

            ResourceName resourceName(fullFileName, normalizedFileName);
            namesList.push_back(resourceName);
            
            currentNameBytes.clear();
            continue;
        }

        currentNameBytes.push_back(currentByte);
    }

    resourceInfo.FileCount = fileCount;
    resourceInfo.TypeCount = dummy2Num;
    resourceInfo.StringsSize = stringsSize;
    resourceInfo.NamesOffset = namesOffset;
    resourceInfo.InfoOffset = infoOffset;
    resourceInfo.Dummy7Offset = dummy7OffOrg;
    resourceInfo.DataOffset = dataOff;
    resourceInfo.IdclOffset = idclOff;
    resourceInfo.UnknownCount = unknownCount;
    resourceInfo.FileCount2 = fileCount * 2;
    resourceInfo.NamesOffsetEnd = namesOffsetEnd;
    resourceInfo.UnknownOffset = namesEnd;
    resourceInfo.UnknownOffset2 = namesEnd;
    resourceInfo.NamesList = namesList;

    ReadChunkInfo(mem, resourceInfo);
}