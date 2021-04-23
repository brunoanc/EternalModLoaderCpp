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
#include <fstream>
#include <iterator>
#include <filesystem>

#include "EternalModLoader.hpp"

void AddChunks(mmap_allocator_namespace::mmappable_vector<std::byte>& mem, int resourceIndex)
{
    std::ios::sync_with_stdio(false);

    long fileSize = (long)std::filesystem::file_size(ResourceList[resourceIndex].Path);

    if (ResourceList[resourceIndex].ModListNew.empty())
        return;

    std::vector<std::byte> header(mem.begin(), mem.begin() + ResourceList[resourceIndex].InfoOffset);

    std::vector<std::byte> info(mem.begin() + ResourceList[resourceIndex].InfoOffset, mem.begin() + ResourceList[resourceIndex].NamesOffset);

    std::vector<std::byte> nameOffsets(mem.begin() + ResourceList[resourceIndex].NamesOffset, mem.begin() + ResourceList[resourceIndex].NamesOffsetEnd);

    std::vector<std::byte> names(mem.begin() + ResourceList[resourceIndex].NamesOffsetEnd, mem.begin() + ResourceList[resourceIndex].UnknownOffset);

    std::vector<std::byte> unknown(mem.begin() + ResourceList[resourceIndex].UnknownOffset, mem.begin() + ResourceList[resourceIndex].Dummy7Offset);

    long nameIdsOffset = ResourceList[resourceIndex].Dummy7Offset + (ResourceList[resourceIndex].TypeCount * 4);

    std::vector<std::byte> typeIds(mem.begin() + ResourceList[resourceIndex].Dummy7Offset, mem.begin() + nameIdsOffset);

    std::vector<std::byte> nameIds(mem.begin() + nameIdsOffset, mem.begin() + ResourceList[resourceIndex].IdclOffset);

    std::vector<std::byte> idcl(mem.begin() + ResourceList[resourceIndex].IdclOffset, mem.begin() + ResourceList[resourceIndex].DataOffset);

    std::vector<std::byte> data(mem.begin() + ResourceList[resourceIndex].DataOffset, mem.begin() + fileSize);

    int infoOldLength = (int)info.size();
    int nameIdsOldLength = (int)nameIds.size();

    std::vector<ResourceChunk> newChunks;

    for (Mod& mod : ResourceList[resourceIndex].ModListNew) {
        long fileOffset;
        long placement = 0x10 - ((long)data.size() % 0x10) + 0x30;
        fileOffset = (long)data.size() + placement + ResourceList[resourceIndex].DataOffset;
        data.resize(data.size() + placement + mod.FileBytes.size());
        std::copy(mod.FileBytes.begin(), mod.FileBytes.end(), data.end() - (long)mod.FileBytes.size());

        long nameId;
        long nameIdOffset;
        nameId = (long)ResourceList[resourceIndex].NamesList.size();
        nameIds.resize(nameIds.size() + 8);
        nameIdOffset = (long)nameIds.size() / 8 - 1;
        nameIds.resize(nameIds.size() + 8);
        std::copy((std::byte*)&nameId, (std::byte*)&nameId + 8, nameIds.end() - 8);
        std::byte nameOffsetsBytes[8];
        std::copy(nameOffsets.end() - 8, nameOffsets.end(), nameOffsetsBytes);
        long lastOffset;
        std::copy(nameOffsetsBytes, nameOffsetsBytes + 8, (std::byte*)&lastOffset);
        long lastNameOffset = 0;

        for (int i = (int)lastOffset; i < names.size(); i++) {
            if (names[i] == (std::byte)0) {
                lastNameOffset = i + 1;
                break;
            }
        }

        const char* nameChars = mod.Name.c_str();
        names.resize(names.size() + mod.Name.size() + 1);
        std::copy((std::byte*)nameChars, (std::byte*)nameChars + mod.Name.size(), names.begin() + lastNameOffset);

        long newCount;
        std::copy(nameOffsets.begin(), nameOffsets.begin() + 8, (std::byte*)&newCount);
        newCount += 1;
        std::copy((std::byte*)&newCount, (std::byte*)&newCount + 8, nameOffsets.begin());
        nameOffsets.resize(nameOffsets.size() + 8);

        std::copy((std::byte*)&lastNameOffset, (std::byte*)&lastNameOffset + 8, nameOffsets.end() - 8);

        std::byte lastInfo[0x90];
        std::copy(info.end() - 0x90, info.end(), lastInfo);
        info.resize(info.size() + 0x90);
        std::copy(lastInfo, lastInfo + 0x90, info.end() - 0x90);
        std::copy((std::byte*)nameIdOffset, (std::byte*)nameIdOffset + 8, info.end() - 0x70);
        std::copy((std::byte*)fileOffset, (std::byte*)fileOffset + 8, info.end() - 0x58);

        long fileBytesSize = mod.FileBytes.size();

        std::copy((std::byte*)&fileBytesSize, (std::byte*)&fileBytesSize + 8, info.begin() + (long)info.size() - 0x50);
        std::copy((std::byte*)&fileBytesSize, (std::byte*)&fileBytesSize + 8, info.begin() + (long)info.size() - 0x48);

        info[info.size() - 0x20] = (std::byte)0;

        ResourceChunk newChunk(mod.Name, fileOffset);
        newChunk.NameId = nameId;
        newChunk.Size = (long)mod.FileBytes.size();
        newChunk.SizeZ = (long)mod.FileBytes.size();
        newChunk.CompressionMode = (std::byte)0;

        newChunks.push_back(newChunk);

        std::cout << "\tAdded " << mod.Name << std::endl;
        ResourceList[resourceIndex].NamesList.push_back(mod.Name);
    }

    long namesOffsetAdd = (long)info.size() - infoOldLength;
    long newSize = (long)nameOffsets.size() + (long)names.size();
    long unknownAdd = namesOffsetAdd + (newSize - ResourceList[resourceIndex].StringsSize);
    long typeIdsAdd = unknownAdd;
    long nameIdsAdd = typeIdsAdd;
    long idclAdd = (long)nameIdsAdd + (long)(nameIds.size() - nameIdsOldLength);
    long dataAdd = idclAdd;

    int fileCountAdd = ResourceList[resourceIndex].FileCount + newChunks.size();
    std::copy((std::byte*)&fileCountAdd, (std::byte*)&fileCountAdd + 4, header.begin() + 0x20);

    int fileCount2Add = ResourceList[resourceIndex].FileCount2 + newChunks.size() * 2;
    std::copy((std::byte*)&fileCount2Add, (std::byte*)&fileCount2Add + 4, header.begin() + 0x2C);

    std::copy((std::byte*)&newSize, (std::byte*)&newSize + 4, header.begin() + 0x38);

    long nameOffsetAdd = ResourceList[resourceIndex].NamesOffset + namesOffsetAdd;
    std::copy((std::byte*)&nameOffsetAdd, (std::byte*)&nameOffsetAdd + 8, header.begin() + 0x40);

    long unknownOffsetAdd = ResourceList[resourceIndex].UnknownOffset + unknownAdd;
    std::copy((std::byte*)&unknownOffsetAdd, (std::byte*)&unknownOffsetAdd + 8, header.begin() + 0x48);

    long unknownOffsetAdd2 = ResourceList[resourceIndex].UnknownOffset2 + unknownAdd;
    std::copy((std::byte*)&unknownOffsetAdd2, (std::byte*)&unknownOffsetAdd2 + 8, header.begin() + 0x58);

    long dummy7OffsetAdd = ResourceList[resourceIndex].Dummy7Offset + typeIdsAdd;
    std::copy((std::byte*)&dummy7OffsetAdd, (std::byte*)&dummy7OffsetAdd + 8, header.begin() + 0x60);

    long dataOffsetAdd = ResourceList[resourceIndex].DataOffset + dataAdd;
    std::copy((std::byte*)&dataOffsetAdd, (std::byte*)&dataOffsetAdd + 8, header.begin() + 0x68);

    long idclOffsetAdd = ResourceList[resourceIndex].IdclOffset + idclAdd;
    std::copy((std::byte*)&idclOffsetAdd, (std::byte*)&idclOffsetAdd + 8, header.begin() + 0x74);

    info.reserve(2 * (0x38 + info.size() + 8));

    for (int i = 0; i < (int)info.size() / 0x90; i++) {
        int fileOffset = 0x38 + (i * 0x90);
        long newOffsetPlusDataAdd;
        std::copy(info.begin() + fileOffset, info.begin() + fileOffset + 8, (std::byte*)&newOffsetPlusDataAdd);
        newOffsetPlusDataAdd += dataAdd;
        std::copy((std::byte*)&newOffsetPlusDataAdd, (std::byte*)&newOffsetPlusDataAdd + 8, info.begin() + fileOffset);
    }

    long newContainerLength = (long)(header.size() + info.size() + nameOffsets.size() + names.size() + unknown.size() + typeIds.size() + nameIds.size() + idcl.size() + data.size());
    mem.munmap_file();
    std::filesystem::resize_file(ResourceList[resourceIndex].Path, newContainerLength);
    mem.mmap_file(ResourceList[resourceIndex].Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, newContainerLength, mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);

    long memPosition = 0;
    std::copy(header.begin(), header.end(), mem.begin() + memPosition);
    memPosition += (long)header.size();

    std::copy(info.begin(), info.end(), mem.begin() + memPosition);
    memPosition += (long)info.size();

    std::copy(nameOffsets.begin(), nameOffsets.end(), mem.begin() + memPosition);
    memPosition += (long)nameOffsets.size();

    std::copy(names.begin(), names.end(), mem.begin() + memPosition);
    memPosition += (long)names.size();

    std::copy(unknown.begin(), unknown.end(), mem.begin() + memPosition);
    memPosition += (long)unknown.size();

    std::copy(typeIds.begin(), typeIds.end(), mem.begin() + memPosition);
    memPosition += (long)typeIds.size();

    std::copy(nameIds.begin(), nameIds.end(), mem.begin() + memPosition);
    memPosition += (long)nameIds.size();

    std::copy(idcl.begin(), idcl.end(), mem.begin() + memPosition);
    memPosition += (long)idcl.size();

    std::copy(data.begin(), data.end(), mem.begin() + memPosition);

    if ((long)newChunks.size() != 0) {
        std::cout << "Number of files added: " << GREEN << newChunks.size() << " file(s) " << RESET << "in " << YELLOW << ResourceList[resourceIndex].Path << RESET << "." << std::endl;
    }
}