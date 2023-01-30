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

#include "ReadChunkInfo.hpp"
#include "Utils.hpp"
#include "ReadResourceFile.hpp"

void ReadResource(MemoryMappedFile& memoryMappedFile, ResourceContainer& resourceContainer)
{
    unsigned int fileCount;
    std::copy(memoryMappedFile.Mem + 0x20, memoryMappedFile.Mem + 0x24, reinterpret_cast<std::byte*>(&fileCount));

    unsigned int unknownCount;
    std::copy(memoryMappedFile.Mem + 0x24, memoryMappedFile.Mem + 0x28, reinterpret_cast<std::byte*>(&unknownCount));

    unsigned int dummy2Num; // Number of TypeIds
    std::copy(memoryMappedFile.Mem + 0x28, memoryMappedFile.Mem + 0x2C, reinterpret_cast<std::byte*>(&dummy2Num));

    unsigned int stringsSize; // Total size of nameOffsets and names
    std::copy(memoryMappedFile.Mem + 0x38, memoryMappedFile.Mem + 0x3C, reinterpret_cast<std::byte*>(&stringsSize));

    uint64_t namesOffset;
    std::copy(memoryMappedFile.Mem + 0x40, memoryMappedFile.Mem + 0x48, reinterpret_cast<std::byte*>(&namesOffset));

    uint64_t namesEnd;
    std::copy(memoryMappedFile.Mem + 0x48, memoryMappedFile.Mem + 0x50, reinterpret_cast<std::byte*>(&namesEnd));

    uint64_t infoOffset;
    std::copy(memoryMappedFile.Mem + 0x50, memoryMappedFile.Mem + 0x58, reinterpret_cast<std::byte*>(&infoOffset));

    uint64_t dummy7OffOrg; // Offset of TypeIds, needs addition to get offset of nameIds
    std::copy(memoryMappedFile.Mem + 0x60, memoryMappedFile.Mem + 0x68, reinterpret_cast<std::byte*>(&dummy7OffOrg));

    uint64_t dataOff;
    std::copy(memoryMappedFile.Mem + 0x68, memoryMappedFile.Mem + 0x70, reinterpret_cast<std::byte*>(&dataOff));

    uint64_t idclOff;
    std::copy(memoryMappedFile.Mem + 0x74, memoryMappedFile.Mem + 0x7C, reinterpret_cast<std::byte*>(&idclOff));

    // Read all the file names now
    uint64_t namesNum;
    std::copy(memoryMappedFile.Mem + namesOffset, memoryMappedFile.Mem + namesOffset + 8, reinterpret_cast<std::byte*>(&namesNum));

    size_t namesOffsetEnd = namesOffset + (namesNum + 1) * 8;
    size_t namesSize = namesEnd - namesOffsetEnd;

    std::vector<ResourceName> namesList;
    std::vector<char> currentNameBytes;
    char currentByte;

    for (size_t i = 0; i < namesSize; i++) {
        if (namesList.size() == namesNum) {
            break;
        }

        currentByte = static_cast<char>(memoryMappedFile.Mem[namesOffsetEnd+ i]);

        if (currentByte == 0 || i == namesSize - 1) {
            // Support full filenames and "normalized" filenames (backwards compatibility)
            std::string fullFileName(currentNameBytes.data(), currentNameBytes.size());
            std::string normalizedFileName = NormalizeResourceFilename(fullFileName);

            ResourceName resourceName(fullFileName, normalizedFileName);
            namesList.push_back(resourceName);

            currentNameBytes.clear();
            continue;
        }

        currentNameBytes.push_back(currentByte);
    }

    // Assign all values to resource
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

    // Get resource chunks
    ReadChunkInfo(memoryMappedFile, resourceContainer);
}
