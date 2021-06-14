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

void ReadChunkInfo(FILE *&resourceFile, ResourceContainer &resourceContainer)
{
    long dummy7Offset = resourceContainer.Dummy7Offset + (resourceContainer.TypeCount * 4);
    fseek(resourceFile, dummy7Offset, SEEK_SET);

    for (int i = 0; i < resourceContainer.FileCount; i++) {
        fseek(resourceFile, 0x20 + resourceContainer.InfoOffset + (0x90 * i), SEEK_SET);

        long nameId;
        fread(&nameId, 8, 1, resourceFile);

        fseek(resourceFile, 0x38 + resourceContainer.InfoOffset + (0x90 * i), SEEK_SET);

        long fileOffset;
        fread(&fileOffset, 8, 1, resourceFile);

        long sizeOffset = ftell(resourceFile);

        long sizeZ;
        fread(&sizeZ, 8, 1, resourceFile);

        long size;
        fread(&size, 8, 1, resourceFile);

        fseek(resourceFile, 0x70 + resourceContainer.InfoOffset + (0x90 * i), SEEK_SET);

        std::byte compressionMode = (std::byte)fgetc(resourceFile);
        nameId = ((nameId + 1) * 8) + dummy7Offset;

        fseek(resourceFile, nameId, SEEK_SET);
        fread(&nameId, 8, 1, resourceFile);

        ResourceName name = resourceContainer.NamesList[nameId];

        ResourceChunk chunk = ResourceChunk(name, fileOffset);
        chunk.FileOffset = sizeOffset - 8;
        chunk.SizeOffset = sizeOffset;
        chunk.SizeZ = sizeZ;
        chunk.Size = size;
        chunk.CompressionMode = compressionMode;
        resourceContainer.ChunkList.push_back(chunk);
    }
}