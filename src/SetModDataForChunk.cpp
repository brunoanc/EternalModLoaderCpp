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

#include <algorithm>
#include <cstring>
#include <mutex>
#include "ProgramOptions.hpp"
#include "SetModDataForChunk.hpp"

extern std::mutex mtx;

bool SetModDataForChunk(
    MemoryMappedFile& memoryMappedFile,
    ResourceContainer& resourceContainer,
    ResourceChunk& chunk,
    ResourceModFile& modFile,
    const uint64_t compressedSize,
    const uint64_t uncompressedSize,
    const std::byte *compressionMode,
    std::unique_ptr<std::byte[]>& buffer,
    int bufferSize)
{
    // Update chunk sizes
    chunk.Size = uncompressedSize;
    chunk.SizeZ = compressedSize;
    size_t resourceFileSize = memoryMappedFile.Size;

    if (!ProgramOptions::SlowMode) {
        // Add the data at the end of the container
        size_t dataSectionLength = resourceFileSize - resourceContainer.DataOffset;
        size_t placement = 0x10 - (dataSectionLength % 0x10) + 0x30;
        size_t newContainerSize = resourceFileSize + modFile.FileBytes.size() + placement;
        uint64_t dataOffset = newContainerSize - modFile.FileBytes.size();

        if (!memoryMappedFile.ResizeFile(newContainerSize)) {
            return false;
        }

        std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), memoryMappedFile.Mem + dataOffset);

        // Set the new data offset
        std::copy(reinterpret_cast<std::byte*>(&dataOffset), reinterpret_cast<std::byte*>(&dataOffset) + 8, memoryMappedFile.Mem + chunk.FileOffset);
    }
    else {
        uint64_t fileOffset, size;
        std::copy(memoryMappedFile.Mem + chunk.FileOffset,
            memoryMappedFile.Mem + chunk.FileOffset + 8, reinterpret_cast<std::byte*>(&fileOffset));
        std::copy(memoryMappedFile.Mem + chunk.FileOffset + 8,
            memoryMappedFile.Mem + chunk.FileOffset + 16, reinterpret_cast<std::byte*>(&size));

        ssize_t sizeDiff = modFile.FileBytes.size() - size;

        // We will need to expand the file if the new size is greater than the old one
        // If its shorter, we will replace all the bytes and zero out the remaining bytes
        if (sizeDiff > 0) {
            // Expand the memory stream so the new file fits
            size_t newContainerSize = resourceFileSize + sizeDiff;

            if (!memoryMappedFile.ResizeFile(newContainerSize)) {
                return false;
            }

            size_t toRead;

            mtx.lock();

            while (resourceFileSize > fileOffset + size) {
                toRead = (resourceFileSize - bufferSize) >= (fileOffset + size) ? bufferSize : (resourceFileSize - fileOffset - size);
                resourceFileSize -= toRead;

                std::copy(memoryMappedFile.Mem + resourceFileSize, memoryMappedFile.Mem + resourceFileSize + toRead, buffer.get());
                std::copy(buffer.get(), buffer.get() + toRead, memoryMappedFile.Mem + resourceFileSize + sizeDiff);
            }

            mtx.unlock();

            // Write the new file bytes now that the file has been expanded and there's enough space
            std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), memoryMappedFile.Mem + fileOffset);
        }
        else {
            std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), memoryMappedFile.Mem + fileOffset);

            // Zero out the remaining bytes if the file is shorter
            if (sizeDiff < 0) {
                auto emptyArray = std::make_unique<std::byte[]>(-sizeDiff);
                std::memset(emptyArray.get(), 0, -sizeDiff);

                std::copy(emptyArray.get(), emptyArray.get() - sizeDiff, memoryMappedFile.Mem + fileOffset + modFile.FileBytes.size());
            }
        }

        // If the file was expanded, update file offsets for every file after the one we replaced
        if (sizeDiff > 0) {
            auto x = std::find(resourceContainer.ChunkList.begin(), resourceContainer.ChunkList.end(), chunk);
            size_t index = std::distance(resourceContainer.ChunkList.begin(), x);

            for (size_t i = index + 1; i < resourceContainer.ChunkList.size(); i++) {
                std::copy(memoryMappedFile.Mem + resourceContainer.ChunkList[i].FileOffset,
                    memoryMappedFile.Mem + resourceContainer.ChunkList[i].FileOffset + 8, reinterpret_cast<std::byte*>(&fileOffset));
                uint64_t newFileOffset = fileOffset + sizeDiff;
                std::copy(reinterpret_cast<std::byte*>(&newFileOffset),
                    reinterpret_cast<std::byte*>(&newFileOffset) + 8, memoryMappedFile.Mem + resourceContainer.ChunkList[i].FileOffset);
            }
        }
    }

     // Replace the file size data
    std::copy(reinterpret_cast<const std::byte*>(&compressedSize),
        reinterpret_cast<const std::byte*>(&compressedSize) + 8, memoryMappedFile.Mem + chunk.SizeOffset);
    std::copy(reinterpret_cast<const std::byte*>(&uncompressedSize),
        reinterpret_cast<const std::byte*>(&uncompressedSize) + 8, memoryMappedFile.Mem + chunk.SizeOffset + 8);

    // Clear the compression flag
    if (compressionMode != nullptr) {
        memoryMappedFile.Mem[chunk.SizeOffset + 0x30] = *compressionMode;
    }

    return true;
}
