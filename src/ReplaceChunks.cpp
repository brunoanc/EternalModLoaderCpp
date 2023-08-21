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
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <mutex>
#include "BlangDecrypt.hpp"
#include "BlangFileEntry.hpp"
#include "Colors.hpp"
#include "GetObject.hpp"
#include "MapResourcesFile.hpp"
#include "Oodle.hpp"
#include "PackageMapSpecInfo.hpp"
#include "PathToResource.hpp"
#include "ProgramOptions.hpp"
#include "SetModDataForChunk.hpp"
#include "Utils.hpp"
#include "ReplaceChunks.hpp"
#include "jsonxx/jsonxx.h"

namespace fs = std::filesystem;

extern std::mutex mtx;

void ReplaceChunks(MemoryMappedFile& memoryMappedFile, ResourceContainer& resourceContainer,
    std::map<uint64_t, ResourceDataEntry>& resourceDataMap, std::stringstream& os,
    std::unique_ptr<std::byte[]>& buffer, int bufferSize)
{
    // For map resources modifications
    ResourceChunk *mapResourcesChunk = nullptr;
    std::unique_ptr<MapResourcesFile> mapResourcesFile;
    std::vector<std::byte> originalDecompressedMapResources;
    bool invalidMapResources = false;
    size_t fileCount = 0;

    // For .blang file modifications
    std::map<std::string, BlangFileEntry> blangFileEntries;

    // Sort mod file list by priority
    std::stable_sort(resourceContainer.ModFileList.begin(), resourceContainer.ModFileList.end(),
        [](const ResourceModFile& resource1, const ResourceModFile& resource2) { return resource1.Parent.LoadPriority > resource2.Parent.LoadPriority; });

    // Load mod files now
    for (auto& modFile : resourceContainer.ModFileList) {
        ResourceChunk *chunk = nullptr;

        // Handle AssetsInfo JSON files
        if (modFile.IsAssetsInfoJson && modFile.AssetsInfo.has_value()) {
            // Add the extra resources to packagemapspec.json if specified
            if (!modFile.AssetsInfo->Resources.empty()) {
                mtx.lock();

                // Deserialize the packagemapspec JSON if it hasn't been deserialized yet
                if (PackageMapSpecInfo::PackageMapSpec == nullptr && !PackageMapSpecInfo::InvalidPackageMapSpec) {
                    if (!PackageMapSpecInfo::ReadPackageMapSpec(ProgramOptions::BasePath)) {
                        os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to parse " << PackageMapSpecInfo::PackageMapSpecPath << " - malformed JSON?\n";
                    }
                }

                // Add the extra resources, then rewrite the JSON
                if (PackageMapSpecInfo::PackageMapSpec != nullptr && !PackageMapSpecInfo::InvalidPackageMapSpec) {
                    for (auto& extraResource : modFile.AssetsInfo->Resources) {
                        // Add the extra resources before all the original resources the level loads
                        // Find the necessary map and file indexes
                        int fileIndex = -1;
                        int mapIndex = -1;

                        // Get the map name of the resource container
                        std::string modFileMapName = fs::path(modFile.Name).stem().string();

                        // Check if there is a resource with the map's name, if so, build the map name using the path of the resource container
                        if (!PathToResourceContainer(modFileMapName + ".resources").empty()) {
                            const std::string baseFolder = std::string(2, SEPARATOR).insert(1, "base"); // "/base/"
                            modFileMapName = fs::path(resourceContainer.Path.substr(resourceContainer.Path.find(baseFolder) + 6)).parent_path().make_preferred().append(modFileMapName).string();
                        }

                        for (int i = 0; i < static_cast<int>(PackageMapSpecInfo::PackageMapSpec->Files.size()); i++) {
                            if (PackageMapSpecInfo::PackageMapSpec->Files[i].Name.find(extraResource.Name) != std::string::npos) {
                                fileIndex = i;
                                break;
                            }
                        }

                        for (int i = 0; i < static_cast<int>(PackageMapSpecInfo::PackageMapSpec->Maps.size()); i++) {
                            if (PackageMapSpecInfo::PackageMapSpec->Maps[i].Name == modFileMapName) {
                                mapIndex = i;
                                break;
                            }
                        }

                        if (fileIndex == -1) {
                            os << Colors::Red << "ERROR: " << Colors::Reset << "Invalid extra resource " << extraResource.Name << ", skipping" << '\n';
                            continue;
                        }

                        if (mapIndex == -1) {
                            os << Colors::Red << "ERROR: " << Colors::Reset << "Map reference not found for " << modFile.Name << ", skipping" << '\n';
                            continue;
                        }

                        // Remove the extra resource, if specified
                        if (extraResource.Remove) {
                            bool mapFileRefRemoved = false;

                            // Find the map file reference to remove
                            for (auto i = static_cast<ssize_t>(PackageMapSpecInfo::PackageMapSpec->MapFileRefs.size()) - 1; i >= 0; i--) {
                                if (PackageMapSpecInfo::PackageMapSpec->MapFileRefs[i].File == fileIndex && PackageMapSpecInfo::PackageMapSpec->MapFileRefs[i].Map == mapIndex) {
                                    PackageMapSpecInfo::PackageMapSpec->MapFileRefs.erase(PackageMapSpecInfo::PackageMapSpec->MapFileRefs.begin() + i);
                                    mapFileRefRemoved = true;
                                    break;
                                }
                            }

                            if (mapFileRefRemoved) {
                                os << "\tRemoved resource " << PackageMapSpecInfo::PackageMapSpec->Files[fileIndex].Name << " to be loaded in map "
                                    << PackageMapSpecInfo::PackageMapSpec->Maps[mapIndex].Name << '\n';
                            }
                            else {
                                if (ProgramOptions::Verbose) {
                                    os << Colors::Red << "WARNING: " << "Resource " << extraResource.Name << " for map "
                                        << PackageMapSpecInfo::PackageMapSpec->Maps[mapIndex].Name << " set to be removed was not found" << '\n';
                                }
                            }

                            continue;
                        }

                        // If the resource is already referenced to be loaded in the map, delete it first to allow us to move it wherever we want
                        for (auto i = static_cast<ssize_t>(PackageMapSpecInfo::PackageMapSpec->MapFileRefs.size()) - 1; i >= 0; i--) {
                            if (PackageMapSpecInfo::PackageMapSpec->MapFileRefs[i].File == fileIndex
                                && PackageMapSpecInfo::PackageMapSpec->MapFileRefs[i].Map == mapIndex) {
                                    PackageMapSpecInfo::PackageMapSpec->MapFileRefs.erase(PackageMapSpecInfo::PackageMapSpec->MapFileRefs.begin() + i);

                                    if (ProgramOptions::Verbose) {
                                        os << "\tResource " << PackageMapSpecInfo::PackageMapSpec->Files[fileIndex].Name << " being added to map "
                                            << PackageMapSpecInfo::PackageMapSpec->Maps[mapIndex].Name << " already exists. The load order will be modified as specified." << '\n';
                                    }

                                    break;
                            }
                        }

                        // Add the extra resource now to the map/file references
                        // before the resource that normally appears last in the list for the map
                        int insertIndex = -1;

                        for (int i = 0; i < static_cast<int>(PackageMapSpecInfo::PackageMapSpec->MapFileRefs.size()); i++) {
                            if (PackageMapSpecInfo::PackageMapSpec->MapFileRefs[i].Map == mapIndex) {
                                // If specified, place the resource as the first resource for the map (highest priority)
                                if (extraResource.PlaceFirst) {
                                    insertIndex = i;
                                    break;
                                }

                                insertIndex = i + 1;
                            }
                        }

                        // Place the extra resource before or after another (if specified)
                        if (!extraResource.PlaceByName.empty() && !extraResource.PlaceFirst) {
                            // First check that the placeByName resource actually exists
                            std::string placeBeforeResourcePath = PathToResourceContainer(extraResource.PlaceByName);

                            if (placeBeforeResourcePath.empty()) {
                                os << Colors::Red << "WARNING: " << Colors::Reset << "placeByName resource " << extraResource.PlaceByName
                                    << " not found for extra resource entry " << extraResource.Name << ", using normal placement" << '\n';
                            }
                            else {
                                // Find placement resource index
                                int placeBeforeFileIndex = -1;

                                for (int i = 0; i < static_cast<int>(PackageMapSpecInfo::PackageMapSpec->Files.size()); i++) {
                                    if (PackageMapSpecInfo::PackageMapSpec->Files[i].Name.find(extraResource.PlaceByName) != std::string::npos) {
                                        placeBeforeFileIndex = i;
                                        break;
                                    }
                                }

                                // Find placement resource map-file reference
                                for (int i = 0; i < static_cast<int>(PackageMapSpecInfo::PackageMapSpec->MapFileRefs.size()); i++) {
                                    if (PackageMapSpecInfo::PackageMapSpec->MapFileRefs[i].Map == mapIndex && PackageMapSpecInfo::PackageMapSpec->MapFileRefs[i].File == placeBeforeFileIndex) {
                                        insertIndex = i + (!extraResource.PlaceBefore ? 1 : 0);
                                        break;
                                    }
                                }
                            }
                        }

                        // Create the map-file reference and add it in the proper position
                        PackageMapSpecMapFileRef mapFileRef(fileIndex, mapIndex);

                        if (insertIndex == -1 || insertIndex >= static_cast<int>(PackageMapSpecInfo::PackageMapSpec->MapFileRefs.size())) {
                            PackageMapSpecInfo::PackageMapSpec->MapFileRefs.push_back(mapFileRef);
                        }
                        else {
                            PackageMapSpecInfo::PackageMapSpec->MapFileRefs.insert(PackageMapSpecInfo::PackageMapSpec->MapFileRefs.begin() + insertIndex, mapFileRef);
                        }

                        os << "\tAdded extra resource " << PackageMapSpecInfo::PackageMapSpec->Files[fileIndex].Name << " to be loaded in map "
                            << PackageMapSpecInfo::PackageMapSpec->Maps[mapIndex].Name;

                        if (extraResource.PlaceFirst) {
                            os << " with the highest priority" << '\n';
                        }
                        else if (!extraResource.PlaceByName.empty() && insertIndex != -1) {
                            os << " " << (extraResource.PlaceBefore ? "before" : "after") << " " << extraResource.PlaceByName << '\n';
                        }
                        else {
                            os << " with the lowest priority" << '\n';
                        }

                        PackageMapSpecInfo::WasPackageMapSpecModified = true;
                    }
                }

                mtx.unlock();
            }

            if (modFile.AssetsInfo->Assets.empty() && modFile.AssetsInfo->Maps.empty() && modFile.AssetsInfo->Layers.empty()) {
                continue;
            }

            // Add new assets to .mapresources
            // First, find, read and deserialize the .mapresources file in this container
            // If this is a "gameresources" container, only search for "common.mapresources"
            if (mapResourcesFile == nullptr && !invalidMapResources) {
                for (auto& file : resourceContainer.ChunkList) {
                    if (EndsWith(file.ResourceName.NormalizedFileName, ".mapresources")) {
                        if (StartsWith(resourceContainer.Name, "gameresources") && EndsWith(file.ResourceName.NormalizedFileName, "init.mapresources")) {
                            continue;
                        }

                        mapResourcesChunk = &file;

                        // Read the mapresources file data (it should be compressed)
                        std::vector<std::byte> mapResourcesBytes(mapResourcesChunk->SizeZ);
                        uint64_t mapResourcesFileOffset;

                        std::copy(memoryMappedFile.Mem + mapResourcesChunk->FileOffset,
                            memoryMappedFile.Mem + mapResourcesChunk->FileOffset + 8, reinterpret_cast<std::byte*>(&mapResourcesFileOffset));
                        std::copy(memoryMappedFile.Mem + mapResourcesFileOffset, memoryMappedFile.Mem + mapResourcesFileOffset + mapResourcesBytes.size(), mapResourcesBytes.begin());

                        // Decompress the data
                        try {
                            originalDecompressedMapResources = Oodle::Decompress(mapResourcesBytes, mapResourcesChunk->Size);

                            if (originalDecompressedMapResources.empty()) {
                                throw std::exception();
                            }

                            // Deserialize the decompressed data
                            mapResourcesFile = std::make_unique<MapResourcesFile>(originalDecompressedMapResources);
                        }
                        catch (...) {
                            invalidMapResources = true;
                            os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to decompress " << mapResourcesChunk->ResourceName.NormalizedFileName
                                << " - are you trying to add assets in the wrong .resources archive?" << '\n';
                            break;
                        }
                    }
                }
            }

            // Don't add anything if the file was not found or couldn't be decompressed
            if (mapResourcesFile == nullptr || invalidMapResources) {
                modFile.FileBytes.resize(0);
                continue;
            }

            // Add layers
            if (!modFile.AssetsInfo->Layers.empty()) {
                for (auto& newLayers : modFile.AssetsInfo->Layers) {
                    if (std::find(mapResourcesFile->Layers.begin(), mapResourcesFile->Layers.end(), newLayers.Name) != mapResourcesFile->Layers.end()) {
                        if (ProgramOptions::Verbose) {
                            os << Colors::Red << "ERROR: " << Colors::Reset << "Trying to add layer " << newLayers.Name << " that has already been added in "
                                << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << '\n';
                        }

                        continue;
                    }

                    mapResourcesFile->Layers.push_back(newLayers.Name);
                    os << "\tAdded layer " << newLayers.Name << " to " << mapResourcesChunk->ResourceName.NormalizedFileName
                        << " in " << resourceContainer.Name << "" << '\n';
                }
            }

            // Add maps
            if (!modFile.AssetsInfo->Maps.empty()) {
                for (auto& newMaps : modFile.AssetsInfo->Maps) {
                    if (std::find(mapResourcesFile->Maps.begin(), mapResourcesFile->Maps.end(), newMaps.Name) != mapResourcesFile->Maps.end()) {
                        if (ProgramOptions::Verbose) {
                            os << Colors::Red << "ERROR: " << Colors::Reset << "Trying to add map " << newMaps.Name <<" that has already been added in "
                                << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << '\n';
                        }

                        continue;
                    }

                    mapResourcesFile->Maps.push_back(newMaps.Name);
                    os << "Added map " << newMaps.Name << " to " << mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << '\n';
                }
            }

            // Add assets
            if (!modFile.AssetsInfo->Assets.empty()) {
                for (auto& newAsset : modFile.AssetsInfo->Assets) {
                    if (RemoveWhitespace(newAsset.Name).empty() || RemoveWhitespace(newAsset.MapResourceType).empty()) {
                        if (ProgramOptions::Verbose) {
                            os << "WARNING: " << "Skipping empty resource declaration in " << modFile.Name << '\n';
                        }

                        continue;
                    }

                    // Remove the asset if specified
                    if (newAsset.Remove) {
                        auto x = std::find(mapResourcesFile->AssetTypes.begin(), mapResourcesFile->AssetTypes.end(), newAsset.MapResourceType);

                        if (x == mapResourcesFile->AssetTypes.end()) {
                            if (ProgramOptions::Verbose) {
                                os << Colors::Red << "WARNING: " << Colors::Reset << "Can't remove asset " << newAsset.Name << " with type " << newAsset.MapResourceType <<
                                    " because it doesn't exist in " << mapResourcesChunk->ResourceName.NormalizedFileName << '\n';
                                continue;
                            }
                        }

                        int newAssetTypeIndex = std::distance(mapResourcesFile->AssetTypes.begin(), x);
                        bool assetFound = false;

                        for (size_t i = 0; i < mapResourcesFile->Assets.size(); i++) {
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
                            os << Colors::Red << "WARNING: " << Colors::Reset << "Can't remove asset " << newAsset.Name << " with type " << newAsset.MapResourceType <<
                                " because it doesn't exist in " << mapResourcesChunk->ResourceName.NormalizedFileName << '\n';
                        }

                        continue;
                    }

                    bool alreadyExists = false;

                    for (auto& existingAsset : mapResourcesFile->Assets) {
                        if (existingAsset.Name == newAsset.Name
                            && mapResourcesFile->AssetTypes[existingAsset.AssetTypeIndex] == newAsset.MapResourceType) {
                                alreadyExists = true;
                                break;
                        }
                    }

                    if (alreadyExists) {
                        if (ProgramOptions::Verbose) {
                            os << Colors::Red << "WARNING: " << Colors::Reset << "Failed to add asset " << newAsset.Name <<
                                " that has already been added in " << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << '\n';
                        }

                        continue;
                    }

                    // Find the asset type index
                    int assetTypeIndex = -1;

                    for (int i = 0; i < static_cast<int>(mapResourcesFile->AssetTypes.size()); i++) {
                        if (mapResourcesFile->AssetTypes[i] == newAsset.MapResourceType) {
                            assetTypeIndex = i;
                            break;
                        }
                    }

                    // If not found, add the asset type at the end
                    if (assetTypeIndex == -1) {
                        mapResourcesFile->AssetTypes.push_back(newAsset.MapResourceType);
                        assetTypeIndex = mapResourcesFile->AssetTypes.size() - 1;

                        os << "Added asset type " << newAsset.MapResourceType << " to " <<
                            mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << '\n';
                    }

                     // Determine where to place this new asset in map resources
                    MapAsset placeByExistingAsset;
                    bool found = false;
                    size_t assetPosition = mapResourcesFile->Assets.size();

                    if (!newAsset.PlaceByName.empty()) {
                        if (!newAsset.PlaceByType.empty()) {
                            auto x = std::find(mapResourcesFile->AssetTypes.begin(), mapResourcesFile->AssetTypes.end(), newAsset.PlaceByType);

                            if (x != mapResourcesFile->AssetTypes.end()) {
                                int placeByTypeIndex = std::distance(mapResourcesFile->AssetTypes.begin(), x);

                                for (auto& asset : mapResourcesFile->Assets) {
                                    if (asset.Name == newAsset.PlaceByName && asset.AssetTypeIndex == placeByTypeIndex) {
                                        placeByExistingAsset = asset;
                                        found = true;
                                        break;
                                    }
                                }
                            }
                        }
                        else {
                            for (auto& asset : mapResourcesFile->Assets) {
                                if (asset.Name == newAsset.PlaceByName) {
                                    placeByExistingAsset = asset;
                                    found = true;
                                    break;
                                }
                            }
                        }

                        if (found) {
                            auto x = std::find(mapResourcesFile->Assets.begin(), mapResourcesFile->Assets.end(), placeByExistingAsset);

                            if (x != mapResourcesFile->Assets.end()) {
                                assetPosition = std::distance(mapResourcesFile->Assets.begin(), x);

                                if (!newAsset.PlaceBefore) {
                                    assetPosition++;
                                }
                            }
                        }
                    }

                    if (ProgramOptions::Verbose && found) {
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
            // Handle custom .blang JSON files
            modFile.Name = modFile.Name.substr(modFile.Name.find('/') + 1);
            modFile.Name = fs::path(modFile.Name).replace_extension(".blang").string();

            chunk = GetChunk(modFile.Name, resourceContainer);

            if (chunk == nullptr) {
                modFile.FileBytes.resize(0);
                continue;
            }
        }
        else {
            chunk = GetChunk(modFile.Name, resourceContainer);

            if (chunk == nullptr) {
                // This is a new mod, add it to the new mods list
                resourceContainer.NewModFileList.push_back(modFile);

                // Get the data to add to mapresources from the resource data file, if available
                auto x = resourceDataMap.find(CalculateResourceFileNameHash(modFile.Name));

                if (x == resourceDataMap.end()) {
                    continue;
                }

                ResourceDataEntry resourceData = x->second;

                if (resourceData.MapResourceName.empty()) {
                    if (RemoveWhitespace(resourceData.MapResourceType).empty()) {
                        if (ProgramOptions::Verbose) {
                            os << "WARNING: " << "Mapresources data for asset " << modFile.Name << " is null, skipping" << '\n';
                        }

                        continue;
                    }
                    else {
                        resourceData.MapResourceName = modFile.Name;
                    }
                }

                // First, find, read and deserialize the .mapresources file in this container
                // If this is a "gameresources" container, only search for "common.mapresources"
                if (mapResourcesFile == nullptr && !invalidMapResources) {
                    for (auto& file : resourceContainer.ChunkList) {
                        if (EndsWith(file.ResourceName.NormalizedFileName, ".mapresources")) {
                            if (StartsWith(resourceContainer.Name, "gameresources") && EndsWith(file.ResourceName.NormalizedFileName, "init.mapresources")) {
                                continue;
                            }

                            mapResourcesChunk = &file;

                            // Read the mapresources file data (it should be compressed)
                            std::vector<std::byte> mapResourcesBytes(mapResourcesChunk->SizeZ);
                            uint64_t mapResourcesFileOffset;

                            std::copy(memoryMappedFile.Mem + mapResourcesChunk->FileOffset, memoryMappedFile.Mem + mapResourcesChunk->FileOffset + 8, reinterpret_cast<std::byte*>(&mapResourcesFileOffset));
                            std::copy(memoryMappedFile.Mem + mapResourcesFileOffset, memoryMappedFile.Mem + mapResourcesFileOffset + mapResourcesBytes.size(), mapResourcesBytes.begin());

                            // Decompress the data
                            try {
                                originalDecompressedMapResources = Oodle::Decompress(mapResourcesBytes, mapResourcesChunk->Size);

                                if (originalDecompressedMapResources.empty()) {
                                    throw std::exception();
                                }

                                // Deserialize the decompressed data
                                mapResourcesFile = std::make_unique<MapResourcesFile>(originalDecompressedMapResources);
                            }
                            catch (...) {
                                invalidMapResources = true;
                                os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to decompress " << mapResourcesChunk->ResourceName.NormalizedFileName
                                    << " - are you trying to add assets in the wrong .resources archive?" << '\n';
                                break;
                            }
                        }
                    }
                }

                // Don't add anything if the file was not found or couldn't be decompressed
                if (mapResourcesFile == nullptr || invalidMapResources) {
                    continue;
                }

                // Add the asset info
                bool alreadyExists = false;

                for (auto& existingAsset : mapResourcesFile->Assets) {
                    if (existingAsset.Name == resourceData.MapResourceName
                        && mapResourcesFile->AssetTypes[existingAsset.AssetTypeIndex] == resourceData.MapResourceType) {
                            alreadyExists = true;
                            break;
                    }
                }

                if (alreadyExists) {
                    if (ProgramOptions::Verbose) {
                        os << Colors::Red << "WARNING: " << Colors::Reset << "Trying to add asset " << resourceData.MapResourceName
                            << " that has already been added in " << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << '\n';
                    }

                    continue;
                }

                // Find the asset type index
                int assetTypeIndex = -1;

                for (int i = 0; i < static_cast<int>(mapResourcesFile->AssetTypes.size()); i++) {
                    if (mapResourcesFile->AssetTypes[i] == resourceData.MapResourceType) {
                        assetTypeIndex = i;
                        break;
                    }
                }

                // If not found, add the asset type at the end
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

        // Parse blang JSON files
        if (modFile.IsBlangJson) {
            // Read the .blang file in the container if it hasn't been read yet
            std::string blangFilePath = "strings/" + fs::path(modFile.Name).filename().string();
            BlangFileEntry blangFileEntry;
            auto x = blangFileEntries.find(blangFilePath);
            bool exists = x != blangFileEntries.end();

            if (!exists) {
                uint64_t fileOffset, size;
                std::copy(memoryMappedFile.Mem + chunk->FileOffset, memoryMappedFile.Mem + chunk->FileOffset + 8, reinterpret_cast<std::byte*>(&fileOffset));
                std::copy(memoryMappedFile.Mem + chunk->FileOffset + 8, memoryMappedFile.Mem + chunk->FileOffset + 16, reinterpret_cast<std::byte*>(&size));

                std::vector<std::byte> blangFileBytes(memoryMappedFile.Mem + fileOffset, memoryMappedFile.Mem + fileOffset + size);
                std::vector<std::byte> decryptedBlangFileBytes = IdCrypt(blangFileBytes, modFile.Name, true);

                if (decryptedBlangFileBytes.empty()) {
                    os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to decrypt " << resourceContainer.Name << "/" << modFile.Name << '\n';
                    continue;
                }

                try {
                    blangFileEntry = BlangFileEntry(BlangFile(decryptedBlangFileBytes), *chunk);
                    blangFileEntries[blangFilePath] = blangFileEntry;
                }
                catch (...) {
                    os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to parse " << resourceContainer.Name << "/" << modFile.Name << '\n';
                    continue;
                }
            }

            if (!blangFileEntry.Announce && modFile.Announce) {
                blangFileEntry.Announce = true;
            }

            // Read the blang JSON and add the strings to the .blang file
            jsonxx::Object blangJson;
            jsonxx::Array blangJsonStrings;

            try {
                std::string blangJsonString(reinterpret_cast<char*>(modFile.FileBytes.data()), modFile.FileBytes.size());
                blangJson.parse(blangJsonString);
                blangJsonStrings = blangJson.get<jsonxx::Array>("strings");
            }
            catch (...) {
                os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to parse EternalMod/strings/" << fs::path(modFile.Name).replace_extension(".json").string() << '\n';
                continue;
            }

            for (size_t i = 0; i < blangJsonStrings.size(); i++) {
                jsonxx::Object blangJsonString = blangJsonStrings.get<jsonxx::Object>(i);
                bool stringFound = false;

                for (auto& blangString : blangFileEntries[blangFilePath].BlangFile.Strings) {
                    if (blangJsonString.get<jsonxx::String>("name") == blangString.Identifier) {
                        stringFound = true;
                        blangString.Text = blangJsonString.get<jsonxx::String>("text");

                        if (modFile.Announce) {
                            os << "\tReplaced " << blangString.Identifier << " in " << modFile.Name << '\n';
                        }

                        blangFileEntries[blangFilePath].WasModified = true;
                        break;
                    }
                }

                if (stringFound) {
                    continue;
                }

                BlangString newBlangString;
                newBlangString.Identifier = blangJsonString.get<jsonxx::String>("name");
                newBlangString.Text = blangJsonString.get<jsonxx::String>("text");
                blangFileEntries[blangFilePath].BlangFile.Strings.push_back(newBlangString);

                if (modFile.Announce) {
                    os << "\tAdded " << blangJsonString.get<jsonxx::String>("name") << " in " << modFile.Name << '\n';
                }

                blangFileEntries[blangFilePath].WasModified = true;
            }

            continue;
        }

        // Replace the mod file data now
        uint64_t compressedSize = modFile.FileBytes.size();
        uint64_t uncompressedSize = compressedSize;
        std::byte compressionMode{0};

        // If this is a texture, check if it's compressed, or compress it if necessary
        if ((EndsWith(chunk->ResourceName.NormalizedFileName, ".tga") || EndsWith(chunk->ResourceName.NormalizedFileName, ".png")) && compressedSize != 0) {
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

        if (!SetModDataForChunk(memoryMappedFile, resourceContainer, *chunk, modFile,
        compressedSize, uncompressedSize, &compressionMode, buffer, bufferSize)) {
            os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to set new mod data for " << modFile.Name << " in resource chunk." << '\n';
            continue;
        }

        if (modFile.Announce) {
            os << "\tReplaced " << modFile.Name << '\n';
        }

        fileCount++;
    }

     // Modify the necessary .blang files
    for (auto& blangFileEntry : blangFileEntries) {
        if (!blangFileEntry.second.WasModified) {
            continue;
        }

        std::vector<std::byte> cryptData;

        try {
            std::vector<std::byte> blangFileBytes = blangFileEntry.second.BlangFile.ToByteVector();
            cryptData = IdCrypt(blangFileBytes, blangFileEntry.first, false);

            if (cryptData.empty()) {
                throw std::exception();
            }
        }
        catch (...) {
            os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to encrypt " << blangFileEntry.first << '\n';
            continue;
        }

        ResourceModFile blangModFile(Mod(), blangFileEntry.first, resourceContainer.Name);
        blangModFile.FileBytes = cryptData;
        std::byte compressionMode{0};

        if (!SetModDataForChunk(memoryMappedFile, resourceContainer, blangFileEntry.second.Chunk, blangModFile,
        blangModFile.FileBytes.size(), blangModFile.FileBytes.size(), &compressionMode, buffer, bufferSize)) {
            os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to set new mod data for " << blangFileEntry.first << "in resource chunk." << '\n';
            continue;
        }

        if (blangFileEntry.second.Announce) {
            os << "\tModified " << blangFileEntry.first << '\n';
        }

        fileCount++;
    }

    // Modify the map resources file if needed
    if (mapResourcesFile != nullptr && mapResourcesChunk != nullptr && !originalDecompressedMapResources.empty()) {
        // Serialize the map resources data
        std::vector<std::byte> decompressedMapResourcesData = mapResourcesFile->ToByteVector();

        // Only modify the .mapresources file if it has changed
        if (decompressedMapResourcesData != originalDecompressedMapResources) {
            // Compress the data
            std::vector<std::byte> compressedMapResourcesData = Oodle::Compress(decompressedMapResourcesData);

            if (compressedMapResourcesData.empty()) {
                os << "ERROR: " << Colors::Reset << "Failed to compress " << mapResourcesChunk->ResourceName.NormalizedFileName << '\n';
            }
            else {
                ResourceModFile mapResourcesModFile(Mod(), mapResourcesChunk->ResourceName.NormalizedFileName, resourceContainer.Name);
                mapResourcesModFile.FileBytes = compressedMapResourcesData;

                if (!SetModDataForChunk(memoryMappedFile, resourceContainer, *mapResourcesChunk,  mapResourcesModFile, compressedMapResourcesData.size(), decompressedMapResourcesData.size(), nullptr, buffer, bufferSize)) {
                    os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to set new mod data for " << mapResourcesChunk->ResourceName.NormalizedFileName << "in resource chunk." << '\n';
                    return;
                }

                os << "\tModified " << mapResourcesChunk->ResourceName.NormalizedFileName << '\n';
                fileCount++;
            }
        }
    }

    if (fileCount > 0) {
        os << "Number of files replaced: " << Colors::Green << fileCount << " file(s) " << Colors::Reset << "in " << Colors::Yellow << resourceContainer.Path << Colors::Reset << "." << '\n';
    }

    if (ProgramOptions::SlowMode) {
        os.flush();
    }
}
