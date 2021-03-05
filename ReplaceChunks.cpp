#include <iostream>
#include <filesystem>
#include <algorithm>

#include "EternalModLoader.hpp"

void ReplaceChunks(mmap_allocator_namespace::mmappable_vector<std::byte>& mem, int resourceIndex) {
    std::ios::sync_with_stdio(false);

    int fileCount = 0;

    for (auto& mod : ResourceList[resourceIndex].ModList) {
        int chunkIndex = GetChunk(mod.Name, resourceIndex);
        if (chunkIndex == -1) {
            ResourceList[resourceIndex].ModListNew.push_back(mod);
            continue;
        }

        long fileOffset, size;
        std::copy(mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].FileOffset, mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].FileOffset + 8, (std::byte *)&fileOffset);
        std::copy(mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].FileOffset + 8, mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].FileOffset + 16, (std::byte *)&size);

        long sizeDiff = (long) mod.FileBytes.size() - size;

        if (sizeDiff > 0) {
            long length = (long)std::filesystem::file_size(ResourceList[resourceIndex].Path);
            mem.munmap_file();
            std::filesystem::resize_file(ResourceList[resourceIndex].Path, length + sizeDiff);
            mem.mmap_file(ResourceList[resourceIndex].Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, length + sizeDiff, mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);
            int toRead;

            const int BufferSize = 4096;
            std::byte buffer[BufferSize];

            while (length > (fileOffset + size)) {
                toRead = length - BufferSize >= (fileOffset + size) ? BufferSize : (int)(length - fileOffset - size);
                length -= toRead;
                std::copy(mem.begin() + length, mem.begin() + length + toRead, buffer);
                std::copy(buffer, buffer + toRead, mem.begin() + length + sizeDiff);
            }

            std::copy(mod.FileBytes.begin(), mod.FileBytes.begin() + (long)mod.FileBytes.size(), mem.begin() + fileOffset);
        }
        else {
            std::copy(mod.FileBytes.begin(), mod.FileBytes.begin() + (long)mod.FileBytes.size(), mem.begin() + fileOffset);

            std::byte emptyArray[-sizeDiff];

            if (sizeDiff < 0) {
                std::copy(emptyArray, emptyArray - sizeDiff, mem.begin() + fileOffset + (long)mod.FileBytes.size());
            }
        }

        auto modFileBytesSizeVector = LongToVector((long)mod.FileBytes.size(), 8);
        std::copy(modFileBytesSizeVector.begin(), modFileBytesSizeVector.begin() + 8, mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].SizeOffset);
        std::copy(modFileBytesSizeVector.begin(), modFileBytesSizeVector.begin() + 8, mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].SizeOffset + 8);

        mem[ResourceList[resourceIndex].ChunkList[chunkIndex].SizeOffset + 0x30] = static_cast<std::byte>(0);

        if (sizeDiff > 0) {
            auto x = std::find(ResourceList[resourceIndex].ChunkList.begin(), ResourceList[resourceIndex].ChunkList.end(), ResourceList[resourceIndex].ChunkList[chunkIndex]);
            int index = (int)std::distance(ResourceList[resourceIndex].ChunkList.begin(), x);
            for (int i = index + 1; i < ResourceList[resourceIndex].ChunkList.size(); i++) {
                std::copy(mem.begin() + ResourceList[resourceIndex].ChunkList[i].FileOffset, mem.begin() + ResourceList[resourceIndex].ChunkList[i].FileOffset + 8, (std::byte *)&fileOffset);
                auto fileOffsetSizeDirVector = LongToVector(fileOffset + sizeDiff, 8);
                std::copy(fileOffsetSizeDirVector.begin(), fileOffsetSizeDirVector.begin() + 8, mem.begin() + ResourceList[resourceIndex].ChunkList[i].FileOffset);
            }
        }

        std::cout << "\tReplaced " << mod.Name << std::endl;
        fileCount++;
    }
    if (fileCount > 0) {
        std::cout << "Number of files replaced: " << GREEN << fileCount << " file(s) " << RESET << "in " << YELLOW << ResourceList[resourceIndex].Path << RESET << "." << std::endl;
    }
}
