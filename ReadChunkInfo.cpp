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
#include "EternalModLoader.hpp"

/**
 * @brief Read info from the resources file's chunks
 * 
 * @param memoryMappedFile MemoryMappedFile object containing the resource to modify
 * @param resourceContainer ResourceContainer object to read data into
 */
void ReadChunkInfo(MemoryMappedFile &memoryMappedFile, ResourceContainer &resourceContainer)
{
    int64_t dummy7Off = resourceContainer.Dummy7Offset + (resourceContainer.TypeCount * 4);

    int64_t nameId, fileOffset, sizeOffset, sizeZ, size;
    std::byte compressionMode;
    ResourceName name;

    // Iterate through files to get all chunks
    for (int32_t i = 0; i < resourceContainer.FileCount; i++) {
        std::copy(memoryMappedFile.Mem + 0x20 + resourceContainer.InfoOffset + (0x90 * i),
            memoryMappedFile.Mem + 0x20 + resourceContainer.InfoOffset + (0x90 * i) + 8, (std::byte*)&nameId);

        std::copy(memoryMappedFile.Mem + 0x38 + resourceContainer.InfoOffset + (0x90 * i),
            memoryMappedFile.Mem + 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 8, (std::byte*)&fileOffset);

        sizeOffset = 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 8;

        std::copy(memoryMappedFile.Mem + 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 8,
            memoryMappedFile.Mem + 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 16, (std::byte*)&sizeZ);

        std::copy(memoryMappedFile.Mem + 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 16,
            memoryMappedFile.Mem + 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 24, (std::byte*)&size);

        compressionMode = memoryMappedFile.Mem[0x70 + resourceContainer.InfoOffset + 0x90 * i];

        nameId = ((nameId + 1) * 8) + dummy7Off;
        std::copy(memoryMappedFile.Mem + nameId, memoryMappedFile.Mem + nameId + 8, (std::byte*)&nameId);

        name = resourceContainer.NamesList[nameId];

        ResourceChunk chunk(name, fileOffset);
        chunk.FileOffset = sizeOffset - 8;
        chunk.SizeOffset = sizeOffset;
        chunk.SizeZ = sizeZ;
        chunk.Size = size;
        chunk.CompressionMode = compressionMode;
        resourceContainer.ChunkList.push_back(chunk);
    }
}
