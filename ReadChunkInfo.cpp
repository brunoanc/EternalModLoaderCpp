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

#include <vector>
#include <iostream>

#include "EternalModLoader.hpp"

void ReadChunkInfo(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, ResourceContainer &resourceContainer)
{
    long dummy7Off = resourceContainer.Dummy7Offset + (resourceContainer.TypeCount * 4);

    long nameId, fileOffset, sizeOffset, sizeZ, size;
    std::byte compressionMode;
    ResourceName name;

    for (int i = 0; i < resourceContainer.FileCount; i++) {
        std::copy(mem.begin() + 0x20 + resourceContainer.InfoOffset + (0x90 * i), mem.begin() + 0x20 + resourceContainer.InfoOffset + (0x90 * i) + 8, (std::byte*)&nameId);

        std::copy(mem.begin() + 0x38 + resourceContainer.InfoOffset + (0x90 * i), mem.begin() + 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 8, (std::byte*)&fileOffset);

        sizeOffset = 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 8;

        std::copy(mem.begin() + 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 8, mem.begin() + 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 16, (std::byte*)&sizeZ);

        std::copy(mem.begin() + 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 16, mem.begin() + 0x38 + resourceContainer.InfoOffset + (0x90 * i) + 24, (std::byte*)&size);

        compressionMode = mem[0x70 + resourceContainer.InfoOffset + 0x90 * i];

        nameId = ((nameId + 1) * 8) + dummy7Off;
        std::copy(mem.begin() + nameId, mem.begin() + nameId + 8, (std::byte*)&nameId);
        
        name = resourceContainer.NamesList[nameId];

        ResourceChunk chunk = ResourceChunk(name, fileOffset);
        chunk.FileOffset = sizeOffset - 8;
        chunk.SizeOffset = sizeOffset;
        chunk.SizeZ = sizeZ;
        chunk.Size = size;
        chunk.CompressionMode = compressionMode;
        resourceContainer.ChunkList.push_back(chunk);
    }
}