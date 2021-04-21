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
#include <filesystem>
#include <algorithm>

#include "json/single_include/nlohmann/json.hpp"
#include "EternalModLoader.hpp"

void ReplaceChunks(mmap_allocator_namespace::mmappable_vector<std::byte>& mem, int resourceIndex)
{
    std::ios::sync_with_stdio(false);

    int fileCount = 0;

    for (auto& mod : ResourceList[resourceIndex].ModList) {
        int chunkIndex;

        if (mod.isBlangJson) {
            mod.Name = mod.Name.substr(mod.Name.find('/') + 1);
            mod.Name = std::filesystem::path(mod.Name).replace_extension(".blang");

            chunkIndex = GetChunk(mod.Name, resourceIndex);
            if (chunkIndex == -1) {
                continue;
            }
        }
        else {
            chunkIndex = GetChunk(mod.Name, resourceIndex);
            if (chunkIndex == -1) {
                ResourceList[resourceIndex].ModListNew.push_back(mod);
                continue;
            }
        }

        long fileOffset, size;
        std::copy(mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].FileOffset, mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].FileOffset + 8, (std::byte*)&fileOffset);
        std::copy(mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].FileOffset + 8, mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].FileOffset + 16, (std::byte*)&size);

        long sizeDiff = (long)mod.FileBytes.size() - size;

        if (mod.isBlangJson) {
            nlohmann::json blangJson;
            try {
                blangJson = nlohmann::json::parse(std::string((char*)mod.FileBytes.data(), mod.FileBytes.size()));

                if (blangJson == NULL || blangJson["strings"].empty()) {
                    throw std::exception();
                }

                for (auto& blangJsonString : blangJson["strings"]) {
                    if (blangJsonString == NULL || blangJsonString["name"].empty() || blangJsonString["text"].empty()) {
                        throw std::exception();
                    }
                }
            }
            catch (const std::exception& e) {
                std::cout << RED << "ERROR: " << RESET << "Failed to parse EternalMod/strings/" << std::filesystem::path(mod.Name).replace_extension(".json").string() << std::endl;
                continue;
            }

            std::vector<std::byte> blangFileBytes(size);
            std::copy(mem.begin() + fileOffset, mem.begin() + fileOffset + size, blangFileBytes.begin());

            std::vector<std::byte> decryptedBlangFileBytes = IdCrypt(blangFileBytes, mod.Name, true);
            if (decryptedBlangFileBytes.empty()) {
                std::cout << RED << "ERROR: " << RESET << "Failed to decrypt " << ResourceList[resourceIndex].Name << "/" << mod.Name << std::endl;
                continue;
            }

            BlangFile blangFile;
            try {
                blangFile = ParseBlang(decryptedBlangFileBytes, ResourceList[resourceIndex].Name);
            }
            catch (const std::exception& e) {
                std::cout << RED << "ERROR: " << RESET << "Failed to parse " << ResourceList[resourceIndex].Name << "/" << mod.Name << std::endl;
                continue;
            }

            for (auto& blangJsonString : blangJson["strings"]) {
                bool stringFound = false;

                for (auto& blangString : blangFile.Strings) {
                    if (blangJsonString["name"] == blangString.Identifier) {
                        stringFound = true;
                        blangString.Text = blangJsonString["text"];

                        std::cout << "\tReplaced " << blangString.Identifier << " in " << mod.Name << std::endl;
                        break;
                    }
                }

                if (stringFound) {
                    continue;
                }

                BlangString newBlangString;
                newBlangString.Identifier = blangJsonString["name"];
                newBlangString.Text = blangJsonString["text"];

                std::cout << "\tAdded " << blangJsonString["name"].get<std::string>() << " in " << mod.Name << std::endl;
            }

            std::vector<std::byte> cryptDataBuffer = WriteBlangToVector(blangFile, ResourceList[resourceIndex].Name);

            std::vector<std::byte> encryptedBlangFileBytes = IdCrypt(cryptDataBuffer, mod.Name, false);
            if (encryptedBlangFileBytes.empty()) {
                std::cout << RED << "ERROR: " << RESET << "Failed to re-encrypt " << ResourceList[resourceIndex].Name << "/" << mod.Name << std::endl;
                continue;
            }

            mod.FileBytes = encryptedBlangFileBytes;
        }

        if (sizeDiff > 0) {
            long length = (long)std::filesystem::file_size(ResourceList[resourceIndex].Path);
            mem.munmap_file();
            std::filesystem::resize_file(ResourceList[resourceIndex].Path, length + sizeDiff);
            mem.mmap_file(ResourceList[resourceIndex].Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, length + sizeDiff, mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);
            int toRead;

            const int bufferSize = 4096;
            std::byte buffer[bufferSize];

            while (length > (fileOffset + size)) {
                toRead = length - bufferSize >= (fileOffset + size) ? bufferSize : (int)(length - fileOffset - size);
                length -= toRead;
                std::copy(mem.begin() + length, mem.begin() + length + toRead, buffer);
                std::copy(buffer, buffer + toRead, mem.begin() + length + sizeDiff);
            }

            std::copy(mod.FileBytes.begin(), mod.FileBytes.begin() + (long)mod.FileBytes.size(), mem.begin() + fileOffset);
        }
        else {
            std::copy(mod.FileBytes.begin(), mod.FileBytes.begin() + (long)mod.FileBytes.size(), mem.begin() + fileOffset);

            std::byte emptyArray[-sizeDiff] = {};

            if (sizeDiff < 0) {
                std::copy(emptyArray, emptyArray - sizeDiff, mem.begin() + fileOffset + (long)mod.FileBytes.size());
            }
        }

        auto modFileBytesSizeVector = LongToVector((long)mod.FileBytes.size(), 8);
        std::copy(modFileBytesSizeVector.begin(), modFileBytesSizeVector.begin() + 8, mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].SizeOffset);
        std::copy(modFileBytesSizeVector.begin(), modFileBytesSizeVector.begin() + 8, mem.begin() + ResourceList[resourceIndex].ChunkList[chunkIndex].SizeOffset + 8);

        mem[ResourceList[resourceIndex].ChunkList[chunkIndex].SizeOffset + 0x30] = (std::byte)0;

        if (sizeDiff > 0) {
            auto x = std::find(ResourceList[resourceIndex].ChunkList.begin(), ResourceList[resourceIndex].ChunkList.end(), ResourceList[resourceIndex].ChunkList[chunkIndex]);
            int index = (int)std::distance(ResourceList[resourceIndex].ChunkList.begin(), x);
            for (int i = index + 1; i < ResourceList[resourceIndex].ChunkList.size(); i++) {
                std::copy(mem.begin() + ResourceList[resourceIndex].ChunkList[i].FileOffset, mem.begin() + ResourceList[resourceIndex].ChunkList[i].FileOffset + 8, (std::byte*)&fileOffset);
                auto fileOffsetSizeDirVector = LongToVector(fileOffset + sizeDiff, 8);
                std::copy(fileOffsetSizeDirVector.begin(), fileOffsetSizeDirVector.begin() + 8, mem.begin() + ResourceList[resourceIndex].ChunkList[i].FileOffset);
            }
        }

        if (!mod.isBlangJson) {
            std::cout << "\tReplaced " << mod.Name << std::endl;
        }

        fileCount++;
    }
    if (fileCount > 0) {
        std::cout << "Number of files replaced: " << GREEN << fileCount << " file(s) " << RESET << "in " << YELLOW << ResourceList[resourceIndex].Path << RESET << "." << std::endl;
    }
}
