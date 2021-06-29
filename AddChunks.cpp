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
#include <iterator>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <sstream>

#include "EternalModLoader.hpp"

extern const std::byte *DivinityMagic;

/**
 * @brief Add new chunks to the given resource file
 * 
 * @param memoryMappedFile MemoryMappedFile object containing the resource to modify
 * @param resourceContainer ResourceContainer object containing the resources's data
 * @param os StringStream to output to
 */
void AddChunks(MemoryMappedFile &memoryMappedFile, ResourceContainer &resourceContainer, std::stringstream &os)
{
    if (resourceContainer.NewModFileList.empty())
        return;

    std::stable_sort(resourceContainer.NewModFileList.begin(), resourceContainer.NewModFileList.end(),
        [](const ResourceModFile &resource1, const ResourceModFile &resource2) { return resource1.Parent.LoadPriority > resource2.Parent.LoadPriority; });

    std::vector<std::byte> header(memoryMappedFile.Mem, memoryMappedFile.Mem + resourceContainer.InfoOffset);

    std::vector<std::byte> info(memoryMappedFile.Mem + resourceContainer.InfoOffset, memoryMappedFile.Mem + resourceContainer.NamesOffset);

    std::vector<std::byte> nameOffsets(memoryMappedFile.Mem + resourceContainer.NamesOffset, memoryMappedFile.Mem + resourceContainer.NamesOffsetEnd);

    std::vector<std::byte> names(memoryMappedFile.Mem + resourceContainer.NamesOffsetEnd, memoryMappedFile.Mem + resourceContainer.UnknownOffset);

    std::vector<std::byte> unknown(memoryMappedFile.Mem + resourceContainer.UnknownOffset, memoryMappedFile.Mem + resourceContainer.Dummy7Offset);

    int64_t nameIdsOffset = resourceContainer.Dummy7Offset + (resourceContainer.TypeCount * 4);

    std::vector<std::byte> typeIds(memoryMappedFile.Mem + resourceContainer.Dummy7Offset, memoryMappedFile.Mem + nameIdsOffset);

    std::vector<std::byte> nameIds(memoryMappedFile.Mem + nameIdsOffset, memoryMappedFile.Mem + resourceContainer.IdclOffset);

    std::vector<std::byte> idcl(memoryMappedFile.Mem + resourceContainer.IdclOffset, memoryMappedFile.Mem + resourceContainer.DataOffset);

    std::vector<std::byte> data(memoryMappedFile.Mem + resourceContainer.DataOffset, memoryMappedFile.Mem + memoryMappedFile.Size);
    int64_t originalDataSize = data.size();

    int32_t infoOldLength = info.size();
    int32_t nameIdsOldLength = nameIds.size();
    int32_t newChunksCount = 0;

    for (auto &modFile : resourceContainer.ModFileList) {
        if (modFile.IsAssetsInfoJson && modFile.AssetsInfo.has_value() && !modFile.AssetsInfo.value().Assets.empty()) {
            for (auto &newModFile : resourceContainer.NewModFileList) {
                for (auto &assetsInfoAssets : modFile.AssetsInfo.value().Assets) {
                    std::string normalPath = assetsInfoAssets.Name;
                    std::string declPath = normalPath;

                    if (!assetsInfoAssets.MapResourceType.empty())
                        declPath = "generated/decls/" + ToLower(assetsInfoAssets.MapResourceType) + "/" + assetsInfoAssets.Name + ".decl";

                    if (newModFile.Name == declPath || newModFile.Name == normalPath) {
                        newModFile.ResourceType = assetsInfoAssets.ResourceType.empty() ? "rs_streamfile" : assetsInfoAssets.ResourceType;
                        newModFile.Version = (uint16_t)assetsInfoAssets.Version;
                        newModFile.StreamDbHash = assetsInfoAssets.StreamDbHash;
                        newModFile.SpecialByte1 = assetsInfoAssets.SpecialByte1;
                        newModFile.SpecialByte2 = assetsInfoAssets.SpecialByte2;
                        newModFile.SpecialByte3 = assetsInfoAssets.SpecialByte3;
                        newModFile.PlaceBefore = assetsInfoAssets.PlaceBefore;
                        newModFile.PlaceByName = assetsInfoAssets.PlaceByName;
                        newModFile.PlaceByType = assetsInfoAssets.PlaceByType;

                        if (Verbose) {
                            os << "\tSet resources type " << newModFile.ResourceType << " (version: " << newModFile.Version.value()
                                << ", streamdb hash: " << newModFile.StreamDbHash.value() << ") for new file: " << newModFile.Name << '\n';
                        }

                        break;
                    }
                }
            }
        }
    }

    for (auto &modFile : resourceContainer.NewModFileList) {
        if (modFile.IsAssetsInfoJson || modFile.IsBlangJson)
            continue;

        if (resourceContainer.ContainsResourceWithName(modFile.Name)) {
            if (Verbose) {
                os << RED << "WARNING: " << RESET << "Trying to add resource " << modFile.Name
                    << " that has already been added to " << resourceContainer.Name << ", skipping" << '\n';
            }

            modFile.FileBytes.resize(0);
            continue;
        }

        ResourceDataEntry resourceData;
        std::map<uint64_t, ResourceDataEntry>::iterator x = ResourceDataMap.find(CalculateResourceFileNameHash(modFile.Name));

        if (x != ResourceDataMap.end()) {
            resourceData = x->second;

            modFile.ResourceType = modFile.ResourceType.empty() ? resourceData.ResourceType : modFile.ResourceType;
            modFile.Version = !modFile.Version.has_value() ? (uint16_t)resourceData.Version : modFile.Version;
            modFile.StreamDbHash = !modFile.StreamDbHash.has_value() ? resourceData.StreamDbHash : modFile.StreamDbHash;
            modFile.SpecialByte1 = !modFile.SpecialByte1.has_value() ? resourceData.SpecialByte1 : modFile.SpecialByte1;
            modFile.SpecialByte2 = !modFile.SpecialByte2.has_value() ? resourceData.SpecialByte2 : modFile.SpecialByte2;
            modFile.SpecialByte3 = !modFile.SpecialByte3.has_value() ? resourceData.SpecialByte3 : modFile.SpecialByte3;
        }

        if (modFile.ResourceType.empty() && !modFile.Version.has_value() && !modFile.StreamDbHash.has_value()) {
            modFile.ResourceType = "rs_streamfile";
            modFile.Version = 0;
            modFile.StreamDbHash = 0;
            modFile.SpecialByte1 = (std::byte)0;
            modFile.SpecialByte2 = (std::byte)0;
            modFile.SpecialByte3 = (std::byte)0;

            if (Verbose && !EndsWith(modFile.Name, ".decl")) {
                os << RED << "WARNING: " << RESET << "No resource data found for file: " << modFile.Name << '\n';
            }
        }

        if (!modFile.ResourceType.empty()) {
            bool found = false;

            for (auto &name : resourceContainer.NamesList) {
                if (name.NormalizedFileName == modFile.ResourceType) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                int64_t typeLastOffset;
                std::copy(nameOffsets.end() - 8, nameOffsets.end(), (std::byte*)&typeLastOffset);

                int64_t typeLastNameOffset = 0;

                for (int32_t i = typeLastOffset; i < names.size(); i++) {
                    if (names[i] == (std::byte)0) {
                        typeLastNameOffset = i + 1;
                        break;
                    }
                }

                names.resize(names.size() + modFile.ResourceType.size() + 1);
                std::copy((std::byte*)modFile.ResourceType.c_str(), (std::byte*)modFile.ResourceType.c_str() + modFile.ResourceType.size() + 1, names.begin() + typeLastNameOffset);

                int64_t typeNewCount;
                std::copy(nameOffsets.begin(), nameOffsets.begin() + 8, (std::byte*)&typeNewCount);
                typeNewCount += 1;

                std::copy((std::byte*)&typeNewCount, (std::byte*)&typeNewCount + 8, nameOffsets.begin());
                nameOffsets.resize(nameOffsets.size() + 8);
                std::copy((std::byte*)&typeLastNameOffset, (std::byte*)&typeLastNameOffset + 8, nameOffsets.end() - 8);

                ResourceName newResourceName(modFile.ResourceType, modFile.ResourceType);
                resourceContainer.NamesList.push_back(newResourceName);

                os << "\tAdded resource type name " << modFile.ResourceType << " to " << resourceContainer.Name << '\n';
            }
        }

        int64_t lastOffset;
        std::copy(nameOffsets.end() - 8, nameOffsets.end(), (std::byte*)&lastOffset);

        int64_t lastNameOffset = 0;

        for (int32_t i = lastOffset; i < names.size(); i++) {
            if (names[i] == (std::byte)0) {
                lastNameOffset = i + 1;
                break;
            }
        }

        std::byte *nameChars = (std::byte*)modFile.Name.c_str();
        names.resize(names.size() + modFile.Name.size() + 1);
        std::copy(nameChars, nameChars + modFile.Name.size() + 1, names.begin() + lastNameOffset);

        int64_t newCount;
        std::copy(nameOffsets.begin(), nameOffsets.begin() + 8, (std::byte*)&newCount);
        newCount += 1;

        std::copy((std::byte*)&newCount, (std::byte*)&newCount + 8, nameOffsets.begin());
        nameOffsets.resize(nameOffsets.size() + 8);

        std::copy((std::byte*)&lastNameOffset, (std::byte*)&lastNameOffset + 8, nameOffsets.end() - 8);

        ResourceName newResourceName(modFile.Name, modFile.Name);
        resourceContainer.NamesList.push_back(newResourceName);

        uint64_t compressedSize = modFile.FileBytes.size();
        uint64_t uncompressedSize = compressedSize;
        std::byte compressionMode = (std::byte)0;

        if (modFile.Name.find(".tga") != std::string::npos) {
            if (!memcmp(modFile.FileBytes.data(), DivinityMagic, 8)) {
                std::copy(modFile.FileBytes.begin() + 8, modFile.FileBytes.begin() + 16, (std::byte*)&uncompressedSize);

                modFile.FileBytes = std::vector<std::byte>(modFile.FileBytes.begin() + 16, modFile.FileBytes.end());
                compressedSize = modFile.FileBytes.size();
                compressionMode = (std::byte)2;

                if (Verbose)
                    os << "\tSuccessfully set compressed texture data for file " << modFile.Name << '\n';
            }
            else if (CompressTextures) {
                std::vector<std::byte> compressedData;

                try {
                    compressedData = OodleCompress(modFile.FileBytes, OodleFormat::Kraken, OodleCompressionLevel::Normal);

                    if (compressedData.empty())
                        throw std::exception();
                }
                catch (...) {
                    os << RED << "ERROR: " << RESET << "Failed to compress " << modFile.Name << '\n';
                    continue;
                }

                modFile.FileBytes = compressedData;
                compressedSize = compressedData.size();
                compressionMode = (std::byte)2;

                if (Verbose)
                    os << "\tSuccessfully compressed texture file " << modFile.Name << '\n';
            }
        }

        int64_t resourceFileSize = memoryMappedFile.Size;
        int64_t placement = 0x10 - (data.size() % 0x10) + 0x30;
        int64_t fileOffset = resourceFileSize + (data.size() - originalDataSize) + placement;
        data.resize(data.size() + placement + modFile.FileBytes.size());
        std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), data.end() - modFile.FileBytes.size());

        int64_t nameId = resourceContainer.GetResourceNameId(modFile.Name);
        nameIds.resize(nameIds.size() + 16);
        int64_t nameIdOffset = ((nameIds.size() - 8) / 8) - 1;

        int64_t assetTypeNameId = resourceContainer.GetResourceNameId(modFile.ResourceType);

        if (assetTypeNameId == -1)
            assetTypeNameId = 0;

        std::copy((std::byte*)&assetTypeNameId, (std::byte*)&assetTypeNameId + 8, nameIds.end() - 16);
        std::copy((std::byte*)&nameId, (std::byte*)&nameId + 8, nameIds.end() - 8);

        int64_t newInfoSectionOffset = -1;

        /*if (!modFile.PlaceByName.empty()) {
            int64_t existingNameId = -1;
            int64_t existingNameOffset = -1;
            if (!modFile.PlaceByType.empty()) {
                existingNameId = resourceContainer.GetResourceNameId("generated/decls/" + ToLower(modFile.PlaceByType) + "/" + modFile.PlaceByName + ".decl");
            }
            if (existingNameId == -1) {
                existingNameId = resourceContainer.GetResourceNameId(modFile.PlaceByName);
            }
            if (existingNameId != -1) {
                for (int32_t i = 0, j = nameIds.size() / 8; i < j; i++) {
                    int64_t currentNameId;
                    std::copy(nameIds.begin() + i * 8, nameIds.begin() + i * 8 + 8, (std::byte*)&currentNameId);
                    if (currentNameId == existingNameId) {
                        existingNameOffset = i - 1;
                        break;
                    }
                }
                if (existingNameOffset != -1) {
                    int32_t pos = 0;
                    for (int32_t i = 0, j = info.size() / 0x90; i < j; i++) {
                        pos += 32;
                        int64_t nameOffset;
                        std::copy(info.begin() + pos, info.begin() + pos + 8, (std::byte*)&nameOffset);
                        pos += 0x70;
                        if (nameOffset == existingNameOffset) {
                            newInfoSectionOffset = i * 0x90;
                            if (!modFile.PlaceBefore)
                                newInfoSectionOffset += 0x90;
                            break;
                        }
                    }
                }
            }
        }*/

        std::byte newFileInfo[0x90];
        std::copy(info.end() - 0x90, info.end(), newFileInfo);

        std::copy((std::byte*)&nameIdOffset, (std::byte*)&nameIdOffset + 8, newFileInfo + sizeof(newFileInfo) - 0x70);
        std::copy((std::byte*)&fileOffset, (std::byte*)&fileOffset + 8, newFileInfo + sizeof(newFileInfo) - 0x58);

        std::copy((std::byte*)&compressedSize, (std::byte*)&compressedSize + 8, newFileInfo + sizeof(newFileInfo) - 0x50);
        std::copy((std::byte*)&uncompressedSize, (std::byte*)&uncompressedSize + 8, newFileInfo + sizeof(newFileInfo) - 0x48);

        std::copy((std::byte*)&modFile.StreamDbHash.value(), (std::byte*)&modFile.StreamDbHash.value() + 8, newFileInfo + sizeof(newFileInfo) - 0x40);
        std::copy((std::byte*)&modFile.StreamDbHash.value(), (std::byte*)&modFile.StreamDbHash.value() + 8, newFileInfo + sizeof(newFileInfo) - 0x30);

        int32_t version = modFile.Version.value();
        std::copy((std::byte*)&version, (std::byte*)&version + 4, newFileInfo + sizeof(newFileInfo) - 0x28);

        int32_t SpecialByte1Int = (int32_t)modFile.SpecialByte1.value();
        int32_t SpecialByte2Int = (int32_t)modFile.SpecialByte2.value();
        int32_t SpecialByte3Int = (int32_t)modFile.SpecialByte3.value();

        std::copy((std::byte*)&SpecialByte1Int, (std::byte*)&SpecialByte1Int + 4, newFileInfo + sizeof(newFileInfo) - 0x24);
        std::copy((std::byte*)&SpecialByte2Int, (std::byte*)&SpecialByte2Int + 4, newFileInfo + sizeof(newFileInfo) - 0x1E);
        std::copy((std::byte*)&SpecialByte3Int, (std::byte*)&SpecialByte3Int + 4, newFileInfo + sizeof(newFileInfo) - 0x1D);

        newFileInfo[sizeof(newFileInfo) - 0x20] = compressionMode;

        int16_t metaEntries = 0;
        std::copy((std::byte*)&metaEntries, (std::byte*)&metaEntries + 2, newFileInfo + sizeof(newFileInfo) - 0x10);

        info.resize(info.size() + 0x90);

        if (newInfoSectionOffset != -1 /*&& modFile.ResourceType == "rs_streamfile"*/) {
            int64_t bufSize = info.size() - newInfoSectionOffset - 0x90;
            std::byte *buf = new std::byte[bufSize];
            std::copy(info.begin() + newInfoSectionOffset, info.begin() + newInfoSectionOffset + bufSize, buf);
            std::copy(buf, buf + bufSize, info.begin() + newInfoSectionOffset + 0x90);
            std::copy(newFileInfo, newFileInfo + sizeof(newFileInfo), info.begin() + newInfoSectionOffset);
        }
        else {
            std::copy(newFileInfo, newFileInfo + sizeof(newFileInfo), info.end() - 0x90);
        }

        os << "\tAdded " << modFile.Name << '\n';
        modFile.FileBytes.resize(0);
        newChunksCount++;
    }

    int64_t namesOffsetAdd = info.size() - infoOldLength;
    int64_t newSize = nameOffsets.size() + names.size();
    int64_t unknownAdd = namesOffsetAdd + (newSize - resourceContainer.StringsSize);
    int64_t typeIdsAdd = unknownAdd;
    int64_t nameIdsAdd = typeIdsAdd;
    int64_t idclAdd = nameIdsAdd + (nameIds.size() - nameIdsOldLength);
    int64_t dataAdd = idclAdd;

    int32_t fileCountAdd = resourceContainer.FileCount + newChunksCount;
    std::copy((std::byte*)&fileCountAdd, (std::byte*)&fileCountAdd + 4, header.begin() + 0x20);

    int32_t fileCount2Add = resourceContainer.FileCount2 + newChunksCount * 2;
    std::copy((std::byte*)&fileCount2Add, (std::byte*)&fileCount2Add + 4, header.begin() + 0x2C);

    std::copy((std::byte*)&newSize, (std::byte*)&newSize + 4, header.begin() + 0x38);

    int64_t nameOffsetAdd = resourceContainer.NamesOffset + namesOffsetAdd;
    std::copy((std::byte*)&nameOffsetAdd, (std::byte*)&nameOffsetAdd + 8, header.begin() + 0x40);

    int64_t unknownOffsetAdd = resourceContainer.UnknownOffset + unknownAdd;
    std::copy((std::byte*)&unknownOffsetAdd, (std::byte*)&unknownOffsetAdd + 8, header.begin() + 0x48);

    int64_t unknownOffsetAdd2 = resourceContainer.UnknownOffset2 + unknownAdd;
    std::copy((std::byte*)&unknownOffsetAdd2, (std::byte*)&unknownOffsetAdd2 + 8, header.begin() + 0x58);

    int64_t dummy7OffsetAdd = resourceContainer.Dummy7Offset + typeIdsAdd;
    std::copy((std::byte*)&dummy7OffsetAdd, (std::byte*)&dummy7OffsetAdd + 8, header.begin() + 0x60);

    int64_t dataOffsetAdd = resourceContainer.DataOffset + dataAdd;
    std::copy((std::byte*)&dataOffsetAdd, (std::byte*)&dataOffsetAdd + 8, header.begin() + 0x68);

    int64_t idclOffsetAdd = resourceContainer.IdclOffset + idclAdd;
    std::copy((std::byte*)&idclOffsetAdd, (std::byte*)&idclOffsetAdd + 8, header.begin() + 0x74);

    info.reserve(2 * (0x38 + info.size() + 8));

    for (int32_t i = 0; i < info.size() / 0x90; i++) {
        int32_t fileOffset = 0x38 + (i * 0x90);

        int64_t newOffsetPlusDataAdd;
        std::copy(info.begin() + fileOffset, info.begin() + fileOffset + 8, (std::byte*)&newOffsetPlusDataAdd);
        newOffsetPlusDataAdd += dataAdd;

        std::copy((std::byte*)&newOffsetPlusDataAdd, (std::byte*)&newOffsetPlusDataAdd + 8, info.begin() + fileOffset);
    }

    uint64_t pos = 0;
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
        int64_t newContainerSize = pos + data.size();

        if (!memoryMappedFile.ResizeFile(newContainerSize)) {
            os << RED << "ERROR: " << RESET << "Failed to resize " << resourceContainer.Path << '\n';
            return;
        }
    }

    std::copy(data.begin(), data.end(), memoryMappedFile.Mem + pos);

    if (newChunksCount != 0)
        os << "Number of files added: " << GREEN << newChunksCount << " file(s) " << RESET << "in " << YELLOW << resourceContainer.Path << RESET << "." << '\n';

    if (SlowMode)
        os.flush();
}