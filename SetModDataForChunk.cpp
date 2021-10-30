#include <iostream>
#include <algorithm>
#include <cstring>
#include "EternalModLoader.hpp"

/**
 * @brief Set the mod data in the given chunk
 * 
 * @param memoryMappedFile MemoryMappedFile object containing the resource to modify
 * @param resourceContainer ResourceContainer object containing the resources's data
 * @param chunk ResourceChunk object containing the chunk's data
 * @param modFile ResourceModFile object containing the mod file's data
 * @param compressedSize Mod file's compressed size
 * @param uncompressedSize Mod file's uncompressed size
 * @param compressionMode Pointer to a byte containing the compression mode to set, if nullptr it won't be changed
 * @return True on success, false otherwise
 */
bool SetModDataForChunk(
    MemoryMappedFile &memoryMappedFile,
    ResourceContainer &resourceContainer,
    ResourceChunk &chunk,
    ResourceModFile &modFile,
    const uint64_t compressedSize,
    const uint64_t uncompressedSize,
    const std::byte *compressionMode
)
{
    // Update chunk sizes
    chunk.Size = uncompressedSize;
    chunk.SizeZ = compressedSize;
    int64_t resourceFileSize = memoryMappedFile.Size;

    if (!SlowMode) {
        // Add the data at the end of the container
        int64_t dataSectionLength = resourceFileSize - resourceContainer.DataOffset;
        int64_t placement = 0x10 - (dataSectionLength % 0x10) + 0x30;
        int64_t newContainerSize = resourceFileSize + modFile.FileBytes.size() + placement;
        int64_t dataOffset = newContainerSize - modFile.FileBytes.size();

        if (!memoryMappedFile.ResizeFile(newContainerSize)) {
            return false;
        }

        std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), memoryMappedFile.Mem + dataOffset);

        // Set the new data offset
        std::copy((std::byte*)&dataOffset, (std::byte*)&dataOffset + 8, memoryMappedFile.Mem + chunk.FileOffset);
    }
    else {
        int64_t fileOffset, size;
        std::copy(memoryMappedFile.Mem + chunk.FileOffset, memoryMappedFile.Mem + chunk.FileOffset + 8, (std::byte*)&fileOffset);
        std::copy(memoryMappedFile.Mem + chunk.FileOffset + 8, memoryMappedFile.Mem + chunk.FileOffset + 16, (std::byte*)&size);

        int64_t sizeDiff = modFile.FileBytes.size() - size;

        // We will need to expand the file if the new size is greater than the old one
        // If its shorter, we will replace all the bytes and zero out the remaining bytes
        if (sizeDiff > 0) {
            // Expand the memory stream so the new file fits
            int64_t newContainerSize = resourceFileSize + sizeDiff;

            if (!memoryMappedFile.ResizeFile(newContainerSize)) {
                return false;
            }

            int32_t toRead;

            while (resourceFileSize > fileOffset + size) {
                toRead = (resourceFileSize - BufferSize) >= (fileOffset + size) ? BufferSize : (resourceFileSize - fileOffset - size);
                resourceFileSize -= toRead;

                std::copy(memoryMappedFile.Mem + resourceFileSize, memoryMappedFile.Mem + resourceFileSize + toRead, Buffer);
                std::copy(Buffer, Buffer + toRead, memoryMappedFile.Mem + resourceFileSize + sizeDiff);
            }

            // Write the new file bytes now that the file has been expanded and there's enough space
            std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), memoryMappedFile.Mem + fileOffset);
        }
        else {
            std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), memoryMappedFile.Mem + fileOffset);

            // Zero out the remaining bytes if the file is shorter
            if (sizeDiff < 0) {
                std::byte *emptyArray = new std::byte[-sizeDiff];
                std::memset(emptyArray, 0, -sizeDiff);

                std::copy(emptyArray, emptyArray - sizeDiff, memoryMappedFile.Mem + fileOffset + modFile.FileBytes.size());
                delete[] emptyArray;
            }
        }

        // If the file was expanded, update file offsets for every file after the one we replaced
        if (sizeDiff > 0) {
            std::vector<ResourceChunk>::iterator x = std::find(resourceContainer.ChunkList.begin(), resourceContainer.ChunkList.end(), chunk);
            int32_t index = std::distance(resourceContainer.ChunkList.begin(), x);

            for (int32_t i = index + 1; i < resourceContainer.ChunkList.size(); i++) {
                std::copy(memoryMappedFile.Mem + resourceContainer.ChunkList[i].FileOffset, memoryMappedFile.Mem + resourceContainer.ChunkList[i].FileOffset + 8, (std::byte*)&fileOffset);
                int64_t newFileOffset = fileOffset + sizeDiff;
                std::copy((std::byte*)&newFileOffset, (std::byte*)&newFileOffset + 8, memoryMappedFile.Mem + resourceContainer.ChunkList[i].FileOffset);
            }
        }
    }

     // Replace the file size data
    std::copy((std::byte*)&compressedSize, (std::byte*)&compressedSize + 8, memoryMappedFile.Mem + chunk.SizeOffset);
    std::copy((std::byte*)&uncompressedSize, (std::byte*)&uncompressedSize + 8, memoryMappedFile.Mem + chunk.SizeOffset + 8);

    // Clear the compression flag
    if (compressionMode != nullptr) {
        memoryMappedFile.Mem[chunk.SizeOffset + 0x30] = *compressionMode;
    }

    return true;
}
