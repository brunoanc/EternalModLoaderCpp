#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include <filesystem>

#include "EternalModLoader.hpp"

void AddChunks(mmap_allocator_namespace::mmappable_vector<std::byte>& mem, int resourceIndex) {
    long fileSize = (long)std::filesystem::file_size(ResourceList[resourceIndex].Path);

    std::ios::sync_with_stdio(false);

    if (ResourceList[resourceIndex].ModListNew.empty()) return;

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
        fileOffset = (long) data.size() + placement + ResourceList[resourceIndex].DataOffset;
        data.resize(data.size() + placement + mod.FileBytes.size());
        std::copy(mod.FileBytes.begin(), mod.FileBytes.end(), data.end() - (long)mod.FileBytes.size());

        long nameId;
        long nameIdOffset;
        nameId = (long) ResourceList[resourceIndex].NamesList.size();
        nameIds.resize(nameIds.size() + 8);
        nameIdOffset = (long) nameIds.size() / 8 - 1;
        nameIds.resize(nameIds.size() + 8);
        std::vector<std::byte> nameIdsVector = LongToVector(nameId, 8);
        std::copy(nameIdsVector.begin(), nameIdsVector.end(), nameIds.end() - 8);
        std::vector<std::byte> nameOffsetsBytes(8);
        std::copy(nameOffsets.end() - 8, nameOffsets.end(), nameOffsetsBytes.begin());
        long lastOffset = VectorToNumber(nameOffsetsBytes, 8);
        long lastNameOffset = 0;

        for (int i = (int) lastOffset; i < names.size(); i++) {
            if ((int)(names[i]) == 0) {
                lastNameOffset = i + 1;
                break;
            }
        }

        std::vector<char> nameChars(mod.Name.begin(), mod.Name.end());
        std::vector<std::byte> nameBytes;
        nameBytes.insert(nameBytes.begin(), (std::byte*)nameChars.data(), (std::byte*)nameChars.data() + nameChars.size());
        names.resize(names.size() + nameChars.size() + 1);
        std::copy(nameBytes.begin(), nameBytes.end(), names.begin() + lastNameOffset);

        std::vector<std::byte> newCount = VectorIntegralAdd(nameOffsets, 8, 1);
        std::copy(newCount.begin(), newCount.end(), nameOffsets.begin());
        nameOffsets.resize(nameOffsets.size() + 8);

        std::vector<std::byte> lastNameOffsetVector = LongToVector(lastNameOffset, 8);
        std::copy(lastNameOffsetVector.begin(), lastNameOffsetVector.end(), nameOffsets.end() - 8);

        std::vector<std::byte> lastInfo(0x90);
        std::copy(info.end() - 0x90, info.end(), lastInfo.begin());
        info.resize(info.size() + 0x90);
        std::copy(lastInfo.begin(), lastInfo.end(), info.end() - 0x90);
        std::vector<std::byte> nameIdOffsetVector = LongToVector(nameIdOffset, 8);
        std::copy(nameIdOffsetVector.begin(), nameIdOffsetVector.end(), info.end() - 0x70);
        std::vector<std::byte> fileOffsetVector = LongToVector(fileOffset, 8);
        std::copy(fileOffsetVector.begin(), fileOffsetVector.end(), info.end() - 0x58);

        std::vector<std::byte> FileBytesSizeVector = LongToVector((long)mod.FileBytes.size(), 8);

        std::copy(FileBytesSizeVector.begin(), FileBytesSizeVector.end(), info.begin() + (long) info.size() - 0x50);
        std::copy(FileBytesSizeVector.begin(), FileBytesSizeVector.end(), info.begin() + (long) info.size() - 0x48);

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

    std::vector<std::byte> fileCountAddVector = LongToVector((long)(ResourceList[resourceIndex].FileCount + newChunks.size()), 4);
    std::copy(fileCountAddVector.begin(), fileCountAddVector.end(), header.begin() + 0x20);

    std::vector<std::byte> fileCount2AddVector = LongToVector((long)ResourceList[resourceIndex].FileCount2 + (long)(newChunks.size() * 2), 4);
    std::copy(fileCount2AddVector.begin(), fileCount2AddVector.end(), header.begin() + 0x2C);

    std::vector<std::byte> newSizeVector = LongToVector(newSize, 4);
    std::copy(newSizeVector.begin(), newSizeVector.end(), header.begin() + 0x38);

    std::vector<std::byte> nameOffsetAddVector = LongToVector(ResourceList[resourceIndex].NamesOffset + namesOffsetAdd, 8);
    std::copy(nameOffsetAddVector.begin(), nameOffsetAddVector.end(), header.begin() + 0x40);

    std::vector<std::byte> unknownOffsetAddVector = LongToVector(ResourceList[resourceIndex].UnknownOffset + unknownAdd, 8);
    std::copy(unknownOffsetAddVector.begin(), unknownOffsetAddVector.end(), header.begin() + 0x48);

    std::vector<std::byte> unknownOffsetAdd2Vector = LongToVector(ResourceList[resourceIndex].UnknownOffset2 + unknownAdd, 8);
    std::copy(unknownOffsetAdd2Vector.begin(), unknownOffsetAdd2Vector.end(), header.begin() + 0x58);

    std::vector<std::byte> dummy7OffsetAddVector = LongToVector(ResourceList[resourceIndex].Dummy7Offset + typeIdsAdd, 8);
    std::copy(dummy7OffsetAddVector.begin(), dummy7OffsetAddVector.end(), header.begin() + 0x60);

    std::vector<std::byte> dataOffsetAddVector = LongToVector(ResourceList[resourceIndex].DataOffset + dataAdd, 8);
    std::copy(dataOffsetAddVector.begin(), dataOffsetAddVector.end(), header.begin() + 0x68);

    std::vector<std::byte> idclOffsetAdd = LongToVector(ResourceList[resourceIndex].IdclOffset + idclAdd, 8);
    std::copy(idclOffsetAdd.begin(), idclOffsetAdd.end(), header.begin() + 0x74);

    std::vector<std::byte> newOffsetBuffer(16);

    info.reserve(2 * (0x38 + info.size() + 8));

    for (int i = 0; i < (int)info.size() / 0x90; i++)
    {
        int fileOffset = 0x38 + (i * 0x90);
        std::copy(info.begin() + fileOffset, info.begin() + fileOffset + 8, newOffsetBuffer.begin());
        std::vector<std::byte> newOffsetBufferPlusDataAdd = VectorIntegralAdd(newOffsetBuffer, 8, dataAdd);
        std::copy(newOffsetBufferPlusDataAdd.begin(), newOffsetBufferPlusDataAdd.end(), info.begin() + fileOffset);
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

    if ((long)newChunks.size() != 0)
    {
        std::cout << "Number of files added: " << GREEN << newChunks.size() << " file(s) " << RESET << "in " << YELLOW << ResourceList[resourceIndex].Path << RESET << "." << std::endl;
    }
}
