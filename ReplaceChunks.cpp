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
#include <cstring>

#include "jsonxx/jsonxx.h"
#include "EternalModLoader.hpp"

const std::byte *DivinityMagic = (std::byte*)"DIVINITY";
const std::string PackageMapSpecJsonFileName = "packagemapspec.json";
class PackageMapSpecInfo PackageMapSpecInfo;

/**
 * @brief Replace chunks in the given resource file
 * 
 * @param memoryMappedFile MemoryMappedFile object containing the resource to modify
 * @param resourceContainer ResourceContainer object containing the resources's data
 * @param os StringStream to output to
 */
void ReplaceChunks(MemoryMappedFile &memoryMappedFile, ResourceContainer &resourceContainer, std::stringstream &os)
{
    ResourceChunk *mapResourcesChunk = nullptr;
    MapResourcesFile *mapResourcesFile = nullptr;
    std::vector<std::byte> originalDecompressedMapResources;
    bool invalidMapResources = false;
    int32_t fileCount = 0;
    std::map<std::string, BlangFileEntry> blangFileEntries;

    std::stable_sort(resourceContainer.ModFileList.begin(), resourceContainer.ModFileList.end(),
        [](const ResourceModFile &resource1, const ResourceModFile &resource2) { return resource1.Parent.LoadPriority > resource2.Parent.LoadPriority; });

    for (auto &modFile : resourceContainer.ModFileList) {
        ResourceChunk *chunk = nullptr;

        if (modFile.IsAssetsInfoJson && modFile.AssetsInfo.has_value()) {

            if (!modFile.AssetsInfo->Resources.empty()) {
                mtx.lock();

                if (PackageMapSpecInfo.PackageMapSpec == nullptr && !PackageMapSpecInfo.invalidPackageMapSpec) {
                    PackageMapSpecInfo.PackageMapSpecPath = BasePath + PackageMapSpecJsonFileName;
                    FILE *packageMapSpecFile = fopen(PackageMapSpecInfo.PackageMapSpecPath.c_str(), "rb");

                    if (!packageMapSpecFile) {
                        os << RED << "ERROR: " << RESET << PackageMapSpecInfo.PackageMapSpecPath << " not found while trying to add extra resources for level "
                            << resourceContainer.Name << '\n';
                        PackageMapSpecInfo.invalidPackageMapSpec = true;
                    }
                    else {
                        int64_t filesize = std::filesystem::file_size(PackageMapSpecInfo.PackageMapSpecPath);
                        std::vector<std::byte> packageMapSpecBytes(filesize);

                        if (fread(packageMapSpecBytes.data(), 1, filesize, packageMapSpecFile) != filesize) {
                            os << RED << "ERROR: " << RESET << "Failed to read data from " << PackageMapSpecInfo.PackageMapSpecPath
                                << " while trying to add extra resources for level " << resourceContainer.Name << '\n';
                            PackageMapSpecInfo.invalidPackageMapSpec = true;
                        }

                        fclose(packageMapSpecFile);

                        try {
                            std::string packageMapSpecJson((char*)packageMapSpecBytes.data(), packageMapSpecBytes.size());
                            PackageMapSpecInfo.PackageMapSpec = new PackageMapSpec(packageMapSpecJson);
                        }
                        catch (...) {
                            os << RED << "ERROR: " << RESET << "Failed to parse " << PackageMapSpecInfo.PackageMapSpecPath << '\n';
                            PackageMapSpecInfo.invalidPackageMapSpec = true;
                        }
                    }
                }

                if (PackageMapSpecInfo.PackageMapSpec != nullptr && !PackageMapSpecInfo.invalidPackageMapSpec) {
                    for (auto &extraResource : modFile.AssetsInfo->Resources) {
                        std::string extraResourcePath = PathToResourceContainer(extraResource.Name);

                        if (extraResourcePath.empty()) {
                            os << RED << "WARNING: " << RESET << "Trying to add non-existing extra resource " << extraResource.Name
                                << " to " << resourceContainer.Name << ", skipping" << '\n';
                            continue;
                        }

                        int32_t fileIndex = -1;
                        int32_t mapIndex = -1;

                        for (int32_t i = 0; i < PackageMapSpecInfo.PackageMapSpec->Files.size(); i++) {
                            if (PackageMapSpecInfo.PackageMapSpec->Files[i].Name.find(extraResource.Name) != std::string::npos) {
                                fileIndex = i;
                                break;
                            }
                        }

                        std::string modFileMapName = std::filesystem::path(modFile.Name).stem().string();

                        if (StartsWith(resourceContainer.Name, "dlc_hub")) {
                            modFileMapName = "game/dlc/hub/hub";
                        }
                        else if (StartsWith(resourceContainer.Name, "hub")) {
                            modFileMapName = "game/hub/hub";
                        }

                        for (int32_t i = 0; i < PackageMapSpecInfo.PackageMapSpec->Maps.size(); i++) {
                            if (EndsWith(PackageMapSpecInfo.PackageMapSpec->Maps[i].Name, modFileMapName)) {
                                mapIndex = i;
                                break;
                            }
                        }

                        if (fileIndex == -1) {
                            os << RED << "ERROR: " << RESET << "Invalid extra resource " << extraResource.Name << ", skipping" << '\n';
                            continue;
                        }

                        if (mapIndex == -1) {
                            os << RED << "ERROR: " << RESET << "Map reference not found for " << modFile.Name << ", skipping" << '\n';
                            continue;
                        }

                        if (extraResource.Remove) {
                            bool mapFileRefRemoved = false;

                            for (int32_t i = PackageMapSpecInfo.PackageMapSpec->MapFileRefs.size() - 1; i >= 0; i--) {
                                if (PackageMapSpecInfo.PackageMapSpec->MapFileRefs[i].File == fileIndex && PackageMapSpecInfo.PackageMapSpec->MapFileRefs[i].Map == mapIndex) {
                                    PackageMapSpecInfo.PackageMapSpec->MapFileRefs.erase(PackageMapSpecInfo.PackageMapSpec->MapFileRefs.begin() + i);
                                    mapFileRefRemoved = true;
                                    break;
                                }
                            }

                            if (mapFileRefRemoved) {
                                os << "\tRemoved resource " << PackageMapSpecInfo.PackageMapSpec->Files[fileIndex].Name << " to be loaded in map "
                                    << PackageMapSpecInfo.PackageMapSpec->Maps[mapIndex].Name << '\n';
                            }
                            else {
                                if (Verbose) {
                                    os << RED << "WARNING: " << "Resource " << extraResource.Name << " for map "
                                        << PackageMapSpecInfo.PackageMapSpec->Maps[mapIndex].Name << " set to be removed was not found" << '\n';
                                }
                            }

                            continue;
                        }

                        for (int32_t i = PackageMapSpecInfo.PackageMapSpec->MapFileRefs.size() - 1; i >= 0; i--) {
                            if (PackageMapSpecInfo.PackageMapSpec->MapFileRefs[i].File == fileIndex
                                && PackageMapSpecInfo.PackageMapSpec->MapFileRefs[i].Map == mapIndex) {
                                    PackageMapSpecInfo.PackageMapSpec->MapFileRefs.erase(PackageMapSpecInfo.PackageMapSpec->MapFileRefs.begin() + i);

                                    if (Verbose) {
                                        os << "\tResource " << PackageMapSpecInfo.PackageMapSpec->Files[fileIndex].Name << " being added to map "
                                            << PackageMapSpecInfo.PackageMapSpec->Maps[mapIndex].Name << " already exists. The load order will be modified as specified." << '\n';
                                    }

                                    break;
                            }
                        }

                        int32_t insertIndex = -1;

                        for (int32_t i = 0; i < PackageMapSpecInfo.PackageMapSpec->MapFileRefs.size(); i++) {
                            if (PackageMapSpecInfo.PackageMapSpec->MapFileRefs[i].Map == mapIndex) {
                                if (extraResource.PlaceFirst) {
                                    insertIndex = i;
                                    break;
                                }

                                insertIndex = i + 1;
                            }
                        }

                        if (!extraResource.PlaceByName.empty() && !extraResource.PlaceFirst) {
                            std::string placeBeforeResourcePath = PathToResourceContainer(extraResource.PlaceByName);

                            if (placeBeforeResourcePath.empty()) {
                                os << RED << "WARNING: " << RESET << "placeByName resource " << extraResource.PlaceByName
                                    << " not found for extra resource entry " << extraResource.Name << ", using normal placement" << '\n';
                            }
                            else {
                                int32_t placeBeforeFileIndex = -1;

                                for (int32_t i = 0; i < PackageMapSpecInfo.PackageMapSpec->Files.size(); i++) {
                                    if (PackageMapSpecInfo.PackageMapSpec->Files[i].Name.find(extraResource.PlaceByName) != std::string::npos) {
                                        placeBeforeFileIndex = i;
                                        break;
                                    }
                                }

                                for (int32_t i = 0; i < PackageMapSpecInfo.PackageMapSpec->MapFileRefs.size(); i++) {
                                    if (PackageMapSpecInfo.PackageMapSpec->MapFileRefs[i].Map == mapIndex && PackageMapSpecInfo.PackageMapSpec->MapFileRefs[i].File == placeBeforeFileIndex) {
                                        insertIndex = i + (!extraResource.PlaceBefore ? 1 : 0);
                                        break;
                                    }
                                }
                            }
                        }

                        PackageMapSpecMapFileRef mapFileRef(fileIndex, mapIndex);

                        if (insertIndex == -1 || insertIndex >= PackageMapSpecInfo.PackageMapSpec->MapFileRefs.size()) {
                            PackageMapSpecInfo.PackageMapSpec->MapFileRefs.push_back(mapFileRef);
                        }
                        else {
                            PackageMapSpecInfo.PackageMapSpec->MapFileRefs.insert(PackageMapSpecInfo.PackageMapSpec->MapFileRefs.begin() + insertIndex, mapFileRef);
                        }

                        os << "\tAdded extra resource " << PackageMapSpecInfo.PackageMapSpec->Files[fileIndex].Name << " to be loaded in map "
                            << PackageMapSpecInfo.PackageMapSpec->Maps[mapIndex].Name;

                        if (extraResource.PlaceFirst) {
                            os << " with the highest priority" << '\n';
                        }
                        else if (!extraResource.PlaceByName.empty() && insertIndex != -1) {
                            os << " " << (extraResource.PlaceBefore ? "before" : "after") << " " << extraResource.PlaceByName << '\n';
                        }
                        else {
                            os << " with the lowest priority" << '\n';
                        }

                        PackageMapSpecInfo.WasPackageMapSpecModified = true;
                    }
                }

                mtx.unlock();
            }

            if (modFile.AssetsInfo->Assets.empty() && modFile.AssetsInfo->Maps.empty() && modFile.AssetsInfo->Layers.empty())
                continue;

            if (mapResourcesFile == nullptr && !invalidMapResources) {
                for (auto &file : resourceContainer.ChunkList) {
                    if (EndsWith(file.ResourceName.NormalizedFileName, ".mapresources")) {
                        if (StartsWith(resourceContainer.Name, "gameresources") && EndsWith(file.ResourceName.NormalizedFileName, "init.mapresources"))
                            continue;

                        mapResourcesChunk = &file;

                        std::vector<std::byte> mapResourcesBytes(mapResourcesChunk->SizeZ);
                        uint64_t mapResourcesFileOffset;

                        std::copy(memoryMappedFile.Mem + mapResourcesChunk->FileOffset, memoryMappedFile.Mem + mapResourcesChunk->FileOffset + 8, (std::byte*)&mapResourcesFileOffset);
                        std::copy(memoryMappedFile.Mem + mapResourcesFileOffset, memoryMappedFile.Mem + mapResourcesFileOffset + mapResourcesBytes.size(), mapResourcesBytes.begin());

                        try {
                            originalDecompressedMapResources = OodleDecompress(mapResourcesBytes, mapResourcesChunk->Size);

                            if (originalDecompressedMapResources.empty())
                                throw std::exception();

                            mapResourcesFile = new MapResourcesFile(originalDecompressedMapResources);
                        }
                        catch (...) {
                            invalidMapResources = true;
                            os << RED << "ERROR: " << RESET << "Failed to decompress " << mapResourcesChunk->ResourceName.NormalizedFileName
                                << " - are you trying to add assets in the wrong .resources archive?" << '\n';
                            break;
                        }
                    }
                }
            }

            if (mapResourcesFile == nullptr || invalidMapResources) {
                modFile.FileBytes.resize(0);
                continue;
            }

            if (!modFile.AssetsInfo->Layers.empty()) {
                for (auto &newLayers : modFile.AssetsInfo->Layers) {
                    if (std::find(mapResourcesFile->Layers.begin(), mapResourcesFile->Layers.end(), newLayers.Name) != mapResourcesFile->Layers.end()) {
                        if (Verbose) {
                            os << RED << "ERROR: " << RESET << "Trying to add layer " << newLayers.Name << " that has already been added in "
                                << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << '\n';
                        }

                        continue;
                    }

                    mapResourcesFile->Layers.push_back(newLayers.Name);
                    os << "\tAdded layer " << newLayers.Name << " to " << mapResourcesChunk->ResourceName.NormalizedFileName
                        << " in " << resourceContainer.Name << "" << '\n';
                }
            }

            if (!modFile.AssetsInfo->Maps.empty()) {
                for (auto &newMaps : modFile.AssetsInfo->Maps) {
                    if (std::find(mapResourcesFile->Maps.begin(), mapResourcesFile->Maps.end(), newMaps.Name) != mapResourcesFile->Maps.end()) {
                        if (Verbose) {
                            os << RED << "ERROR: " << RESET << "Trying to add map " << newMaps.Name <<" that has already been added in "
                                << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << '\n';
                        }

                        continue;
                    }

                    mapResourcesFile->Maps.push_back(newMaps.Name);
                    os << "Added map " << newMaps.Name << " to " << mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << '\n';
                }
            }

            if (!modFile.AssetsInfo->Assets.empty()) {
                for (auto &newAsset : modFile.AssetsInfo->Assets) {
                    if (RemoveWhitespace(newAsset.Name).empty() || RemoveWhitespace(newAsset.MapResourceType).empty()) {
                        if (Verbose)
                            os << "WARNING: " << "Skipping empty resource declaration in " << modFile.Name << '\n';

                        continue;
                    }

                    if (newAsset.Remove) {
                        std::vector<std::string>::iterator x = std::find(mapResourcesFile->AssetTypes.begin(), mapResourcesFile->AssetTypes.end(), newAsset.MapResourceType);

                        if (x == mapResourcesFile->AssetTypes.end()) {
                            if (Verbose) {
                                os << RED << "WARNING: " << RESET << "Can't remove asset " << newAsset.Name << " with type " << newAsset.MapResourceType <<
                                    " because it doesn't exist in " << mapResourcesChunk->ResourceName.NormalizedFileName << '\n';
                                continue;
                            }
                        }

                        int32_t newAssetTypeIndex = std::distance(mapResourcesFile->AssetTypes.begin(), x);
                        bool assetFound = false;

                        for (int32_t i = 0; i < mapResourcesFile->Assets.size(); i++) {
                            if (mapResourcesFile->Assets[i].Name == newAsset.Name
                                && mapResourcesFile->Assets[i].AssetTypeIndex == newAssetTypeIndex) {
                                    assetFound = true;
                                    mapResourcesFile->Assets.erase(mapResourcesFile->Assets.begin() + i);
                                    break;
                            }
                        }

                        if (assetFound) {
                            os << "\tRemoved asset " << newAsset.Name << " with type " << newAsset.MapResourceType <<
                                " from " << mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << '\n';
                        }
                        else {
                            os << RED << "WARNING: " << RESET << "Can't remove asset " << newAsset.Name << " with type " << newAsset.MapResourceType <<
                                " because it doesn't exist in " << mapResourcesChunk->ResourceName.NormalizedFileName << '\n';
                        }

                        continue;
                    }

                    bool alreadyExists = false;

                    for (auto &existingAsset : mapResourcesFile->Assets) {
                        if (existingAsset.Name == newAsset.Name
                            && mapResourcesFile->AssetTypes[existingAsset.AssetTypeIndex] == newAsset.MapResourceType) {
                                alreadyExists = true;
                                break;
                        }
                    }

                    if (alreadyExists) {
                        if (Verbose) {
                            os << RED << "WARNING: " << RESET << "Failed to add asset " << newAsset.Name <<
                                " that has already been added in " << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << '\n';
                        }

                        continue;
                    }

                    int32_t assetTypeIndex = -1;

                    for (int32_t i = 0; i < mapResourcesFile->AssetTypes.size(); i++) {
                        if (mapResourcesFile->AssetTypes[i] == newAsset.MapResourceType) {
                            assetTypeIndex = i;
                            break;
                        }
                    }

                    if (assetTypeIndex == -1) {
                        mapResourcesFile->AssetTypes.push_back(newAsset.MapResourceType);
                        assetTypeIndex = mapResourcesFile->AssetTypes.size() - 1;

                        os << "Added asset type " << newAsset.MapResourceType << " to " <<
                            mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << '\n';
                    }

                    MapAsset placeByExistingAsset;
                    bool found = false;
                    int32_t assetPosition = mapResourcesFile->Assets.size();

                    if (!newAsset.PlaceByName.empty()) {
                        if (!newAsset.PlaceByType.empty()) {
                            std::vector<std::string>::iterator x = std::find(mapResourcesFile->AssetTypes.begin(), mapResourcesFile->AssetTypes.end(), newAsset.PlaceByType);

                            if (x != mapResourcesFile->AssetTypes.end()) {
                                int32_t placeByTypeIndex = std::distance(mapResourcesFile->AssetTypes.begin(), x);

                                for (auto &asset : mapResourcesFile->Assets) {
                                    if (asset.Name == newAsset.PlaceByName && asset.AssetTypeIndex == placeByTypeIndex) {
                                        placeByExistingAsset = asset;
                                        found = true;
                                        break;
                                    }
                                }
                            }
                        }
                        else {
                            for (auto &asset : mapResourcesFile->Assets) {
                                if (asset.Name == newAsset.PlaceByName) {
                                    placeByExistingAsset = asset;
                                    found = true;
                                    break;
                                }
                            }
                        }

                        if (found) {
                            std::vector<MapAsset>::iterator x = std::find(mapResourcesFile->Assets.begin(), mapResourcesFile->Assets.end(), placeByExistingAsset);

                            if (x != mapResourcesFile->Assets.end()) {
                                assetPosition = std::distance(mapResourcesFile->Assets.begin(), x);

                                if (!newAsset.PlaceBefore)
                                    assetPosition++;
                            }
                        }
                    }

                    if (Verbose && found) {
                        os << "\tAsset " << newAsset.Name << " with type " << newAsset.MapResourceType << " will be added before asset " << placeByExistingAsset.Name << " with type "
                            << mapResourcesFile->AssetTypes[placeByExistingAsset.AssetTypeIndex] << " to " << mapResourcesChunk->ResourceName.NormalizedFileName << " in "<< resourceContainer.Name << '\n';
                    }

                    MapAsset newMapAsset;
                    newMapAsset.AssetTypeIndex = assetTypeIndex;
                    newMapAsset.Name = newAsset.Name;
                    newMapAsset.UnknownData4 = 128;

                    mapResourcesFile->Assets.insert(mapResourcesFile->Assets.begin() + assetPosition, newMapAsset);

                    os << "\tAdded asset " << newAsset.Name << " with type " << newAsset.MapResourceType << " to " << mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << '\n';
                }
            }

            modFile.FileBytes.resize(0);
            continue;
        }
        else if (modFile.IsBlangJson) {
            modFile.Name = modFile.Name.substr(modFile.Name.find('/') + 1);
            modFile.Name = std::filesystem::path(modFile.Name).replace_extension(".blang").string();

            chunk = GetChunk(modFile.Name, resourceContainer);

            if (chunk == nullptr) {
                modFile.FileBytes.resize(0);
                continue;
            }
        }
        else {
            chunk = GetChunk(modFile.Name, resourceContainer);

            if (chunk == nullptr) {
                resourceContainer.NewModFileList.push_back(modFile);

                std::map<uint64_t, ResourceDataEntry>::iterator x = ResourceDataMap.find(CalculateResourceFileNameHash(modFile.Name));

                if (x == ResourceDataMap.end())
                    continue;

                ResourceDataEntry resourceData = x->second;

                if (resourceData.MapResourceName.empty()) {
                    if (RemoveWhitespace(resourceData.MapResourceType).empty()) {
                        if (Verbose)
                            os << "WARNING: " << "Mapresources data for asset " << modFile.Name << " is null, skipping" << '\n';

                        continue;
                    }
                    else {
                        resourceData.MapResourceName = modFile.Name;
                    }
                }

                if (mapResourcesFile == nullptr && !invalidMapResources) {
                    for (auto &file : resourceContainer.ChunkList) {
                        if (EndsWith(file.ResourceName.NormalizedFileName, ".mapresources")) {
                            if (StartsWith(resourceContainer.Name, "gameresources") && EndsWith(file.ResourceName.NormalizedFileName, "init.mapresources"))
                                continue;

                            mapResourcesChunk = &file;

                            std::vector<std::byte> mapResourcesBytes(mapResourcesChunk->SizeZ);
                            uint64_t mapResourcesFileOffset;

                            std::copy(memoryMappedFile.Mem + mapResourcesChunk->FileOffset, memoryMappedFile.Mem + mapResourcesChunk->FileOffset + 8, (std::byte*)&mapResourcesFileOffset);
                            std::copy(memoryMappedFile.Mem + mapResourcesFileOffset, memoryMappedFile.Mem + mapResourcesFileOffset + mapResourcesBytes.size(), mapResourcesBytes.begin());

                            try {
                                originalDecompressedMapResources = OodleDecompress(mapResourcesBytes, mapResourcesChunk->Size);

                                if (originalDecompressedMapResources.empty())
                                    throw std::exception();

                                mapResourcesFile = new MapResourcesFile(originalDecompressedMapResources);
                            }
                            catch (...) {
                                invalidMapResources = true;
                                os << RED << "ERROR: " << RESET << "Failed to decompress " << mapResourcesChunk->ResourceName.NormalizedFileName
                                    << " - are you trying to add assets in the wrong .resources archive?" << '\n';
                                break;
                            }
                        }
                    }
                }

                if (mapResourcesFile == nullptr || invalidMapResources)
                    continue;

                bool alreadyExists = false;

                for (auto &existingAsset : mapResourcesFile->Assets) {
                    if (existingAsset.Name == resourceData.MapResourceName
                        && mapResourcesFile->AssetTypes[existingAsset.AssetTypeIndex] == resourceData.MapResourceType) {
                            alreadyExists = true;
                            break;
                    }
                }

                if (alreadyExists) {
                    if (Verbose)
                        os << RED << "WARNING: " << RESET << "Trying to add asset " << resourceData.MapResourceName
                            << " that has already been added in " << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << '\n';

                    continue;
                }

                int32_t assetTypeIndex = -1;

                for (int32_t i = 0; i < mapResourcesFile->AssetTypes.size(); i++) {
                    if (mapResourcesFile->AssetTypes[i] == resourceData.MapResourceType) {
                        assetTypeIndex = i;
                        break;
                    }
                }

                if (assetTypeIndex == -1) {
                    mapResourcesFile->AssetTypes.push_back(resourceData.MapResourceType);
                    assetTypeIndex = mapResourcesFile->AssetTypes.size() - 1;

                    os << "\tAdded asset type " << resourceData.MapResourceType << " to "
                        << mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << '\n';
                }

                MapAsset newMapAsset;
                newMapAsset.AssetTypeIndex = assetTypeIndex;
                newMapAsset.Name = resourceData.MapResourceName;
                newMapAsset.UnknownData4 = 128;
                mapResourcesFile->Assets.push_back(newMapAsset);

                os << "\tAdded asset " << resourceData.MapResourceName << " with type " << resourceData.MapResourceType
                    << " to " << mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << '\n';
                continue;
            }
        }

        if (modFile.IsBlangJson) {
            std::string blangFilePath = "strings/" + std::filesystem::path(modFile.Name).filename().string();
            BlangFileEntry blangFileEntry;
            std::map<std::string, BlangFileEntry>::iterator x = blangFileEntries.find(blangFilePath);
            bool exists = x != blangFileEntries.end();

            if (!exists) {
                int64_t fileOffset, size;
                std::copy(memoryMappedFile.Mem + chunk->FileOffset, memoryMappedFile.Mem + chunk->FileOffset + 8, (std::byte*)&fileOffset);
                std::copy(memoryMappedFile.Mem + chunk->FileOffset + 8, memoryMappedFile.Mem + chunk->FileOffset + 16, (std::byte*)&size);

                std::vector<std::byte> blangFileBytes(memoryMappedFile.Mem + fileOffset, memoryMappedFile.Mem + fileOffset + size);
                std::vector<std::byte> decryptedBlangFileBytes = IdCrypt(blangFileBytes, modFile.Name, true);

                if (decryptedBlangFileBytes.empty()) {
                    os << RED << "ERROR: " << RESET << "Failed to decrypt " << resourceContainer.Name << "/" << modFile.Name << '\n';
                    continue;
                }

                try {
                    blangFileEntry = BlangFileEntry(BlangFile(decryptedBlangFileBytes), *chunk);
                    blangFileEntries[blangFilePath] = blangFileEntry;
                }
                catch (...) {
                    os << RED << "ERROR: " << RESET << "Failed to parse " << resourceContainer.Name << "/" << modFile.Name << '\n';
                    continue;
                }
            }

            jsonxx::Object blangJson;

            try {
                std::string blangJsonString((char*)modFile.FileBytes.data(), modFile.FileBytes.size());
                blangJson.parse(blangJsonString);
            }
            catch (...) {
                os << RED << "ERROR: " << RESET << "Failed to parse EternalMod/strings/" << std::filesystem::path(modFile.Name).replace_extension(".json").string() << '\n';
                continue;
            }

            jsonxx::Array blangJsonStrings = blangJson.get<jsonxx::Array>("strings");

            for (int32_t i = 0; i < blangJsonStrings.size(); i++) {
                jsonxx::Object blangJsonString = blangJsonStrings.get<jsonxx::Object>(i);
                bool stringFound = false;

                for (auto &blangString : blangFileEntries[blangFilePath].BlangFile.Strings) {
                    if (blangJsonString.get<jsonxx::String>("name") == blangString.Identifier) {
                        stringFound = true;
                        blangString.Text = blangJsonString.get<jsonxx::String>("text");

                        os << "\tReplaced " << blangString.Identifier << " in " << modFile.Name << '\n';
                        blangFileEntries[blangFilePath].WasModified = true;
                        break;
                    }
                }

                if (stringFound)
                    continue;

                BlangString newBlangString;
                newBlangString.Identifier = blangJsonString.get<jsonxx::String>("name");
                newBlangString.Text = blangJsonString.get<jsonxx::String>("text");
                blangFileEntries[blangFilePath].BlangFile.Strings.push_back(newBlangString);

                os << "\tAdded " << blangJsonString.get<jsonxx::String>("name") << " in " << modFile.Name << '\n';
                blangFileEntries[blangFilePath].WasModified = true;
            }

            continue;
        }

        uint64_t compressedSize = modFile.FileBytes.size();
        uint64_t uncompressedSize = compressedSize;
        std::byte compressionMode = (std::byte)0;

        if (EndsWith(chunk->ResourceName.NormalizedFileName, ".tga")) {
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

        if (!SetModDataForChunk(memoryMappedFile, resourceContainer, *chunk, modFile, compressedSize, uncompressedSize, &compressionMode)) {
            os << RED << "ERROR: " << RESET << "Failed to set new mod data for " << modFile.Name << " in resource chunk." << '\n';
            continue;
        }

        os << "\tReplaced " << modFile.Name << '\n';
        fileCount++;
    }

    for (auto &blangFileEntry : blangFileEntries) {
        if (!blangFileEntry.second.WasModified)
            continue;

        std::vector<std::byte> cryptData;

        try {
            std::vector<std::byte> blangFileBytes = blangFileEntry.second.BlangFile.ToByteVector();
            cryptData = IdCrypt(blangFileBytes, blangFileEntry.first, false);

            if (cryptData.empty())
                throw std::exception();
        }
        catch (...) {
            os << RED << "ERROR: " << RESET << "Failed to encrypt " << blangFileEntry.first << '\n';
            continue;
        }

        ResourceModFile blangModFile(Mod(), blangFileEntry.first, resourceContainer.Name);
        blangModFile.FileBytes = cryptData;
        std::byte compressionMode = (std::byte)0;

        if (!SetModDataForChunk(memoryMappedFile, resourceContainer, blangFileEntry.second.Chunk, blangModFile, blangModFile.FileBytes.size(), blangModFile.FileBytes.size(), &compressionMode)) {
            os << RED << "ERROR: " << RESET << "Failed to set new mod data for " << blangFileEntry.first << "in resource chunk." << '\n';
            continue;
        }

        os << "\tModified " << blangFileEntry.first << '\n';
        fileCount++;
    }

    if (mapResourcesFile != nullptr && mapResourcesChunk != nullptr && !originalDecompressedMapResources.empty()) {
        std::vector<std::byte> decompressedMapResourcesData = mapResourcesFile->ToByteVector();

        if (decompressedMapResourcesData != originalDecompressedMapResources) {
            std::vector<std::byte> compressedMapResourcesData = OodleCompress(decompressedMapResourcesData, OodleFormat::Kraken, OodleCompressionLevel::Normal);

            if (compressedMapResourcesData.empty()) {
                os << "ERROR: " << RESET << "Failed to compress " << mapResourcesChunk->ResourceName.NormalizedFileName << '\n';
            }
            else {
                ResourceModFile mapResourcesModFile(Mod(), mapResourcesChunk->ResourceName.NormalizedFileName, resourceContainer.Name);
                mapResourcesModFile.FileBytes = compressedMapResourcesData;

                if (!SetModDataForChunk(memoryMappedFile, resourceContainer, *mapResourcesChunk,  mapResourcesModFile, compressedMapResourcesData.size(), decompressedMapResourcesData.size(), nullptr)) {
                    os << RED << "ERROR: " << RESET << "Failed to set new mod data for " << mapResourcesChunk->ResourceName.NormalizedFileName << "in resource chunk." << '\n';
                    delete mapResourcesFile;
                    return;
                }

                os << "\tModified " << mapResourcesChunk->ResourceName.NormalizedFileName << '\n';
                delete mapResourcesFile;
                fileCount++;
            }
        }
    }
    
    if (fileCount > 0)
        os << "Number of files replaced: " << GREEN << fileCount << " file(s) " << RESET << "in " << YELLOW << resourceContainer.Path << RESET << "." << '\n';

    if (SlowMode)
        os.flush();
}