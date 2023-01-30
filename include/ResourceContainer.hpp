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

#ifndef RESOURCECONTAINER_HPP
#define RESOURCECONTAINER_HPP

#include <string>
#include <vector>
#include <optional>
#include "AssetsInfo.hpp"
#include "Mod.hpp"

class ResourceModFile
{
public:
    Mod Parent;
    std::string Name;
    std::string ResourceName;
    std::vector<std::byte> FileBytes;
    bool IsBlangJson{false};
    bool IsAssetsInfoJson{false};
    std::optional<class AssetsInfo> AssetsInfo{std::nullopt};
    std::optional<uint64_t> StreamDbHash{std::nullopt};
    std::string ResourceType;
    std::optional<unsigned short> Version{std::nullopt};
    bool PlaceBefore{false};
    std::string PlaceByName;
    std::string PlaceByType;
    std::optional<std::byte> SpecialByte1{std::nullopt};
    std::optional<std::byte> SpecialByte2{std::nullopt};
    std::optional<std::byte> SpecialByte3{std::nullopt};
    bool Announce{true};

    ResourceModFile(Mod parent, std::string name, std::string resourceName, bool announce = true)
        : Parent(parent), Name(name), ResourceName(resourceName), Announce(announce) {}
};

class ResourceName
{
public:
    std::string FullFileName;
    std::string NormalizedFileName;

    ResourceName() {}
    ResourceName(std::string fullFileName, std::string normalizedFileName) : FullFileName(fullFileName), NormalizedFileName(normalizedFileName) {}
};

class ResourceChunk
{
public:
    class ResourceName ResourceName;
    uint64_t FileOffset{0};
    uint64_t SizeOffset{0};
    uint64_t SizeZ{0};
    uint64_t Size{0};
    std::byte CompressionMode{0};

    ResourceChunk() {}
    ResourceChunk(class ResourceName name, uint64_t fileOffset) : ResourceName(name), FileOffset(fileOffset) {}

    bool operator==(const ResourceChunk& chunk)
    {
        return ResourceName.FullFileName == chunk.ResourceName.FullFileName && FileOffset == chunk.FileOffset;
    }
};

class ResourceContainer
{
public:
    std::string Name;
    std::string Path;
    int FileCount{0};
    int TypeCount{0};
    int StringsSize{0};
    uint64_t NamesOffset{0};
    uint64_t InfoOffset{0};
    uint64_t Dummy7Offset{0};
    uint64_t DataOffset{0};
    uint64_t IdclOffset{0};
    int UnknownCount{0};
    int FileCount2{0};
    uint64_t NamesOffsetEnd{0};
    uint64_t UnknownOffset{0};
    uint64_t UnknownOffset2{0};
    std::vector<ResourceName> NamesList;
    std::vector<ResourceChunk> ChunkList;
    std::vector<ResourceModFile> ModFileList;
    std::vector<ResourceModFile> NewModFileList;

    ResourceContainer(std::string name, std::string path): Name(name), Path(path) {}

    bool ContainsResourceWithName(std::string name) const
    {
        for (auto& resourceName : NamesList) {
            if (resourceName.FullFileName == name || resourceName.NormalizedFileName == name) {
                return true;
            }
        }

        return false;
    }

    ssize_t GetResourceNameId(std::string name) const
    {
        for (size_t i = 0; i < NamesList.size(); i++) {
            if (NamesList[i].FullFileName == name || NamesList[i].NormalizedFileName == name) {
                return i;
            }
        }

        return -1;
    }
};

#endif
