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

#include "json/json.hpp"
#include "EternalModLoader.hpp"

#ifdef _WIN32
void ReplaceChunks(std::byte *&mem, HANDLE &hFile, HANDLE &fileMapping, ResourceContainer &resourceContainer)
#else
void ReplaceChunks(std::byte *&mem, int32_t &fd, ResourceContainer &resourceContainer)
#endif
{
    ResourceChunk *mapResourcesChunk = NULL;
    MapResourcesFile *mapResourcesFile = NULL;
    std::vector<std::byte> originalDecompressedMapResources;
    bool invalidMapResources = false;
    int32_t fileCount = 0;
    std::map<std::string, BlangFileEntry> blangFileEntries;
    std::string packageMapSpecPath;
    PackageMapSpec *packageMapSpec = NULL;
    bool invalidPackageMapSpec = false;
    bool wasPackageMapSpecModified = false;

    std::stable_sort(resourceContainer.ModFileList.begin(), resourceContainer.ModFileList.end(),
        [](ResourceModFile resource1, ResourceModFile resource2) { return resource1.Parent.LoadPriority > resource2.Parent.LoadPriority ? true : false; });

    for (auto &modFile : resourceContainer.ModFileList) {
        ResourceChunk *chunk = NULL;

        if (modFile.IsAssetsInfoJson && modFile.AssetsInfo.has_value()) {
            if (!modFile.AssetsInfo->Resources.empty()) {
                if (packageMapSpec == NULL && !invalidPackageMapSpec) {
                    packageMapSpecPath = BasePath + PackageMapSpecJsonFileName;
                    FILE *packageMapSpecFile = fopen(packageMapSpecPath.c_str(), "rb");

                    if (!packageMapSpecFile) {
                        std::cerr << RED << "ERROR: " << RESET << packageMapSpecPath << " not found while trying to add extra resources for level "
                            << resourceContainer.Name << std::endl;
                        invalidPackageMapSpec = true;
                    }
                    else {
                        int64_t filesize = std::filesystem::file_size(packageMapSpecPath);
                        std::vector<std::byte> packageMapSpecBytes(filesize);

                        if (fread(packageMapSpecBytes.data(), 1, filesize, packageMapSpecFile) != filesize) {
                            std::cerr << RED << "ERROR: " << RESET << "Failed to read data from " << packageMapSpecPath
                                << " while trying to add extra resources for level " << resourceContainer.Name << std::endl;
                            invalidPackageMapSpec = true;
                        }

                        fclose(packageMapSpecFile);

                        try {
                            std::string packageMapSpecJson((char*)packageMapSpecBytes.data(), packageMapSpecBytes.size());
                            packageMapSpec = new PackageMapSpec(packageMapSpecJson);
                        }
                        catch (...) {
                            std::cerr << RED << "ERROR: " << RESET << "Failed to parse " << packageMapSpecPath << std::endl;
                            invalidPackageMapSpec = true;
                        }
                    }
                }

                if (packageMapSpec != NULL && !invalidPackageMapSpec) {
                    for (auto &extraResource : modFile.AssetsInfo->Resources) {
                        std::string extraResourcePath = PathToResourceContainer(extraResource.Name);

                        if (extraResourcePath.empty()) {
                            std::cerr << RED << "WARNING: " << RESET << "Trying to add non-existing extra resource " << extraResource.Name
                                << " to " << resourceContainer.Name << ", skipping" << std::endl;
                            continue;
                        }

                        int32_t fileIndex = -1;
                        int32_t mapIndex = -1;

                        for (int32_t i = 0; i < packageMapSpec->Files.size(); i++) {
                            if (packageMapSpec->Files[i].Name.find(extraResource.Name) != std::string::npos) {
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

                        for (int32_t i = 0; i < packageMapSpec->Maps.size(); i++) {
                            if (EndsWith(packageMapSpec->Maps[i].Name, modFileMapName)) {
                                mapIndex = i;
                                break;
                            }
                        }

                        if (fileIndex == -1) {
                            std::cerr << RED << "ERROR: " << RESET << "Invalid extra resource " << extraResource.Name << ", skipping" << std::endl;
                            continue;
                        }

                        if (mapIndex == -1) {
                            std::cerr << RED << "ERROR: " << RESET << "Map reference not found for " << modFile.Name << ", skipping" << std::endl;
                            continue;
                        }

                        if (extraResource.Remove) {
                            bool mapFileRefRemoved = false;

                            for (int32_t i = packageMapSpec->MapFileRefs.size() - 1; i >= 0; i--) {
                                if (packageMapSpec->MapFileRefs[i].File == fileIndex && packageMapSpec->MapFileRefs[i].Map == mapIndex) {
                                    packageMapSpec->MapFileRefs.erase(packageMapSpec->MapFileRefs.begin() + i);
                                    mapFileRefRemoved = true;
                                    break;
                                }
                            }

                            if (mapFileRefRemoved) {
                                std::cout << "\tRemoved resource " << packageMapSpec->Files[fileIndex].Name << " to be loaded in map "
                                    << packageMapSpec->Maps[mapIndex].Name << std::endl;
                            }
                            else {
                                if (Verbose) {
                                    std::cerr << RED << "WARNING: " << "Resource " << extraResource.Name << " for map "
                                        << packageMapSpec->Maps[mapIndex].Name << " set to be removed was not found" << std::endl;
                                }
                            }

                            continue;
                        }

                        for (int32_t i = packageMapSpec->MapFileRefs.size() - 1; i >= 0; i--) {
                            if (packageMapSpec->MapFileRefs[i].File == fileIndex
                                && packageMapSpec->MapFileRefs[i].Map == mapIndex) {
                                    packageMapSpec->MapFileRefs.erase(packageMapSpec->MapFileRefs.begin() + i);

                                    if (Verbose) {
                                        std::cout << "\tResource " << packageMapSpec->Files[fileIndex].Name << " being added to map "
                                            << packageMapSpec->Maps[mapIndex].Name << " already exists. The load order will be modified as specified." << std::endl;
                                    }

                                    break;
                            }
                        }

                        int32_t insertIndex = -1;

                        for (int32_t i = 0; i < packageMapSpec->MapFileRefs.size(); i++) {
                            if (packageMapSpec->MapFileRefs[i].Map == mapIndex) {
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
                                std::cerr << RED << "WARNING: " << RESET << "placeByName resource " << extraResource.PlaceByName
                                    << " not found for extra resource entry " << extraResource.Name << ", using normal placement" << std::endl;
                            }
                            else {
                                int32_t placeBeforeFileIndex = -1;

                                for (int32_t i = 0; i < packageMapSpec->Files.size(); i++) {
                                    if (packageMapSpec->Files[i].Name.find(extraResource.PlaceByName) != std::string::npos) {
                                        placeBeforeFileIndex = i;
                                        break;
                                    }
                                }

                                for (int32_t i = 0; i < packageMapSpec->MapFileRefs.size(); i++) {
                                    if (packageMapSpec->MapFileRefs[i].Map == mapIndex && packageMapSpec->MapFileRefs[i].File == placeBeforeFileIndex) {
                                        insertIndex = i + (!extraResource.PlaceBefore ? 1 : 0);
                                        break;
                                    }
                                }
                            }
                        }

                        PackageMapSpecMapFileRef mapFileRef(fileIndex, mapIndex);

                        if (insertIndex == -1 || insertIndex >= packageMapSpec->MapFileRefs.size()) {
                            packageMapSpec->MapFileRefs.push_back(mapFileRef);
                        }
                        else {
                            packageMapSpec->MapFileRefs.insert(packageMapSpec->MapFileRefs.begin() + insertIndex, mapFileRef);
                        }

                        std::cout << "\tAdded extra resource " << packageMapSpec->Files[fileIndex].Name << " to be loaded in map "
                            << packageMapSpec->Maps[mapIndex].Name;

                        if (extraResource.PlaceFirst) {
                            std::cout << " with the highest priority" << std::endl;
                        }
                        else if (!extraResource.PlaceByName.empty() && insertIndex != -1) {
                            std::cout << " " << (extraResource.PlaceBefore ? "before" : "after") << " " << extraResource.PlaceByName << std::endl;
                        }
                        else {
                            std::cout << " with the lowest priority" << std::endl;
                        }

                        wasPackageMapSpecModified = true;
                    }
                }
            }

            if (modFile.AssetsInfo->Assets.empty() && modFile.AssetsInfo->Maps.empty() && modFile.AssetsInfo->Layers.empty())
                continue;

            if (mapResourcesFile == NULL && !invalidMapResources) {
                for (auto &file : resourceContainer.ChunkList) {
                    if (EndsWith(file.ResourceName.NormalizedFileName, ".mapresources")) {
                        if (StartsWith(resourceContainer.Name, "gameresources") && EndsWith(file.ResourceName.NormalizedFileName, "init.mapresources"))
                            continue;

                        mapResourcesChunk = &file;

                        std::vector<std::byte> mapResourcesBytes(mapResourcesChunk->SizeZ);
                        uint64_t mapResourcesFileOffset;

                        std::copy(mem + mapResourcesChunk->FileOffset, mem + mapResourcesChunk->FileOffset + 8, (std::byte*)&mapResourcesFileOffset);
                        std::copy(mem + mapResourcesFileOffset, mem + mapResourcesFileOffset + mapResourcesBytes.size(), mapResourcesBytes.begin());

                        try {
                            originalDecompressedMapResources = OodleDecompress(mapResourcesBytes, mapResourcesChunk->Size);

                            if (originalDecompressedMapResources.empty())
                                throw std::exception();

                            mapResourcesFile = new MapResourcesFile(originalDecompressedMapResources);
                        }
                        catch (...) {
                            invalidMapResources = true;
                            std::cerr << RED << "ERROR: " << RESET << "Failed to decompress " << mapResourcesChunk->ResourceName.NormalizedFileName
                                << " - are you trying to add assets in the wrong .resources archive?" << std::endl;
                            break;
                        }
                    }
                }
            }

            if (mapResourcesFile == NULL || invalidMapResources) {
                modFile.FileBytes.resize(0);
                continue;
            }

            if (!modFile.AssetsInfo->Layers.empty()) {
                for (auto &newLayers : modFile.AssetsInfo->Layers) {
                    if (std::find(mapResourcesFile->Layers.begin(), mapResourcesFile->Layers.end(), newLayers.Name) != mapResourcesFile->Layers.end()) {
                        if (Verbose) {
                            std::cerr << RED << "ERROR: " << RESET << "Trying to add layer " << newLayers.Name << " that has already been added in "
                                << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << std::endl;
                        }

                        continue;
                    }

                    mapResourcesFile->Layers.push_back(newLayers.Name);
                    std::cout << "\tAdded layer " << newLayers.Name << " to " << mapResourcesChunk->ResourceName.NormalizedFileName
                        << " in " << resourceContainer.Name << "" << std::endl;
                }
            }

            if (!modFile.AssetsInfo->Maps.empty()) {
                for (auto &newMaps : modFile.AssetsInfo->Maps) {
                    if (std::find(mapResourcesFile->Maps.begin(), mapResourcesFile->Maps.end(), newMaps.Name) != mapResourcesFile->Maps.end()) {
                        if (Verbose) {
                            std::cerr << RED << "ERROR: " << RESET << "Trying to add map " << newMaps.Name <<" that has already been added in "
                                << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << std::endl;
                        }

                        continue;
                    }

                    mapResourcesFile->Maps.push_back(newMaps.Name);
                    std::cout << "Added map " << newMaps.Name << " to " << mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << std::endl;
                }
            }

            if (!modFile.AssetsInfo->Assets.empty()) {
                for (auto &newAsset : modFile.AssetsInfo->Assets) {
                    if (RemoveWhitespace(newAsset.Name).empty() || RemoveWhitespace(newAsset.MapResourceType).empty()) {
                        if (Verbose)
                            std::cerr << "WARNING: " << "Skipping empty resource declaration in " << modFile.Name << std::endl;

                        continue;
                    }

                    if (newAsset.Remove) {
                        std::vector<std::string>::iterator x = std::find(mapResourcesFile->AssetTypes.begin(), mapResourcesFile->AssetTypes.end(), newAsset.MapResourceType);

                        if (x == mapResourcesFile->AssetTypes.end()) {
                            if (Verbose) {
                                std::cerr << RED << "WARNING: " << RESET << "Can't remove asset " << newAsset.Name << " with type " << newAsset.MapResourceType <<
                                    " because it doesn't exist in " << mapResourcesChunk->ResourceName.NormalizedFileName << std::endl;
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
                            std::cout << "\tRemoved asset " << newAsset.Name << " with type " << newAsset.MapResourceType <<
                                " from " << mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << std::endl;
                        }
                        else {
                            std::cerr << RED << "WARNING: " << RESET << "Can't remove asset " << newAsset.Name << " with type " << newAsset.MapResourceType <<
                                " because it doesn't exist in " << mapResourcesChunk->ResourceName.NormalizedFileName << std::endl;
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
                            std::cerr << RED << "WARNING: " << RESET << "Failed to add asset " << newAsset.Name <<
                                " that has already been added in " << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << std::endl;
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

                        std::cout << "Added asset type " << newAsset.MapResourceType << " to " <<
                            mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << std::endl;
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
                        std::cout << "\tAsset " << newAsset.Name << " with type " << newAsset.MapResourceType << " will be added before asset " << placeByExistingAsset.Name << " with type "
                            << mapResourcesFile->AssetTypes[placeByExistingAsset.AssetTypeIndex] << " to " << mapResourcesChunk->ResourceName.NormalizedFileName << " in "<< resourceContainer.Name << std::endl;
                    }

                    MapAsset newMapAsset;
                    newMapAsset.AssetTypeIndex = assetTypeIndex;
                    newMapAsset.Name = newAsset.Name;
                    newMapAsset.UnknownData4 = 128;

                    mapResourcesFile->Assets.insert(mapResourcesFile->Assets.begin() + assetPosition, newMapAsset);

                    std::cout << "\tAdded asset " << newAsset.Name << " with type " << newAsset.MapResourceType << " to " << mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << std::endl;
                }
            }

            modFile.FileBytes.resize(0);
            continue;
        }
        else if (modFile.IsBlangJson) {
            modFile.Name = modFile.Name.substr(modFile.Name.find('/') + 1);
            modFile.Name = std::filesystem::path(modFile.Name).replace_extension(".blang").string();

            chunk = GetChunk(modFile.Name, resourceContainer);

            if (chunk == NULL) {
                modFile.FileBytes.resize(0);
                continue;
            }
        }
        else {
            chunk = GetChunk(modFile.Name, resourceContainer);

            if (chunk == NULL) {
                resourceContainer.NewModFileList.push_back(modFile);

                std::map<uint64_t, ResourceDataEntry>::iterator x = ResourceDataMap.find(CalculateResourceFileNameHash(modFile.Name));

                if (x == ResourceDataMap.end())
                    continue;

                ResourceDataEntry resourceData = x->second;

                if (resourceData.MapResourceName.empty()) {
                    if (RemoveWhitespace(resourceData.MapResourceType).empty()) {
                        if (Verbose)
                            std::cerr << "WARNING: " << "Mapresources data for asset " << modFile.Name << " is null, skipping" << std::endl;

                        continue;
                    }
                    else {
                        resourceData.MapResourceName = modFile.Name;
                    }
                }

                if (mapResourcesFile == NULL && !invalidMapResources) {
                    for (auto &file : resourceContainer.ChunkList) {
                        if (EndsWith(file.ResourceName.NormalizedFileName, ".mapresources")) {
                            if (EndsWith(resourceContainer.Name, "gameresources") && EndsWith(file.ResourceName.NormalizedFileName, "init.mapresources"))
                                continue;

                            mapResourcesChunk = &file;

                            std::vector<std::byte> mapResourcesBytes(mapResourcesChunk->SizeZ);
                            uint64_t mapResourcesFileOffset;

                            std::copy(mem + mapResourcesChunk->FileOffset, mem + mapResourcesChunk->FileOffset + 8, (std::byte*)&mapResourcesFileOffset);
                            std::copy(mem + mapResourcesFileOffset, mem + mapResourcesFileOffset + mapResourcesBytes.size(), mapResourcesBytes.begin());

                            try {
                                originalDecompressedMapResources = OodleDecompress(mapResourcesBytes, mapResourcesChunk->Size);

                                if (originalDecompressedMapResources.empty())
                                    throw std::exception();

                                mapResourcesFile = new MapResourcesFile(originalDecompressedMapResources);
                            }
                            catch (...) {
                                invalidMapResources = true;
                                std::cerr << RED << "ERROR: " << RESET << "Failed to decompress " << mapResourcesChunk->ResourceName.NormalizedFileName
                                    << " - are you trying to add assets in the wrong .resources archive?" << std::endl;
                                break;
                            }
                        }
                    }
                }

                if (mapResourcesFile == NULL || invalidMapResources)
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
                        std::cerr << RED << "WARNING: " << RESET << "Trying to add asset " << resourceData.MapResourceName
                            << " that has already been added in " << mapResourcesChunk->ResourceName.NormalizedFileName << ", skipping" << std::endl;

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
                    std::cout << resourceData.MapResourceType << std::endl;
                    mapResourcesFile->AssetTypes.push_back(resourceData.MapResourceType);
                    assetTypeIndex = mapResourcesFile->AssetTypes.size() - 1;

                    std::cout << "\tAdded asset type " << resourceData.MapResourceType << " to "
                        << mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << std::endl;
                }

                MapAsset newMapAsset;
                newMapAsset.AssetTypeIndex = assetTypeIndex;
                newMapAsset.Name = resourceData.MapResourceName;
                newMapAsset.UnknownData4 = 128;
                mapResourcesFile->Assets.push_back(newMapAsset);

                std::cout << "\tAdded asset " << resourceData.MapResourceName << " with type " << resourceData.MapResourceType
                    << " to " << mapResourcesChunk->ResourceName.NormalizedFileName << " in " << resourceContainer.Name << std::endl;
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
                std::copy(mem + chunk->FileOffset, mem + chunk->FileOffset + 8, (std::byte*)&fileOffset);
                std::copy(mem + chunk->FileOffset + 8, mem + chunk->FileOffset + 16, (std::byte*)&size);

                std::vector<std::byte> blangFileBytes(mem + fileOffset, mem + fileOffset + size);
                std::vector<std::byte> decryptedBlangFileBytes = IdCrypt(blangFileBytes, modFile.Name, true);

                if (decryptedBlangFileBytes.empty()) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to decrypt " << resourceContainer.Name << "/" << modFile.Name << std::endl;
                    continue;
                }

                try {
                    blangFileEntry = BlangFileEntry(BlangFile(decryptedBlangFileBytes), *chunk);
                    blangFileEntries[blangFilePath] = blangFileEntry;
                }
                catch (...) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to parse " << resourceContainer.Name << "/" << modFile.Name << std::endl;
                    continue;
                }
            }

            nlohmann::json blangJson;

            try {
                blangJson = nlohmann::json::parse(std::string((char*)modFile.FileBytes.data(), modFile.FileBytes.size()));

                if (blangJson == NULL || blangJson["strings"].empty())
                    throw std::exception();

                for (auto &blangJsonString : blangJson["strings"]) {
                    if (blangJsonString == NULL || blangJsonString["name"].empty())
                        throw std::exception();
                }
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to parse EternalMod/strings/" << std::filesystem::path(modFile.Name).replace_extension(".json").string() << std::endl;
                continue;
            }

            for (auto &blangJsonString : blangJson["strings"]) {
                bool stringFound = false;

                for (auto &blangString : blangFileEntries[blangFilePath].BlangFile.Strings) {
                    if (blangJsonString["name"] == blangString.Identifier) {
                        stringFound = true;
                        blangString.Text = blangJsonString["text"];

                        std::cout << "\tReplaced " << blangString.Identifier << " in " << modFile.Name << std::endl;
                        blangFileEntries[blangFilePath].WasModified = true;
                        break;
                    }
                }

                if (stringFound)
                    continue;

                BlangString newBlangString;
                newBlangString.Identifier = blangJsonString["name"];
                newBlangString.Text = blangJsonString["text"];
                blangFileEntries[blangFilePath].BlangFile.Strings.push_back(newBlangString);

                std::cout << "\tAdded " << blangJsonString["name"].get<std::string>() << " in " << modFile.Name << std::endl;
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
                    std::cout << "\tSuccessfully set compressed texture data for file " << modFile.Name << std::endl;
            }
            else if (CompressTextures) {
                std::vector<std::byte> compressedData;

                try {
                    compressedData = OodleCompress(modFile.FileBytes, OodleFormat::Kraken, OodleCompressionLevel::Normal);

                    if (compressedData.empty())
                        throw std::exception();
                }
                catch (...) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to compress " << modFile.Name << std::endl;
                    continue;
                }

                modFile.FileBytes = compressedData;
                compressedSize = compressedData.size();
                compressionMode = (std::byte)2;

                if (Verbose)
                    std::cout << "\tSuccessfully compressed texture file " << modFile.Name << std::endl;
            }
        }

#ifdef _WIN32
        if (SetModDataForChunk(mem, hFile, fileMapping, resourceContainer, *chunk, modFile, compressedSize, uncompressedSize, &compressionMode) == -1) {
#else
        if (SetModDataForChunk(mem, fd, resourceContainer, *chunk, modFile, compressedSize, uncompressedSize, &compressionMode) == -1) {
#endif
            std::cerr << RED << "ERROR: " << RESET << "Failed to set new mod data for " << modFile.Name << " in resource chunk." << std::endl;
            continue;
        }

        std::cout << "\tReplaced " << modFile.Name << std::endl;
        fileCount++;
    }

    if (packageMapSpec != NULL && wasPackageMapSpecModified) {
        FILE *packageMapSpecFile = fopen(packageMapSpecPath.c_str(), "wb");

        if (!packageMapSpecFile) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to write to " << packageMapSpecPath
                << "while trying to add extra resources for level " << resourceContainer.Name << std::endl;
        }
        else {
            try {
                std::string newPackageMapSpecJson = packageMapSpec->Dump();

                if (fwrite(newPackageMapSpecJson.c_str(), 1, newPackageMapSpecJson.size(), packageMapSpecFile) != newPackageMapSpecJson.size())
                    throw std::exception();
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to write to " << packageMapSpecPath
                    << "while trying to add extra resources for level " << resourceContainer.Name << std::endl;
            }

            fclose(packageMapSpecFile);
        }

        delete packageMapSpec;
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
            std::cerr << RED << "ERROR: " << RESET << "Failed to encrypt " << blangFileEntry.first << std::endl;
            continue;
        }

        ResourceModFile blangModFile(Mod(), blangFileEntry.first);
        blangModFile.FileBytes = cryptData;
        std::byte compressionMode = (std::byte)0;

#ifdef _WIN32
        if (SetModDataForChunk(mem, hFile, fileMapping, resourceContainer, blangFileEntry.second.Chunk, blangModFile, blangModFile.FileBytes.size(), blangModFile.FileBytes.size(), &compressionMode) == -1) {
#else
        if (SetModDataForChunk(mem, fd, resourceContainer, blangFileEntry.second.Chunk, blangModFile, blangModFile.FileBytes.size(), blangModFile.FileBytes.size(), &compressionMode) == -1) {
#endif
            std::cerr << RED << "ERROR: " << RESET << "Failed to set new mod data for " << blangFileEntry.first << "in resource chunk." << std::endl;
            continue;
        }

        std::cout << "\tModified " << blangFileEntry.first << std::endl;
    }

    if (mapResourcesFile != NULL && mapResourcesChunk != NULL && !originalDecompressedMapResources.empty()) {
        std::vector<std::byte> decompressedMapResourcesData = mapResourcesFile->ToByteVector();

        if (decompressedMapResourcesData != originalDecompressedMapResources) {
            std::vector<std::byte> compressedMapResourcesData = OodleCompress(decompressedMapResourcesData, OodleFormat::Kraken, OodleCompressionLevel::Normal);

            if (compressedMapResourcesData.empty()) {
                std::cerr << "ERROR: " << RESET << "Failed to compress " << mapResourcesChunk->ResourceName.NormalizedFileName << std::endl;
            }
            else {
                ResourceModFile mapResourcesModFile(Mod(), mapResourcesChunk->ResourceName.NormalizedFileName);
                mapResourcesModFile.FileBytes = compressedMapResourcesData;

#ifdef _WIN32
                if (SetModDataForChunk(mem, hFile, fileMapping, resourceContainer, *mapResourcesChunk,  mapResourcesModFile, compressedMapResourcesData.size(), decompressedMapResourcesData.size(), NULL) == -1) {
#else
                if (SetModDataForChunk(mem, fd, resourceContainer, *mapResourcesChunk,  mapResourcesModFile, compressedMapResourcesData.size(), decompressedMapResourcesData.size(), NULL) == -1) {
#endif
                    std::cerr << RED << "ERROR: " << RESET << "Failed to set new mod data for " << mapResourcesChunk->ResourceName.NormalizedFileName << "in resource chunk." << std::endl;
                    delete mapResourcesFile;
                    return;
                }

                std::cout << "\tModified " << mapResourcesChunk->ResourceName.NormalizedFileName << std::endl;
                delete mapResourcesFile;
            }
        }
    }
    
    if (fileCount > 0)
        std::cout << "Number of files replaced: " << GREEN << fileCount << " file(s) " << RESET << "in " << YELLOW << resourceContainer.Path << RESET << "." << std::endl;
}