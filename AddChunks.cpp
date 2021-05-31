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

void AddChunks(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, ResourceInfo &resourceInfo)
{
    long fileSize = std::filesystem::file_size(resourceInfo.Path);

    if (resourceInfo.ModListNew.empty())
        return;

    std::vector<std::byte> header(mem.begin(), mem.begin() + resourceInfo.InfoOffset);

    std::vector<std::byte> info(mem.begin() + resourceInfo.InfoOffset, mem.begin() + resourceInfo.NamesOffset);

    std::vector<std::byte> nameOffsets(mem.begin() + resourceInfo.NamesOffset, mem.begin() + resourceInfo.NamesOffsetEnd);

    std::vector<std::byte> names(mem.begin() + resourceInfo.NamesOffsetEnd, mem.begin() + resourceInfo.UnknownOffset);

    std::vector<std::byte> unknown(mem.begin() + resourceInfo.UnknownOffset, mem.begin() + resourceInfo.Dummy7Offset);

    long nameIdsOffset = resourceInfo.Dummy7Offset + (resourceInfo.TypeCount * 4);

    std::vector<std::byte> typeIds(mem.begin() + resourceInfo.Dummy7Offset, mem.begin() + nameIdsOffset);

    std::vector<std::byte> nameIds(mem.begin() + nameIdsOffset, mem.begin() + resourceInfo.IdclOffset);

    std::vector<std::byte> idcl(mem.begin() + resourceInfo.IdclOffset, mem.begin() + resourceInfo.DataOffset);

    std::vector<std::byte> data(mem.begin() + resourceInfo.DataOffset, mem.begin() + fileSize);

    int infoOldLength = info.size();
    int nameIdsOldLength = nameIds.size();
    int newChunksCount = 0;

    for (Mod &mod : resourceInfo.ModList) {
        if (mod.IsAssetsInfoJson && mod.AssetsInfo.has_value() && !mod.AssetsInfo.value().Assets.empty()) {
            for (auto &newMod : resourceInfo.ModListNew) {
                for (auto &assetsInfoAssets : mod.AssetsInfo.value().Assets) {
                    if (RemoveWhitespace(assetsInfoAssets.Path).empty())
                        continue;

                    if (assetsInfoAssets.Path == newMod.Name) {
                        newMod.ResourceType = assetsInfoAssets.ResourceType;
                        newMod.Version = (unsigned short)assetsInfoAssets.Version;
                        newMod.StreamDbHash = assetsInfoAssets.StreamDbHash;
                        newMod.SpecialByte1 = assetsInfoAssets.SpecialByte1;
                        newMod.SpecialByte2 = assetsInfoAssets.SpecialByte2;
                        newMod.SpecialByte3 = assetsInfoAssets.SpecialByte3;

                        std::cout << "\tSet resources type " << newMod.ResourceType << " (version: " << newMod.Version.value()
                            << ", streamdb hash: " << newMod.StreamDbHash.value() << ") for new file: " << newMod.Name << std::endl;
                        break;
                    }
                }
            }
        }
    }

    for (Mod &mod : resourceInfo.ModListNew) {
        if (resourceInfo.ContainsResourceWithName(mod.Name)) {
            if (Verbose) {
                std::cerr << RED << "WARNING: " << RESET << "Trying to add resource " << mod.Name
                    << " that has already been added to " << resourceInfo.Name << ", skipping" << std::endl;
            }

            continue;
        }

        if (mod.IsAssetsInfoJson || mod.IsBlangJson)
            continue;

        ResourceDataEntry resourceData;
        std::map<unsigned long, ResourceDataEntry>::iterator x = ResourceDataMap.find(CalculateResourceFileNameHash(mod.Name));

        if (x != ResourceDataMap.end()) {
            resourceData = x->second;

            mod.ResourceType = mod.ResourceType.empty() ? resourceData.ResourceType : mod.ResourceType;
            mod.Version = !mod.Version.has_value() ? (unsigned short)resourceData.Version : mod.Version;
            mod.StreamDbHash = !mod.StreamDbHash.has_value() ? resourceData.StreamDbHash : mod.StreamDbHash;
            mod.SpecialByte1 = !mod.SpecialByte1.has_value() ? resourceData.SpecialByte1 : mod.SpecialByte1;
            mod.SpecialByte2 = !mod.SpecialByte2.has_value() ? resourceData.SpecialByte2 : mod.SpecialByte2;
            mod.SpecialByte3 = !mod.SpecialByte3.has_value() ? resourceData.SpecialByte3 : mod.SpecialByte3;
        }

        if (mod.ResourceType.empty() && !mod.Version.has_value() && !mod.StreamDbHash.has_value()) {
            mod.ResourceType = "rs_streamfile";
            mod.Version = 0;
            mod.StreamDbHash = 0;
            mod.SpecialByte1 = (std::byte)0;
            mod.SpecialByte2 = (std::byte)0;
            mod.SpecialByte3 = (std::byte)0;

            if (Verbose) {
                std::cerr << RED << "WARNING: " << RESET << "No resource data found for file: " << mod.Name << std::endl;
            }
        }

        if (!mod.ResourceType.empty()) {
            bool found = false;

            for (auto &name : resourceInfo.NamesList) {
                if (name.NormalizedFileName == mod.ResourceType) {
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

                names.resize(names.size() + mod.ResourceType.size() + 1);
                std::copy((std::byte*)mod.ResourceType.c_str(), (std::byte*)mod.ResourceType.c_str() + mod.ResourceType.size() + 1, names.begin() + typeLastNameOffset);

                long typeNewCount;
                std::copy(nameOffsets.begin(), nameOffsets.begin() + 8, (std::byte*)&typeNewCount);
                typeNewCount += 1;

                std::copy((std::byte*)&typeNewCount, (std::byte*)&typeNewCount + 8, nameOffsets.begin());
                nameOffsets.resize(nameOffsets.size() + 8);
                std::copy((std::byte*)&typeLastNameOffset, (std::byte*)&typeLastNameOffset + 8, nameOffsets.end() - 8);

                ResourceName newResourceName(mod.ResourceType, mod.ResourceType);
                resourceInfo.NamesList.push_back(newResourceName);

                std::cout << "\tAdded resource type name " << mod.ResourceType << " to " << resourceInfo.Name << std::endl;
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

        std::byte *nameChars = (std::byte*)mod.Name.c_str();
        names.resize(names.size() + mod.Name.size() + 1);
        std::copy(nameChars, nameChars + mod.Name.size() + 1, names.begin() + lastNameOffset);

        long newCount;
        std::copy(nameOffsets.begin(), nameOffsets.begin() + 8, (std::byte*)&newCount);
        newCount += 1;

        std::copy((std::byte*)&newCount, (std::byte*)&newCount + 8, nameOffsets.begin());
        nameOffsets.resize(nameOffsets.size() + 8);

        std::copy((std::byte*)&lastNameOffset, (std::byte*)&lastNameOffset + 8, nameOffsets.end() - 8);

        ResourceName newResourceName(mod.Name, mod.Name);
        resourceInfo.NamesList.push_back(newResourceName);

        long placement = 0x10 - (data.size() % 0x10) + 0x30;
        data.resize(data.size() + placement);

        long fileOffset = data.size() + resourceInfo.DataOffset;
        data.resize(data.size() + mod.FileBytes.size());
        std::copy(mod.FileBytes.begin(), mod.FileBytes.end(), data.end() - mod.FileBytes.size());

        long nameId = resourceInfo.GetResourceNameId(mod.Name);
        nameIds.resize(nameIds.size() + 8);
        long nameIdOffset = (nameIds.size() / 8) - 1;
        nameIds.resize(nameIds.size() + 8);

        long assetTypeNameId = resourceInfo.GetResourceNameId(mod.ResourceType);

        if (assetTypeNameId == -1)
            assetTypeNameId = 0;

        std::copy((std::byte*)&assetTypeNameId, (std::byte*)&assetTypeNameId + 8, nameIds.end() - 16);
        std::copy((std::byte*)&nameId, (std::byte*)&nameId + 8, nameIds.end() - 8);

        std::byte lastInfo[0x90];
        std::copy(info.end() - 0x90, info.end(), lastInfo);
        info.resize(info.size() + 0x90);
        std::copy(lastInfo, lastInfo + 0x90, info.end() - 0x90);

        std::copy((std::byte*)&nameIdOffset, (std::byte*)&nameIdOffset + 8, info.end() - 0x70);
        std::copy((std::byte*)&fileOffset, (std::byte*)&fileOffset + 8, info.end() - 0x58);

        long fileBytesSize = mod.FileBytes.size();

        std::copy((std::byte*)&fileBytesSize, (std::byte*)&fileBytesSize + 8, info.end() - 0x50);
        std::copy((std::byte*)&fileBytesSize, (std::byte*)&fileBytesSize + 8, info.end() - 0x48);

        std::copy((std::byte*)&mod.StreamDbHash.value(), (std::byte*)&mod.StreamDbHash.value() + 8, info.end() - 0x40);
        std::copy((std::byte*)&mod.StreamDbHash.value(), (std::byte*)&mod.StreamDbHash.value() + 8, info.end() - 0x30);

        int version = mod.Version.value();
        std::copy((std::byte*)&version, (std::byte*)&version + 4, info.end() - 0x28);

        int SpecialByte1Int = (int)mod.SpecialByte1.value();
        int SpecialByte2Int = (int)mod.SpecialByte2.value();
        int SpecialByte3Int = (int)mod.SpecialByte3.value();

        std::copy((std::byte*)&SpecialByte1Int, (std::byte*)&SpecialByte1Int + 4, info.end() - 0x24);
        std::copy((std::byte*)&SpecialByte2Int, (std::byte*)&SpecialByte2Int + 4, info.end() - 0x1E);
        std::copy((std::byte*)&SpecialByte3Int, (std::byte*)&SpecialByte3Int + 4, info.end() - 0x1D);

        info[info.size() - 0x20] = (std::byte)0;

        std::cout << "\tAdded " << mod.Name << std::endl;
        newChunksCount++;
    }

    long namesOffsetAdd = info.size() - infoOldLength;
    long newSize = nameOffsets.size() + names.size();
    long unknownAdd = namesOffsetAdd + (newSize - resourceInfo.StringsSize);
    long typeIdsAdd = unknownAdd;
    long nameIdsAdd = typeIdsAdd;
    long idclAdd = nameIdsAdd + (nameIds.size() - nameIdsOldLength);
    long dataAdd = idclAdd;

    int fileCountAdd = resourceInfo.FileCount + newChunksCount;
    std::copy((std::byte*)&fileCountAdd, (std::byte*)&fileCountAdd + 4, header.begin() + 0x20);

    int fileCount2Add = resourceInfo.FileCount2 + newChunksCount * 2;
    std::copy((std::byte*)&fileCount2Add, (std::byte*)&fileCount2Add + 4, header.begin() + 0x2C);

    std::copy((std::byte*)&newSize, (std::byte*)&newSize + 4, header.begin() + 0x38);

    long nameOffsetAdd = resourceInfo.NamesOffset + namesOffsetAdd;
    std::copy((std::byte*)&nameOffsetAdd, (std::byte*)&nameOffsetAdd + 8, header.begin() + 0x40);

    long unknownOffsetAdd = resourceInfo.UnknownOffset + unknownAdd;
    std::copy((std::byte*)&unknownOffsetAdd, (std::byte*)&unknownOffsetAdd + 8, header.begin() + 0x48);

    long unknownOffsetAdd2 = resourceInfo.UnknownOffset2 + unknownAdd;
    std::copy((std::byte*)&unknownOffsetAdd2, (std::byte*)&unknownOffsetAdd2 + 8, header.begin() + 0x58);

    long dummy7OffsetAdd = resourceInfo.Dummy7Offset + typeIdsAdd;
    std::copy((std::byte*)&dummy7OffsetAdd, (std::byte*)&dummy7OffsetAdd + 8, header.begin() + 0x60);

    long dataOffsetAdd = resourceInfo.DataOffset + dataAdd;
    std::copy((std::byte*)&dataOffsetAdd, (std::byte*)&dataOffsetAdd + 8, header.begin() + 0x68);

    long idclOffsetAdd = resourceInfo.IdclOffset + idclAdd;
    std::copy((std::byte*)&idclOffsetAdd, (std::byte*)&idclOffsetAdd + 8, header.begin() + 0x74);

    info.reserve(2 * (0x38 + info.size() + 8));

    for (int i = 0; i < info.size() / 0x90; i++) {
        int fileOffset = 0x38 + (i * 0x90);

        long newOffsetPlusDataAdd;
        std::copy(info.begin() + fileOffset, info.begin() + fileOffset + 8, (std::byte*)&newOffsetPlusDataAdd);
        newOffsetPlusDataAdd += dataAdd;

        std::copy((std::byte*)&newOffsetPlusDataAdd, (std::byte*)&newOffsetPlusDataAdd + 8, info.begin() + fileOffset);
    }

    long newContainerLength = header.size() + info.size() + nameOffsets.size() + names.size() + unknown.size() + typeIds.size() + nameIds.size() + idcl.size() + data.size();

    mem.munmap_file();
    std::filesystem::resize_file(resourceInfo.Path, newContainerLength);

    try {
        mem.mmap_file(resourceInfo.Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, newContainerLength,
            mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);
                
        if (mem.empty())
            throw;
        }
    catch (...) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to load " << resourceInfo.Path << " into memory for writing"<< std::endl;
        return;
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

    std::copy(data.begin(), data.end(), mem.begin() + pos);

    if (newChunksCount != 0)
        std::cout << "Number of files added: " << GREEN << newChunksCount << " file(s) " << RESET << "in " << YELLOW << resourceInfo.Path << RESET << "." << std::endl;
}