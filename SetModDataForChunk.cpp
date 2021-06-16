#include <iostream>
#include <algorithm>
#include <cstring>
#include <filesystem>

#include "EternalModLoader.hpp"

#ifdef _WIN32
int32_t SetModDataForChunk(
    std::byte *&mem,
    HANDLE &hFile,
    HANDLE &fileMapping,
    ResourceContainer &resourceContainer,
    ResourceChunk &chunk,
    ResourceModFile &modFile,
    uint64_t compressedSize,
    uint64_t uncompressedSize,
    std::byte *compressionMode
)
#else
int32_t SetModDataForChunk(
    std::byte *&mem,
    int32_t &fd,
    ResourceContainer &resourceContainer,
    ResourceChunk &chunk,
    ResourceModFile &modFile,
    uint64_t compressedSize,
    uint64_t uncompressedSize,
    std::byte *compressionMode
)
#endif
{
    chunk.Size = uncompressedSize;
    chunk.SizeZ = compressedSize;
    int64_t resourceFileSize = std::filesystem::file_size(resourceContainer.Path);

    if (!SlowMode) {
        int64_t dataSectionLength = resourceFileSize - resourceContainer.DataOffset;
        int64_t placement = 0x10 - (dataSectionLength % 0x10) + 0x30;
        int64_t newContainerSize = resourceFileSize + modFile.FileBytes.size() + placement;
        int64_t dataOffset = newContainerSize - modFile.FileBytes.size();

#ifdef _WIN32
        if (ResizeMmap(mem, hFile, fileMapping, newContainerSize) == -1)
#else
        if (ResizeMmap(mem, fd, resourceContainer.Path, resourceFileSize, newContainerSize) == -1)
#endif
            return -1;

        std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), mem + dataOffset);
        std::copy((std::byte*)&dataOffset, (std::byte*)&dataOffset + 8, mem + chunk.FileOffset);
    }
    else {
        int64_t fileOffset, size;
        std::copy(mem + chunk.FileOffset, mem + chunk.FileOffset + 8, (std::byte*)&fileOffset);
        std::copy(mem + chunk.FileOffset + 8, mem + chunk.FileOffset + 16, (std::byte*)&size);

        int64_t sizeDiff = modFile.FileBytes.size() - size;

        if (sizeDiff > 0) {
            int64_t newContainerSize = resourceFileSize + sizeDiff;

#ifdef _WIN32
            if (ResizeMmap(mem, hFile, fileMapping, newContainerSize) == -1) {
#else
            if (ResizeMmap(mem, fd, resourceContainer.Path, resourceFileSize, newContainerSize) == -1) {
#endif
                std::cerr << RED << "ERROR: " << RESET << "Failed to resize " << resourceContainer.Path << std::endl;
                return - 1;
            }

            int32_t toRead;

            while (resourceFileSize > fileOffset + size) {
                toRead = (resourceFileSize - BufferSize) >= (fileOffset + size) ? BufferSize : (resourceFileSize - fileOffset - size);
                resourceFileSize -= toRead;

                std::copy(mem + resourceFileSize, mem + resourceFileSize + toRead, Buffer);
                std::copy(Buffer, Buffer + toRead, mem + resourceFileSize + sizeDiff);
            }

            std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), mem + fileOffset);
        }
        else {
            std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), mem + fileOffset);

            if (sizeDiff < 0) {
                std::byte *emptyArray = new std::byte[-sizeDiff];
                std::memset(emptyArray, 0, -sizeDiff);

                std::copy(emptyArray, emptyArray - sizeDiff, mem + fileOffset + modFile.FileBytes.size());
                delete[] emptyArray;
            }
        }

        if (sizeDiff > 0) {
            std::vector<ResourceChunk>::iterator x = std::find(resourceContainer.ChunkList.begin(), resourceContainer.ChunkList.end(), chunk);
            int32_t index = std::distance(resourceContainer.ChunkList.begin(), x);

            for (int32_t i = index + 1; i < resourceContainer.ChunkList.size(); i++) {
                std::copy(mem + resourceContainer.ChunkList[i].FileOffset, mem + resourceContainer.ChunkList[i].FileOffset + 8, (std::byte*)&fileOffset);
                int64_t newFileOffset = fileOffset + sizeDiff;
                std::copy((std::byte*)&newFileOffset, (std::byte*)&newFileOffset + 8, mem + resourceContainer.ChunkList[i].FileOffset);
            }
        }
    }

    std::copy((std::byte*)&compressedSize, (std::byte*)&compressedSize + 8, mem + chunk.SizeOffset);
    std::copy((std::byte*)&uncompressedSize, (std::byte*)&uncompressedSize + 8, mem + chunk.SizeOffset + 8);

    if (compressionMode != NULL)
        mem[chunk.SizeOffset + 0x30] = *compressionMode;

    return 0;
}