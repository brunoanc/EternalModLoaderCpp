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
#include <algorithm>

#include "EternalModLoader.hpp"

void AddChunks(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, ResourceContainer &resourceContainer)
{
    std::stable_sort(resourceContainer.NewModFileList.begin(), resourceContainer.NewModFileList.end(),
        [](ResourceModFile resource1, ResourceModFile resource2) { return resource1.Parent.LoadPriority > resource2.Parent.LoadPriority ? true : false; });

    if (resourceContainer.NewModFileList.empty())
        return;

    long fileSize = std::filesystem::file_size(resourceContainer.Path);

    std::vector<std::byte> header(mem.begin(), mem.begin() + resourceContainer.InfoOffset);

    std::vector<std::byte> info(mem.begin() + resourceContainer.InfoOffset, mem.begin() + resourceContainer.NamesOffset);

    std::vector<std::byte> nameOffsets(mem.begin() + resourceContainer.NamesOffset, mem.begin() + resourceContainer.NamesOffsetEnd);

    std::vector<std::byte> names(mem.begin() + resourceContainer.NamesOffsetEnd, mem.begin() + resourceContainer.UnknownOffset);

    std::vector<std::byte> unknown(mem.begin() + resourceContainer.UnknownOffset, mem.begin() + resourceContainer.Dummy7Offset);

    long nameIdsOffset = resourceContainer.Dummy7Offset + (resourceContainer.TypeCount * 4);

    std::vector<std::byte> typeIds(mem.begin() + resourceContainer.Dummy7Offset, mem.begin() + nameIdsOffset);

    std::vector<std::byte> nameIds(mem.begin() + nameIdsOffset, mem.begin() + resourceContainer.IdclOffset);

    std::vector<std::byte> idcl(mem.begin() + resourceContainer.IdclOffset, mem.begin() + resourceContainer.DataOffset);

    int infoOldLength = info.size();
    int nameIdsOldLength = nameIds.size();
    int newChunksCount = 0;

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
                        newModFile.Version = (unsigned short)assetsInfoAssets.Version;
                        newModFile.StreamDbHash = assetsInfoAssets.StreamDbHash;
                        newModFile.SpecialByte1 = assetsInfoAssets.SpecialByte1;
                        newModFile.SpecialByte2 = assetsInfoAssets.SpecialByte2;
                        newModFile.SpecialByte3 = assetsInfoAssets.SpecialByte3;
                        newModFile.PlaceBefore = assetsInfoAssets.PlaceBefore;
                        newModFile.PlaceByName = assetsInfoAssets.PlaceByName;
                        newModFile.PlaceByType = assetsInfoAssets.PlaceByType;

                        if (Verbose) {
                            std::cout << "\tSet resources type " << newModFile.ResourceType << " (version: " << newModFile.Version.value()
                                << ", streamdb hash: " << newModFile.StreamDbHash.value() << ") for new file: " << newModFile.Name << std::endl;
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
                std::cerr << RED << "WARNING: " << RESET << "Trying to add resource " << modFile.Name
                    << " that has already been added to " << resourceContainer.Name << ", skipping" << std::endl;
            }

            continue;
        }

        ResourceDataEntry resourceData;
        std::map<unsigned long, ResourceDataEntry>::iterator x = ResourceDataMap.find(CalculateResourceFileNameHash(modFile.Name));

        if (x != ResourceDataMap.end()) {
            resourceData = x->second;

            modFile.ResourceType = modFile.ResourceType.empty() ? resourceData.ResourceType : modFile.ResourceType;
            modFile.Version = !modFile.Version.has_value() ? (unsigned short)resourceData.Version : modFile.Version;
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

            if (Verbose) {
                std::cerr << RED << "WARNING: " << RESET << "No resource data found for file: " << modFile.Name << std::endl;
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
                long typeLastOffset;
                std::copy(nameOffsets.end() - 8, nameOffsets.end(), (std::byte*)&typeLastOffset);

                long typeLastNameOffset = 0;

                for (int i = typeLastOffset; i < names.size(); i++) {
                    if (names[i] == (std::byte)0) {
                        typeLastNameOffset = i + 1;
                        break;
                    }
                }

                names.resize(names.size() + modFile.ResourceType.size() + 1);
                std::copy((std::byte*)modFile.ResourceType.c_str(), (std::byte*)modFile.ResourceType.c_str() + modFile.ResourceType.size() + 1, names.begin() + typeLastNameOffset);

                long typeNewCount;
                std::copy(nameOffsets.begin(), nameOffsets.begin() + 8, (std::byte*)&typeNewCount);
                typeNewCount += 1;

                std::copy((std::byte*)&typeNewCount, (std::byte*)&typeNewCount + 8, nameOffsets.begin());
                nameOffsets.resize(nameOffsets.size() + 8);
                std::copy((std::byte*)&typeLastNameOffset, (std::byte*)&typeLastNameOffset + 8, nameOffsets.end() - 8);

                ResourceName newResourceName(modFile.ResourceType, modFile.ResourceType);
                resourceContainer.NamesList.push_back(newResourceName);

                std::cout << "\tAdded resource type name " << modFile.ResourceType << " to " << resourceContainer.Name << std::endl;
            }
        }

        long lastOffset;
        std::copy(nameOffsets.end() - 8, nameOffsets.end(), (std::byte*)&lastOffset);

        long lastNameOffset = 0;

        for (int i = lastOffset; i < names.size(); i++) {
            if (names[i] == (std::byte)0) {
                lastNameOffset = i + 1;
                break;
            }
        }

        std::byte *nameChars = (std::byte*)modFile.Name.c_str();
        names.resize(names.size() + modFile.Name.size() + 1);
        std::copy(nameChars, nameChars + modFile.Name.size() + 1, names.begin() + lastNameOffset);

        long newCount;
        std::copy(nameOffsets.begin(), nameOffsets.begin() + 8, (std::byte*)&newCount);
        newCount += 1;

        std::copy((std::byte*)&newCount, (std::byte*)&newCount + 8, nameOffsets.begin());
        nameOffsets.resize(nameOffsets.size() + 8);

        std::copy((std::byte*)&lastNameOffset, (std::byte*)&lastNameOffset + 8, nameOffsets.end() - 8);

        ResourceName newResourceName(modFile.Name, modFile.Name);
        resourceContainer.NamesList.push_back(newResourceName);

        long currentDataSectionLength = mem.size() - resourceContainer.DataOffset;
        long placement = 0x10 - (currentDataSectionLength % 0x10) + 0x30;
        long newContainerSize = mem.size() + modFile.FileBytes.size() + placement;
        long fileOffset = newContainerSize - modFile.FileBytes.size();

        try {
            mem.munmap_file();
            std::filesystem::resize_file(resourceContainer.Path, newContainerSize);

            mem.mmap_file(resourceContainer.Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, newContainerSize,
                mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);

            if (mem.empty())
                throw std::exception();
        }
        catch (...) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to resize " << resourceContainer.Path << std::endl;
            return;
        }

        std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), mem.begin() + fileOffset);

        long nameId = resourceContainer.GetResourceNameId(modFile.Name);
        nameIds.resize(nameIds.size() + 8);
        long nameIdOffset = (nameIds.size() / 8) - 1;
        nameIds.resize(nameIds.size() + 8);

        long assetTypeNameId = resourceContainer.GetResourceNameId(modFile.ResourceType);

        if (assetTypeNameId == -1)
            assetTypeNameId = 0;

        std::copy((std::byte*)&assetTypeNameId, (std::byte*)&assetTypeNameId + 8, nameIds.end() - 16);
        std::copy((std::byte*)&nameId, (std::byte*)&nameId + 8, nameIds.end() - 8);

        long newInfoSectionOffset = -1;

        /*if (!modFile.PlaceByName.empty()) {
            long existingNameId = -1;
            long existingNameOffset = -1;

            if (!modFile.PlaceByType.empty()) {
                existingNameId = resourceContainer.GetResourceNameId("generated/decls/" + ToLower(modFile.PlaceByType) + "/" + modFile.PlaceByName + ".decl");
            }

            if (existingNameId == -1) {
                existingNameId = resourceContainer.GetResourceNameId(modFile.PlaceByName);
            }

            if (existingNameId != -1) {
                for (int i = 0, j = nameIds.size() / 8; i < j; i++) {
                    long currentNameId;
                    std::copy(nameIds.begin() + i * 8, nameIds.begin() + i * 8 + 8, (std::byte*)&currentNameId);

                    if (currentNameId == existingNameId) {
                        existingNameOffset = i - 1;
                        break;
                    }
                }

                if (existingNameOffset != -1) {
                    int pos = 0;

                    for (int i = 0, j = info.size() / 0x90; i < j; i++) {
                        pos += 32;
                        long nameOffset;
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

        long fileBytesSize = modFile.FileBytes.size();

        std::copy((std::byte*)&fileBytesSize, (std::byte*)&fileBytesSize + 8, newFileInfo + sizeof(newFileInfo) - 0x50);
        std::copy((std::byte*)&fileBytesSize, (std::byte*)&fileBytesSize + 8, newFileInfo + sizeof(newFileInfo) - 0x48);

        std::copy((std::byte*)&modFile.StreamDbHash.value(), (std::byte*)&modFile.StreamDbHash.value() + 8, newFileInfo + sizeof(newFileInfo) - 0x40);
        std::copy((std::byte*)&modFile.StreamDbHash.value(), (std::byte*)&modFile.StreamDbHash.value() + 8, newFileInfo + sizeof(newFileInfo) - 0x30);

        int version = modFile.Version.value();
        std::copy((std::byte*)&version, (std::byte*)&version + 4, newFileInfo + sizeof(newFileInfo) - 0x28);

        int SpecialByte1Int = (int)modFile.SpecialByte1.value();
        int SpecialByte2Int = (int)modFile.SpecialByte2.value();
        int SpecialByte3Int = (int)modFile.SpecialByte3.value();

        std::copy((std::byte*)&SpecialByte1Int, (std::byte*)&SpecialByte1Int + 4, newFileInfo + sizeof(newFileInfo) - 0x24);
        std::copy((std::byte*)&SpecialByte2Int, (std::byte*)&SpecialByte2Int + 4, newFileInfo + sizeof(newFileInfo) - 0x1E);
        std::copy((std::byte*)&SpecialByte3Int, (std::byte*)&SpecialByte3Int + 4, newFileInfo + sizeof(newFileInfo) - 0x1D);

        newFileInfo[sizeof(newFileInfo) - 0x20] = (std::byte)0;

        short metaEntries = 0;
        std::copy((std::byte*)&metaEntries, (std::byte*)&metaEntries + 2, newFileInfo + sizeof(newFileInfo) - 0x10);

        info.resize(info.size() + 0x90);

        if (newInfoSectionOffset != -1 /*&& modFile.ResourceType == "rs_streamfile"*/) {
            std::byte buffer[info.size() - newInfoSectionOffset - 0x90];
            std::copy(info.begin() + newInfoSectionOffset, info.begin() + newInfoSectionOffset + sizeof(buffer), buffer);
            std::copy(buffer, buffer + sizeof(buffer), info.begin() + newInfoSectionOffset + 0x90);
            std::copy(newFileInfo, newFileInfo + sizeof(newFileInfo), info.begin() + newInfoSectionOffset);
        }
        else {
            std::copy(newFileInfo, newFileInfo + sizeof(newFileInfo), info.end() - 0x90);
        }

        std::cout << "\tAdded " << modFile.Name << std::endl;
        newChunksCount++;
    }

    long namesOffsetAdd = info.size() - infoOldLength;
    long newSize = nameOffsets.size() + names.size();
    long unknownAdd = namesOffsetAdd + (newSize - resourceContainer.StringsSize);
    long typeIdsAdd = unknownAdd;
    long nameIdsAdd = typeIdsAdd;
    long idclAdd = nameIdsAdd + (nameIds.size() - nameIdsOldLength);
    long dataAdd = idclAdd;

    int fileCountAdd = resourceContainer.FileCount + newChunksCount;
    std::copy((std::byte*)&fileCountAdd, (std::byte*)&fileCountAdd + 4, header.begin() + 0x20);

    int fileCount2Add = resourceContainer.FileCount2 + newChunksCount * 2;
    std::copy((std::byte*)&fileCount2Add, (std::byte*)&fileCount2Add + 4, header.begin() + 0x2C);

    std::copy((std::byte*)&newSize, (std::byte*)&newSize + 4, header.begin() + 0x38);

    long nameOffsetAdd = resourceContainer.NamesOffset + namesOffsetAdd;
    std::copy((std::byte*)&nameOffsetAdd, (std::byte*)&nameOffsetAdd + 8, header.begin() + 0x40);

    long unknownOffsetAdd = resourceContainer.UnknownOffset + unknownAdd;
    std::copy((std::byte*)&unknownOffsetAdd, (std::byte*)&unknownOffsetAdd + 8, header.begin() + 0x48);

    long unknownOffsetAdd2 = resourceContainer.UnknownOffset2 + unknownAdd;
    std::copy((std::byte*)&unknownOffsetAdd2, (std::byte*)&unknownOffsetAdd2 + 8, header.begin() + 0x58);

    long dummy7OffsetAdd = resourceContainer.Dummy7Offset + typeIdsAdd;
    std::copy((std::byte*)&dummy7OffsetAdd, (std::byte*)&dummy7OffsetAdd + 8, header.begin() + 0x60);

    long dataOffsetAdd = resourceContainer.DataOffset + dataAdd;
    std::copy((std::byte*)&dataOffsetAdd, (std::byte*)&dataOffsetAdd + 8, header.begin() + 0x68);

    long idclOffsetAdd = resourceContainer.IdclOffset + idclAdd;
    std::copy((std::byte*)&idclOffsetAdd, (std::byte*)&idclOffsetAdd + 8, header.begin() + 0x74);

    info.reserve(2 * (0x38 + info.size() + 8));

    for (int i = 0; i < info.size() / 0x90; i++) {
        int fileOffset = 0x38 + (i * 0x90);

        long newOffsetPlusDataAdd;
        std::copy(info.begin() + fileOffset, info.begin() + fileOffset + 8, (std::byte*)&newOffsetPlusDataAdd);
        newOffsetPlusDataAdd += dataAdd;

        std::copy((std::byte*)&newOffsetPlusDataAdd, (std::byte*)&newOffsetPlusDataAdd + 8, info.begin() + fileOffset);
    }

    long dataSectionLength = mem.size() - resourceContainer.DataOffset;
    long newContainerSize = header.size() + info.size() + nameOffsets.size() + names.size() + unknown.size() + typeIds.size() + nameIds.size() + idcl.size() + dataSectionLength;

    const int bufferSize = 4096;
    std::byte buffer[bufferSize];

    long oldContainerSize = mem.size();
    long extraBytes = newContainerSize - oldContainerSize;
    long currentPos = oldContainerSize;
    int bytesToRead;

    try {
        mem.munmap_file();
        std::filesystem::resize_file(resourceContainer.Path, newContainerSize);

        mem.mmap_file(resourceContainer.Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, newContainerSize,
            mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);

        if (mem.empty())
            throw std::exception();
    }
    catch (...) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to resize " << resourceContainer.Path << std::endl;
        return;
    }

    while (currentPos > resourceContainer.DataOffset) {
        bytesToRead = currentPos - bufferSize >= resourceContainer.DataOffset ? bufferSize : currentPos - resourceContainer.DataOffset;
        currentPos -= bytesToRead;

        std::copy(mem.begin() + currentPos, mem.begin() + currentPos + bytesToRead, buffer);
        std::copy(buffer, buffer + bytesToRead, mem.begin() + currentPos + extraBytes);
    }

    unsigned long pos = 0;
    std::copy(header.begin(), header.end(), mem.begin() + pos);
    pos += header.size();

    std::copy(info.begin(), info.end(), mem.begin() + pos);
    pos += info.size();

    std::copy(nameOffsets.begin(), nameOffsets.end(), mem.begin() + pos);
    pos += nameOffsets.size();

    std::copy(names.begin(), names.end(), mem.begin() + pos);
    pos += names.size();

    std::copy(unknown.begin(), unknown.end(), mem.begin() + pos);
    pos += unknown.size();

    std::copy(typeIds.begin(), typeIds.end(), mem.begin() + pos);
    pos += typeIds.size();

    std::copy(nameIds.begin(), nameIds.end(), mem.begin() + pos);
    pos += nameIds.size();

    std::copy(idcl.begin(), idcl.end(), mem.begin() + pos);
    pos += idcl.size();

    if (newChunksCount != 0)
        std::cout << "Number of files added: " << GREEN << newChunksCount << " file(s) " << RESET << "in " << YELLOW << resourceContainer.Path << RESET << "." << std::endl;
}
