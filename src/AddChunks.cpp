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
#include <sstream>
#include "Colors.hpp"
#include "Oodle.hpp"
#include "ProgramOptions.hpp"
#include "Utils.hpp"
#include "AddChunks.hpp"
#include "murmur2/MurmurHash2.h"

void AddChunks(MemoryMappedFile& memoryMappedFile, ResourceContainer& resourceContainer,
    std::map<uint64_t, ResourceDataEntry>& resourceDataMap, std::stringstream& os)
{
    if (resourceContainer.NewModFileList.empty()) {
        return;
    }

    // Sort mod file list by priority
    std::stable_sort(resourceContainer.NewModFileList.begin(), resourceContainer.NewModFileList.end(),
        [](const ResourceModFile& resource1, const ResourceModFile& resource2) { return resource1.Parent.LoadPriority > resource2.Parent.LoadPriority; });

    // Get individual sections
    std::vector<std::byte> header(memoryMappedFile.Mem, memoryMappedFile.Mem + resourceContainer.InfoOffset);

    std::vector<std::byte> info(memoryMappedFile.Mem + resourceContainer.InfoOffset, memoryMappedFile.Mem + resourceContainer.NamesOffset);

    std::vector<std::byte> nameOffsets(memoryMappedFile.Mem + resourceContainer.NamesOffset, memoryMappedFile.Mem + resourceContainer.NamesOffsetEnd);

    std::vector<std::byte> names(memoryMappedFile.Mem + resourceContainer.NamesOffsetEnd, memoryMappedFile.Mem + resourceContainer.UnknownOffset);

    std::vector<std::byte> unknown(memoryMappedFile.Mem + resourceContainer.UnknownOffset, memoryMappedFile.Mem + resourceContainer.Dummy7Offset);

    size_t nameIdsOffset = resourceContainer.Dummy7Offset + (resourceContainer.TypeCount * 4);

    std::vector<std::byte> typeIds(memoryMappedFile.Mem + resourceContainer.Dummy7Offset, memoryMappedFile.Mem + nameIdsOffset);

    std::vector<std::byte> nameIds(memoryMappedFile.Mem + nameIdsOffset, memoryMappedFile.Mem + resourceContainer.IdclOffset);

    std::vector<std::byte> idcl(memoryMappedFile.Mem + resourceContainer.IdclOffset, memoryMappedFile.Mem + resourceContainer.DataOffset);

    std::vector<std::byte> data(memoryMappedFile.Mem + resourceContainer.DataOffset, memoryMappedFile.Mem + memoryMappedFile.Size);
    size_t originalDataSize = data.size();

    size_t infoOldLength = info.size();
    size_t nameIdsOldLength = nameIds.size();
    size_t newChunksCount = 0;
    size_t addedCount = 0;

    // Find the resource data for the new mod files and set them
    for (auto& modFile : resourceContainer.ModFileList) {
        if (modFile.IsAssetsInfoJson && modFile.AssetsInfo.has_value() && !modFile.AssetsInfo.value().Assets.empty()) {
            for (auto& newModFile : resourceContainer.NewModFileList) {
                for (auto& assetsInfoAssets : modFile.AssetsInfo.value().Assets) {
                    std::string normalPath = assetsInfoAssets.Name;
                    std::string declPath = normalPath;

                    if (!assetsInfoAssets.MapResourceType.empty()) {
                        declPath = "generated/decls/" + ToLower(assetsInfoAssets.MapResourceType) + "/" + assetsInfoAssets.Name + ".decl";
                    }

                    if (newModFile.Name == declPath || newModFile.Name == normalPath) {
                        newModFile.ResourceType = assetsInfoAssets.ResourceType.empty() ? "rs_streamfile" : assetsInfoAssets.ResourceType;
                        newModFile.Version = static_cast<unsigned short>(assetsInfoAssets.Version);
                        newModFile.StreamDbHash = assetsInfoAssets.StreamDbHash;
                        newModFile.SpecialByte1 = assetsInfoAssets.SpecialByte1;
                        newModFile.SpecialByte2 = assetsInfoAssets.SpecialByte2;
                        newModFile.SpecialByte3 = assetsInfoAssets.SpecialByte3;
                        newModFile.PlaceBefore = assetsInfoAssets.PlaceBefore;
                        newModFile.PlaceByName = assetsInfoAssets.PlaceByName;
                        newModFile.PlaceByType = assetsInfoAssets.PlaceByType;

                        if (ProgramOptions::Verbose) {
                            os << "\tSet resources type " << newModFile.ResourceType << " (version: " << newModFile.Version.value()
                                << ", streamdb hash: " << newModFile.StreamDbHash.value() << ") for new file: " << newModFile.Name << '\n';
                        }

                        break;
                    }
                }
            }
        }
    }

    // Add the new mod files now
    for (auto& modFile : resourceContainer.NewModFileList) {
        // Skip custom files
        if (modFile.IsAssetsInfoJson || modFile.IsBlangJson) {
            continue;
        }

        if (resourceContainer.ContainsResourceWithName(modFile.Name)) {
            if (ProgramOptions::Verbose) {
                os << Colors::Red << "WARNING: " << Colors::Reset << "Trying to add resource " << modFile.Name
                    << " that has already been added to " << resourceContainer.Name << ", skipping" << '\n';
            }

            modFile.FileBytes.resize(0);
            continue;
        }

        // Retrieve the resource data for this file (if needed & available)
        ResourceDataEntry resourceData;
        auto x = resourceDataMap.find(CalculateResourceFileNameHash(modFile.Name));

        if (x != resourceDataMap.end()) {
            resourceData = x->second;

            modFile.ResourceType = modFile.ResourceType.empty() ? resourceData.ResourceType : modFile.ResourceType;
            modFile.Version = !modFile.Version.has_value() ? static_cast<unsigned short>(resourceData.Version) : modFile.Version;
            modFile.StreamDbHash = !modFile.StreamDbHash.has_value() ? resourceData.StreamDbHash : modFile.StreamDbHash;
            modFile.SpecialByte1 = !modFile.SpecialByte1.has_value() ? resourceData.SpecialByte1 : modFile.SpecialByte1;
            modFile.SpecialByte2 = !modFile.SpecialByte2.has_value() ? resourceData.SpecialByte2 : modFile.SpecialByte2;
            modFile.SpecialByte3 = !modFile.SpecialByte3.has_value() ? resourceData.SpecialByte3 : modFile.SpecialByte3;
        }

        // Use rs_streamfile by default if no data was found or specified
        if (modFile.ResourceType.empty() && !modFile.Version.has_value() && !modFile.StreamDbHash.has_value()) {
            modFile.ResourceType = "rs_streamfile";
            modFile.Version = 0;
            modFile.StreamDbHash = 0;
            modFile.SpecialByte1 = std::byte{0};
            modFile.SpecialByte2 = std::byte{0};
            modFile.SpecialByte3 = std::byte{0};

            if (ProgramOptions::Verbose && !EndsWith(modFile.Name, ".decl")) {
                os << Colors::Red << "WARNING: " << Colors::Reset << "No resource data found for file: " << modFile.Name << '\n';
            }
        }

        // Check if the resource type name exists in the current container, and add it if it doesn't
        if (!modFile.ResourceType.empty()) {
            bool found = false;

            for (auto& name : resourceContainer.NamesList) {
                if (name.NormalizedFileName == modFile.ResourceType) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                // Add type name
                uint64_t typeLastOffset;
                std::copy(nameOffsets.end() - 8, nameOffsets.end(), reinterpret_cast<std::byte*>(&typeLastOffset));

                uint64_t typeLastNameOffset = 0;

                for (size_t i = typeLastOffset; i < names.size(); i++) {
                    if (names[i] == std::byte{0}) {
                        typeLastNameOffset = i + 1;
                        break;
                    }
                }

                names.resize(names.size() + modFile.ResourceType.size() + 1);
                std::copy(reinterpret_cast<const std::byte*>(modFile.ResourceType.c_str()),
                    reinterpret_cast<const std::byte*>(modFile.ResourceType.c_str()) + modFile.ResourceType.size() + 1, names.begin() + typeLastNameOffset);

                // Add type name offset
                uint64_t typeNewCount;
                std::copy(nameOffsets.begin(), nameOffsets.begin() + 8, reinterpret_cast<std::byte*>(&typeNewCount));
                typeNewCount += 1;

                std::copy(reinterpret_cast<std::byte*>(&typeNewCount), reinterpret_cast<std::byte*>(&typeNewCount) + 8, nameOffsets.begin());
                nameOffsets.resize(nameOffsets.size() + 8);
                std::copy(reinterpret_cast<std::byte*>(&typeLastNameOffset), reinterpret_cast<std::byte*>(&typeLastNameOffset) + 8, nameOffsets.end() - 8);

                // Add the type name to the list to keep the indexes in the proper order
                ResourceName newResourceName(modFile.ResourceType, modFile.ResourceType);
                resourceContainer.NamesList.push_back(newResourceName);

                os << "\tAdded resource type name " << modFile.ResourceType << " to " << resourceContainer.Name << '\n';
            }
        }

        // Add file name
        uint64_t lastOffset;
        std::copy(nameOffsets.end() - 8, nameOffsets.end(), reinterpret_cast<std::byte*>(&lastOffset));

        uint64_t lastNameOffset = 0;

        for (size_t i = lastOffset; i < names.size(); i++) {
            if (names[i] == std::byte{0}) {
                lastNameOffset = i + 1;
                break;
            }
        }

        auto nameChars = reinterpret_cast<const std::byte*>(modFile.Name.c_str());
        names.resize(names.size() + modFile.Name.size() + 1);
        std::copy(nameChars, nameChars + modFile.Name.size() + 1, names.begin() + lastNameOffset);

        // Add name offset
        uint64_t newCount;
        std::copy(nameOffsets.begin(), nameOffsets.begin() + 8, reinterpret_cast<std::byte*>(&newCount));
        newCount += 1;

        std::copy(reinterpret_cast<std::byte*>(&newCount), reinterpret_cast<std::byte*>(&newCount) + 8, nameOffsets.begin());
        nameOffsets.resize(nameOffsets.size() + 8);

        std::copy(reinterpret_cast<std::byte*>(&lastNameOffset), reinterpret_cast<std::byte*>(&lastNameOffset) + 8, nameOffsets.end() - 8);

        // Add the name to the list to keep the indexes in the proper order
        ResourceName newResourceName(modFile.Name, modFile.Name);
        resourceContainer.NamesList.push_back(newResourceName);

        // If this is a texture, check if it's compressed, or compress if necessary
        uint64_t compressedSize = modFile.FileBytes.size();
        uint64_t uncompressedSize = compressedSize;
        std::byte compressionMode{0};

        if ((modFile.Name.find(".tga") != std::string::npos || modFile.Name.find(".png") != std::string::npos) && compressedSize != 0) {
            // Check if it's a DIVINITY compressed texture
            if (std::memcmp(modFile.FileBytes.data(), "DIVINITY", 8) == 0) {
                // This is a compressed texture, read the uncompressed size
                std::copy(modFile.FileBytes.begin() + 8, modFile.FileBytes.begin() + 16, reinterpret_cast<std::byte*>(&uncompressedSize));

                // Set the compressed texture data, skipping the DIVINITY header (16 bytes)
                modFile.FileBytes = std::vector<std::byte>(modFile.FileBytes.begin() + 16, modFile.FileBytes.end());
                compressedSize = modFile.FileBytes.size();
                compressionMode = std::byte{2};

                if (ProgramOptions::Verbose) {
                    os << "\tSuccessfully set compressed texture data for file " << modFile.Name << '\n';
                }
            }
            else if (ProgramOptions::CompressTextures) {
                // Compress the texture
                std::vector<std::byte> compressedData;

                try {
                    compressedData = Oodle::Compress(modFile.FileBytes);

                    if (compressedData.empty()) {
                        throw std::exception();
                    }
                }
                catch (...) {
                    os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to compress " << modFile.Name << '\n';
                    continue;
                }

                modFile.FileBytes = compressedData;
                compressedSize = compressedData.size();
                compressionMode = std::byte{2};

                if (ProgramOptions::Verbose) {
                    os << "\tSuccessfully compressed texture file " << modFile.Name << '\n';
                }
            }
        }

        // Add the mode file data at the end of the data vector
        size_t resourceFileSize = memoryMappedFile.Size;
        size_t placement = 0x10 - (data.size() % 0x10) + 0x30;
        uint64_t fileOffset = resourceFileSize + (data.size() - originalDataSize) + placement;
        data.resize(data.size() + placement + modFile.FileBytes.size());
        std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), data.end() - modFile.FileBytes.size());

        // Add the asset type name id, if it's not found, use zero
        int64_t nameId = resourceContainer.GetResourceNameId(modFile.Name);
        nameIds.resize(nameIds.size() + 16);
        uint64_t nameIdOffset = ((nameIds.size() - 8) / 8) - 1;

        int64_t assetTypeNameId = resourceContainer.GetResourceNameId(modFile.ResourceType);

        if (assetTypeNameId == -1) {
            assetTypeNameId = 0;
        }

        // Add the asset type name id
        std::copy(reinterpret_cast<std::byte*>(&assetTypeNameId), reinterpret_cast<std::byte*>(&assetTypeNameId) + 8, nameIds.end() - 16);

        // Add the asset filename name id
        std::copy(reinterpret_cast<std::byte*>(&nameId), reinterpret_cast<std::byte*>(&nameId) + 8, nameIds.end() - 8);

        // Create the file info section
        std::byte newFileInfo[0x90];
        std::copy(info.end() - 0x90, info.end(), newFileInfo);

        std::copy(reinterpret_cast<std::byte*>(&nameIdOffset),
            reinterpret_cast<std::byte*>(&nameIdOffset) + 8, newFileInfo + sizeof(newFileInfo) - 0x70);
        std::copy(reinterpret_cast<std::byte*>(&fileOffset),
            reinterpret_cast<std::byte*>(&fileOffset) + 8, newFileInfo + sizeof(newFileInfo) - 0x58);
        std::copy(reinterpret_cast<std::byte*>(&compressedSize),
            reinterpret_cast<std::byte*>(&compressedSize) + 8, newFileInfo + sizeof(newFileInfo) - 0x50);
        std::copy(reinterpret_cast<std::byte*>(&uncompressedSize),
            reinterpret_cast<std::byte*>(&uncompressedSize) + 8, newFileInfo + sizeof(newFileInfo) - 0x48);

        // Set the DataMurmurHash
	uint64_t hash = MurmurHash64B(modFile.FileBytes.data(), modFile.FileBytes.size(), 0xdeadbeef);
        std::copy(reinterpret_cast<std::byte*>(&hash),
            reinterpret_cast<std::byte*>(&hash) + 8, newFileInfo + sizeof(newFileInfo) - 0x40);

        // Set the StreamDB resource hash
        std::copy(reinterpret_cast<std::byte*>(&modFile.StreamDbHash.value()),
            reinterpret_cast<std::byte*>(&modFile.StreamDbHash.value()) + 8, newFileInfo + sizeof(newFileInfo) - 0x30);

        // Set the correct asset version
        int version = modFile.Version.value();
        std::copy(reinterpret_cast<std::byte*>(&version),
            reinterpret_cast<std::byte*>(&version) + 4, newFileInfo + sizeof(newFileInfo) - 0x28);

        // Set the special std::byte values
        auto SpecialByte1Int = static_cast<unsigned int>(modFile.SpecialByte1.value());
        auto SpecialByte2Int = static_cast<unsigned int>(modFile.SpecialByte2.value());
        auto SpecialByte3Int = static_cast<unsigned int>(modFile.SpecialByte3.value());

        std::copy(reinterpret_cast<std::byte*>(&SpecialByte1Int),
            reinterpret_cast<std::byte*>(&SpecialByte1Int) + 4, newFileInfo + sizeof(newFileInfo) - 0x24);
        std::copy(reinterpret_cast<std::byte*>(&SpecialByte2Int),
            reinterpret_cast<std::byte*>(&SpecialByte2Int) + 4, newFileInfo + sizeof(newFileInfo) - 0x1E);
        std::copy(reinterpret_cast<std::byte*>(&SpecialByte3Int),
            reinterpret_cast<std::byte*>(&SpecialByte3Int) + 4, newFileInfo + sizeof(newFileInfo) - 0x1D);

        // Clear the compression mode
        newFileInfo[sizeof(newFileInfo) - 0x20] = compressionMode;

        // Set meta entries to use to 0
        unsigned short metaEntries = 0;
        std::copy(reinterpret_cast<std::byte*>(&metaEntries),
            reinterpret_cast<std::byte*>(&metaEntries) + 2, newFileInfo + sizeof(newFileInfo) - 0x10);

        // Add the new file info section at the end
        info.resize(info.size() + 0x90);
        std::copy(newFileInfo, newFileInfo + sizeof(newFileInfo), info.end() - 0x90);

        if (modFile.Announce) {
            os << "\tAdded " << modFile.Name << '\n';
            addedCount++;
        }

        modFile.FileBytes.resize(0);
        newChunksCount++;
    }

    // Rebuild the entire container now
    size_t namesOffsetAdd = info.size() - infoOldLength;
    size_t newSize = nameOffsets.size() + names.size();
    size_t unknownAdd = namesOffsetAdd + (newSize - resourceContainer.StringsSize);
    size_t typeIdsAdd = unknownAdd;
    size_t nameIdsAdd = typeIdsAdd;
    size_t idclAdd = nameIdsAdd + (nameIds.size() - nameIdsOldLength);
    size_t dataAdd = idclAdd;

    unsigned int fileCountAdd = resourceContainer.FileCount + newChunksCount;
    std::copy(reinterpret_cast<std::byte*>(&fileCountAdd), reinterpret_cast<std::byte*>(&fileCountAdd) + 4, header.begin() + 0x20);

    unsigned int fileCount2Add = resourceContainer.FileCount2 + newChunksCount * 2;
    std::copy(reinterpret_cast<std::byte*>(&fileCount2Add), reinterpret_cast<std::byte*>(&fileCount2Add) + 4, header.begin() + 0x2C);

    std::copy(reinterpret_cast<std::byte*>(&newSize), reinterpret_cast<std::byte*>(&newSize) + 4, header.begin() + 0x38);

    uint64_t nameOffsetAdd = resourceContainer.NamesOffset + namesOffsetAdd;
    std::copy(reinterpret_cast<std::byte*>(&nameOffsetAdd), reinterpret_cast<std::byte*>(&nameOffsetAdd) + 8, header.begin() + 0x40);

    uint64_t unknownOffsetAdd = resourceContainer.UnknownOffset + unknownAdd;
    std::copy(reinterpret_cast<std::byte*>(&unknownOffsetAdd), reinterpret_cast<std::byte*>(&unknownOffsetAdd) + 8, header.begin() + 0x48);

    uint64_t unknownOffsetAdd2 = resourceContainer.UnknownOffset2 + unknownAdd;
    std::copy(reinterpret_cast<std::byte*>(&unknownOffsetAdd2), reinterpret_cast<std::byte*>(&unknownOffsetAdd2) + 8, header.begin() + 0x58);

    uint64_t dummy7OffsetAdd = resourceContainer.Dummy7Offset + typeIdsAdd;
    std::copy(reinterpret_cast<std::byte*>(&dummy7OffsetAdd), reinterpret_cast<std::byte*>(&dummy7OffsetAdd) + 8, header.begin() + 0x60);

    uint64_t dataOffsetAdd = resourceContainer.DataOffset + dataAdd;
    std::copy(reinterpret_cast<std::byte*>(&dataOffsetAdd), reinterpret_cast<std::byte*>(&dataOffsetAdd) + 8, header.begin() + 0x68);

    uint64_t idclOffsetAdd = resourceContainer.IdclOffset + idclAdd;
    std::copy(reinterpret_cast<std::byte*>(&idclOffsetAdd), reinterpret_cast<std::byte*>(&idclOffsetAdd) + 8, header.begin() + 0x74);

    info.reserve(2 * (0x38 + info.size() + 8));

    for (size_t i = 0; i < info.size() / 0x90; i++) {
        size_t fileOffset = 0x38 + (i * 0x90);

        uint64_t newOffsetPlusDataAdd;
        std::copy(info.begin() + fileOffset, info.begin() + fileOffset + 8, reinterpret_cast<std::byte*>(&newOffsetPlusDataAdd));
        newOffsetPlusDataAdd += dataAdd;

        std::copy(reinterpret_cast<std::byte*>(&newOffsetPlusDataAdd), reinterpret_cast<std::byte*>(&newOffsetPlusDataAdd) + 8, info.begin() + fileOffset);
    }

    // Rebuild the container now
    size_t pos = 0;

    std::copy(header.begin(), header.end(), memoryMappedFile.Mem + pos);
    pos += header.size();

    std::copy(info.begin(), info.end(), memoryMappedFile.Mem + pos);
    pos += info.size();

    std::copy(nameOffsets.begin(), nameOffsets.end(), memoryMappedFile.Mem + pos);
    pos += nameOffsets.size();

    std::copy(names.begin(), names.end(), memoryMappedFile.Mem + pos);
    pos += names.size();

    std::copy(unknown.begin(), unknown.end(), memoryMappedFile.Mem + pos);
    pos += unknown.size();

    std::copy(typeIds.begin(), typeIds.end(), memoryMappedFile.Mem + pos);
    pos += typeIds.size();

    std::copy(nameIds.begin(), nameIds.end(), memoryMappedFile.Mem + pos);
    pos += nameIds.size();

    std::copy(idcl.begin(), idcl.end(), memoryMappedFile.Mem + pos);
    pos += idcl.size();

    if (pos + data.size() > memoryMappedFile.Size) {
        size_t newContainerSize = pos + data.size();

        if (!memoryMappedFile.ResizeFile(newContainerSize)) {
            os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to resize " << resourceContainer.Path << '\n';
            return;
        }
    }

    std::copy(data.begin(), data.end(), memoryMappedFile.Mem + pos);

    if (addedCount != 0) {
        os << "Number of files added: " << Colors::Green << addedCount << " file(s) " << Colors::Reset << "in " << Colors::Yellow << resourceContainer.Path << Colors::Reset << "." << '\n';
    }

    if (ProgramOptions::SlowMode) {
        os.flush();
    }
}
