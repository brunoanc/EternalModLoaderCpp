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

void ReadResource(mmap_allocator_namespace::mmappable_vector<std::byte>& mem, int resourceIndex)
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

    std::vector<long> namesOffsetList;
    namesOffsetList.reserve(namesNum);

    long buffer;
    for (int i = 0; i < namesNum; i++) {
        std::copy(mem.begin() + namesOffset + 8 + i * 8, mem.begin() + namesOffset + 8 + (i + 1) * 8, (std::byte*)&buffer);
        namesOffsetList.push_back(buffer);
    }

    long namesOffsetEnd = namesOffset + (namesNum + 1) * 8;
    long namesSize = namesEnd - namesOffsetEnd;
    std::vector<std::string> namesList;
    std::vector<std::byte> currentNameBytes;

    std::byte currentByte;
    for (int i = 0; i < namesSize; i++) {
        currentByte = mem[namesOffset + (namesNum + 1) * 8 + i];

        if (currentByte == (std::byte)0 || i == namesSize - 1) {
            if (currentNameBytes.empty())
                continue;

            std::string name((char*)currentNameBytes.data());

            if (name.find_first_of('$') != std::string::npos) {
                name = name.substr(0, name.find_first_of('$'));
            }

            if (name.find_last_of('#') != std::string::npos) {
                name = name.substr(0, name.find_last_of('#'));
            }

            if (name.find_first_of('#') != std::string::npos) {
                name = name.substr(0, name.find_first_of('#'));
            }

            namesList.push_back(name);
            currentNameBytes.clear();
            continue;
        }

        currentNameBytes.push_back(currentByte);
    }

    ResourceList[resourceIndex].FileCount = fileCount;
    ResourceList[resourceIndex].TypeCount = dummy2Num;
    ResourceList[resourceIndex].StringsSize = stringsSize;
    ResourceList[resourceIndex].NamesOffset = namesOffset;
    ResourceList[resourceIndex].InfoOffset = infoOffset;
    ResourceList[resourceIndex].Dummy7Offset = dummy7OffOrg;
    ResourceList[resourceIndex].DataOffset = dataOff;
    ResourceList[resourceIndex].IdclOffset = idclOff;
    ResourceList[resourceIndex].UnknownCount = unknownCount;
    ResourceList[resourceIndex].FileCount2 = fileCount * 2;
    ResourceList[resourceIndex].NamesOffsetEnd = namesOffsetEnd;
    ResourceList[resourceIndex].UnknownOffset = namesEnd;
    ResourceList[resourceIndex].UnknownOffset2 = namesEnd;
    ResourceList[resourceIndex].NamesList = namesList;

    ReadChunkInfo(mem, resourceIndex);
}