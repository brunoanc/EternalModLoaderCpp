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

void ReplaceChunks(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, ResourceContainer &resourceContainer)
{
    ResourceChunk *mapResourcesChunk = NULL;
    MapResourcesFile *mapResourcesFile = NULL;
    int fileCount = 0;
    
    for (auto &file : resourceContainer.ChunkList) {
        if (EndsWith(file.ResourceName.NormalizedFileName, ".mapresources")) {
            if (resourceContainer.Name.rfind("gameresources", 0) == 0 && EndsWith(file.ResourceName.NormalizedFileName, "init.mapresources"))
                continue;

            mapResourcesChunk = &file;

            long mapResourcesFileOffset;
            std::copy(mem.begin() + mapResourcesChunk->FileOffset, mem.begin() + mapResourcesChunk->FileOffset + 8, (std::byte*)&mapResourcesFileOffset);
            std::vector<std::byte> mapResourcesBytes(mem.begin() + mapResourcesFileOffset, mem.begin() + mapResourcesFileOffset + mapResourcesChunk->SizeZ);

            std::vector<std::byte> decompressedMapResources = OodleDecompress(mapResourcesBytes, mapResourcesChunk->Size);

            if (decompressedMapResources.empty()) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to decompress " << mapResourcesChunk->ResourceName.NormalizedFileName
                    << " - are you trying to add assets in the wrong .resources archive?" << std::endl;
                break;
            }

            try {
                mapResourcesFile = new MapResourcesFile(decompressedMapResources);
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to parse " << mapResourcesChunk->ResourceName.NormalizedFileName
                    << " - are you trying to add assets in the wrong .resources archive?" << std::endl;
                break;
            }

            break;
        }
    }

    std::stable_sort(resourceContainer.ModFileList.begin(), resourceContainer.ModFileList.end(),
        [](ResourceModFile resource1, ResourceModFile resource2) { return resource1.Parent.LoadPriority > resource2.Parent.LoadPriority ? true : false; });

    for (auto &modFile : resourceContainer.ModFileList) {
        ResourceChunk *chunk = NULL;

        if (modFile.IsAssetsInfoJson && modFile.AssetsInfo.has_value()) {
            if (!modFile.AssetsInfo->Resources.empty()) {
                std::string packageMapSpecPath = BasePath + PackageMapSpecJsonFileName;
                FILE *packageMapSpecFile = fopen(packageMapSpecPath.c_str(), "rb");

                if (!packageMapSpecFile) {
                    std::cerr << RED << "ERROR: " << RESET << packageMapSpecPath << " not found while trying to add extra resources for level "
                        << resourceContainer.Name << std::endl;
                }
                else {
                    long filesize = std::filesystem::file_size(packageMapSpecPath);
                    std::vector<std::byte> packageMapSpecBytes(filesize);

                    if (fread(packageMapSpecBytes.data(), 1, filesize, packageMapSpecFile) != filesize) {
                        std::cerr << RED << "ERROR: " << RESET << "Failed to read data from " << packageMapSpecPath
                            << " while trying to add extra resources for level " << resourceContainer.Name << std::endl;
                        continue;
                    }

                    fclose(packageMapSpecFile);

                    PackageMapSpec packageMapSpec;

                    try {
                        std::string packageMapSpecJson((char*)packageMapSpecBytes.data(), packageMapSpecBytes.size());
                        packageMapSpec = PackageMapSpec(packageMapSpecJson);
                    }
                    catch (...) {
                        std::cerr << RED << "ERROR: " << RESET << "Failed to parse " << packageMapSpecPath << std::endl;
                        continue;
                    }

                    for (auto &extraResource : modFile.AssetsInfo->Resources) {
                        std::string extraResourcePath = PathToResourceContainer(extraResource.Name);

                        if (extraResourcePath.empty()) {
                            std::cerr << RED << "WARNING: " << RESET << "Trying to add non-existing extra resource " << extraResource.Name
                                << " to " << resourceContainer.Name << ", skipping" << std::endl;
                            continue;
                        }

                        int fileIndex = -1;
                        int mapIndex = -1;

                        for (int i = 0; i < packageMapSpec.Files.size(); i++) {
                            if (packageMapSpec.Files[i].Name.find(extraResource.Name) != std::string::npos) {
                                fileIndex = i;
                                break;
                            }
                        }

                        std::string modFileMapName = std::filesystem::path(modFile.Name).stem();

                        if (resourceContainer.Name.rfind("dlc_hub", 0) == 0) {
                            modFileMapName = "game/dlc/hub/hub";
                        }
                        else if (resourceContainer.Name.rfind("hub", 0) == 0) {
                            modFileMapName = "game/hub/hub";
                        }

                        for (int i = 0; i < packageMapSpec.Maps.size(); i++) {
                            if (EndsWith(packageMapSpec.Maps[i].Name, modFileMapName)) {
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

                            for (int i = packageMapSpec.MapFileRefs.size() - 1; i >= 0; i--) {
                                if (packageMapSpec.MapFileRefs[i].File == fileIndex && packageMapSpec.MapFileRefs[i].Map == mapIndex) {
                                    packageMapSpec.MapFileRefs.erase(packageMapSpec.MapFileRefs.begin() + i);
                                    mapFileRefRemoved = true;
                                    break;
                                }
                            }

                            if (mapFileRefRemoved) {
                                std::cout << "\tRemoved resource " << packageMapSpec.Files[fileIndex].Name << " to be loaded in map "
                                    << packageMapSpec.Maps[mapIndex].Name << " in " << packageMapSpecPath << "" << std::endl;
                            }
                            else {
                                if (Verbose) {
                                    std::cerr << RED << "WARNING: " << "Resource " << extraResource.Name << " for map "
                                        << packageMapSpec.Maps[mapIndex].Name << " set to be removed was not found" << std::endl;
                                }
                            }

                            continue;
                        }

                        int insertIndex = -1;
                        bool alreadyExists = false;

                        for (int i = 0; i < packageMapSpec.MapFileRefs.size(); i++) {
                            if (packageMapSpec.MapFileRefs[i].Map == mapIndex) {
                                insertIndex = i + 1;

                                if (packageMapSpec.MapFileRefs[i].File == fileIndex) {
                                    alreadyExists = true;
                                    break;
                                }
                            }
                        }

                        if (alreadyExists) {
                            if (Verbose) {
                                std::cerr << RED << "WARNING: " << RESET << "Extra resource " << extraResource.Name << " for map "
                                    << packageMapSpec.Maps[mapIndex].Name << " was already added, skipping" << std::endl;
                            }
                        }

                        if (!extraResource.PlaceByName.empty()) {
                            std::string placeBeforeResourcePath = PathToResourceContainer(extraResource.Name);

                            if (placeBeforeResourcePath.empty()) {
                                std::cerr << RED << "WARNING: " << RESET << "placeByName resource " << extraResource.PlaceByName
                                    << " not found for extra resource entry " << extraResource.Name << ", using normal placement" << std::endl;
                            }
                            else {
                                int placeBeforeFileIndex = 1;

                                for (int i = 0; i < packageMapSpec.Files.size(); i++) {
                                    if (packageMapSpec.Files[i].Name.find(extraResource.PlaceByName) != std::string::npos) {
                                        placeBeforeFileIndex = i;
                                        break;
                                    }
                                }

                                for (int i = 0; i < packageMapSpec.MapFileRefs.size(); i++) {
                                    if (packageMapSpec.MapFileRefs[i].Map == mapIndex
                                        && packageMapSpec.MapFileRefs[i].File == placeBeforeFileIndex) {
                                            insertIndex = i + (!extraResource.PlaceBefore ? 1 : 0);
                                            break;
                                    }
                                }
                            }
                        }

                        PackageMapSpecMapFileRef mapFileRef(fileIndex, mapIndex);

                        if (insertIndex == -1 || insertIndex >= packageMapSpec.MapFileRefs.size()) {
                            packageMapSpec.MapFileRefs.push_back(mapFileRef);
                        }
                        else {
                            packageMapSpec.MapFileRefs.insert(packageMapSpec.MapFileRefs.begin() + insertIndex, mapFileRef);
                        }

                        packageMapSpecFile = fopen(packageMapSpecPath.c_str(), "wb");

                        if (!packageMapSpecFile) {
                            std::cerr << RED << "ERROR: " << RESET << "Failed to write to " << packageMapSpecPath
                                << "while trying to add extra resources for level " << resourceContainer.Name << std::endl;
                        }

                        try {
                            std::string newPackageMapSpecJson = packageMapSpec.Dump();

                            if (fwrite(newPackageMapSpecJson.c_str(), 1, newPackageMapSpecJson.size(), packageMapSpecFile) != newPackageMapSpecJson.size()) {
                                throw std::exception();
                            }
                        }
                        catch (...) {
                            std::cerr << RED << "ERROR: " << RESET << "Failed to write to " << packageMapSpecPath
                                << "while trying to add extra resources for level " << resourceContainer.Name << std::endl;
                            continue;
                        }

                        fclose(packageMapSpecFile);

                        std::cout << "\tAdded extra resource " << packageMapSpec.Files[fileIndex].Name << " to be loaded in map "
                            << packageMapSpec.Maps[mapIndex].Name<< " in " << packageMapSpecPath << "" << std::endl;
                    }
                }
            }

            if (mapResourcesFile == NULL || (modFile.AssetsInfo->Assets.empty() && modFile.AssetsInfo->Maps.empty() && modFile.AssetsInfo->Layers.empty()))
                continue;

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

                        int newAssetTypeIndex = std::distance(mapResourcesFile->AssetTypes.begin(), x);
                        bool assetFound = false;

                        for (int i = 0; i < mapResourcesFile->Assets.size(); i++) {
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

                    int assetTypeIndex = -1;

                    for (int i = 0; i < mapResourcesFile->AssetTypes.size(); i++) {
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
                    int assetPosition = mapResourcesFile->Assets.size();

                    if (!newAsset.PlaceByName.empty()) {
                        if (!newAsset.PlaceByType.empty()) {
                            std::vector<std::string>::iterator x = std::find(mapResourcesFile->AssetTypes.begin(), mapResourcesFile->AssetTypes.end(), newAsset.PlaceByType);

                            if (x != mapResourcesFile->AssetTypes.end()) {
                                int placeByTypeIndex = std::distance(mapResourcesFile->AssetTypes.begin(), x);

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

            continue;
        }
        else if (modFile.IsBlangJson) {
            modFile.Name = modFile.Name.substr(modFile.Name.find('/') + 1);
            modFile.Name = std::filesystem::path(modFile.Name).replace_extension(".blang");

            chunk = GetChunk(modFile.Name, resourceContainer);

            if (chunk == NULL)
                continue;
        }
        else {
            chunk = GetChunk(modFile.Name, resourceContainer);

            if (chunk == NULL) {
                resourceContainer.NewModFileList.push_back(modFile);

                std::map<unsigned long, ResourceDataEntry>::iterator x = ResourceDataMap.find(CalculateResourceFileNameHash(modFile.Name));

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

                if (mapResourcesFile == NULL)
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

                int assetTypeIndex = -1;

                for (int i = 0; i < mapResourcesFile->AssetTypes.size(); i++) {
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

        long fileOffset, size;
        std::copy(mem.begin() + chunk->FileOffset, mem.begin() + chunk->FileOffset + 8, (std::byte*)&fileOffset);
        std::copy(mem.begin() + chunk->FileOffset + 8, mem.begin() + chunk->FileOffset + 16, (std::byte*)&size);

        long sizeDiff = modFile.FileBytes.size() - size;

        if (modFile.IsBlangJson) {
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

            std::vector<std::byte> blangFileBytes(mem.begin() + fileOffset, mem.begin() + fileOffset + size);

            std::vector<std::byte> decryptedBlangFileBytes = IdCrypt(blangFileBytes, modFile.Name, true);

            if (decryptedBlangFileBytes.empty()) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to decrypt " << resourceContainer.Name << "/" << modFile.Name << std::endl;
                continue;
            }

            BlangFile blangFile;

            try {
                blangFile = ParseBlang(decryptedBlangFileBytes, resourceContainer.Name);
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to parse " << resourceContainer.Name << "/" << modFile.Name << std::endl;
                continue;
            }

            for (auto &blangJsonString : blangJson["strings"]) {
                bool stringFound = false;

                for (auto &blangString : blangFile.Strings) {
                    if (blangJsonString["name"] == blangString.Identifier) {
                        stringFound = true;
                        blangString.Text = blangJsonString["text"];

                        std::cout << "\tReplaced " << blangString.Identifier << " in " << modFile.Name << std::endl;
                        break;
                    }
                }

                if (stringFound)
                    continue;

                BlangString newBlangString;
                newBlangString.Identifier = blangJsonString["name"];
                newBlangString.Text = blangJsonString["text"];
                blangFile.Strings.push_back(newBlangString);

                std::cout << "\tAdded " << blangJsonString["name"].get<std::string>() << " in " << modFile.Name << std::endl;
            }

            std::vector<std::byte> cryptDataBuffer = WriteBlangToVector(blangFile, resourceContainer.Name);
            std::vector<std::byte> encryptedBlangFileBytes = IdCrypt(cryptDataBuffer, modFile.Name, false);

            if (encryptedBlangFileBytes.empty()) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to re-encrypt " << resourceContainer.Name << "/" << modFile.Name << std::endl;
                continue;
            }

            modFile.FileBytes = encryptedBlangFileBytes;
        }

        long dataSectionLength = mem.size() - resourceContainer.DataOffset;
        long placement = (0x10 - (dataSectionLength % 0x10)) + 0x30;
        long newContainerSize = mem.size() + modFile.FileBytes.size() + placement;
        long dataOffset = newContainerSize - modFile.FileBytes.size();

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

        std::copy(modFile.FileBytes.begin(), modFile.FileBytes.end(), mem.begin() + dataOffset);
        std::copy((std::byte*)&dataOffset, (std::byte*)&dataOffset + 8, mem.begin() + chunk->FileOffset);

        long modFileBytesSize = modFile.FileBytes.size();
        std::copy((std::byte*)&modFileBytesSize, (std::byte*)&modFileBytesSize + 8, mem.begin() + chunk->SizeOffset);
        std::copy((std::byte*)&modFileBytesSize, (std::byte*)&modFileBytesSize + 8, mem.begin() + chunk->SizeOffset + 8);

        mem[chunk->SizeOffset + 0x30] = (std::byte)0;

        if (!modFile.IsBlangJson && !modFile.IsAssetsInfoJson) {
            std::cout << "\tReplaced " << modFile.Name << std::endl;
            fileCount++;
        }
    }

    std::vector<std::byte> decompressedMapResourcesData = mapResourcesFile->ToByteVector();

    if (decompressedMapResourcesData.size() != mapResourcesChunk->Size) {
        std::vector<std::byte> compressedMapResourcesData = OodleCompress(decompressedMapResourcesData, Kraken, Normal);

        if (compressedMapResourcesData.empty()) {
            std::cerr << "ERROR: " << RESET << "Failed to compress " << mapResourcesChunk->ResourceName.NormalizedFileName << std::endl;
        }
        else {
            mapResourcesChunk->Size = decompressedMapResourcesData.size();
            mapResourcesChunk->SizeZ = compressedMapResourcesData.size();

            long dataSectionLength = mem.size() - resourceContainer.DataOffset;
            long placement = 0x10 - (dataSectionLength % 0x10) + 0x30;
            long newContainerSize = mem.size() + compressedMapResourcesData.size() + placement;
            long dataOffset = newContainerSize - compressedMapResourcesData.size();

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

            std::copy(compressedMapResourcesData.begin(), compressedMapResourcesData.end(), mem.begin() + dataOffset);
            std::copy((std::byte*)&dataOffset, (std::byte*)&dataOffset + 8, mem.begin() + mapResourcesChunk->FileOffset);

            long compressedMapResourcesSize = compressedMapResourcesData.size();
            long decompressedMapResourcesSize = decompressedMapResourcesData.size();
            std::copy((std::byte*)&compressedMapResourcesSize, (std::byte*)&compressedMapResourcesSize + 8, mem.begin() + mapResourcesChunk->SizeOffset);
            std::copy((std::byte*)&decompressedMapResourcesSize, (std::byte*)&decompressedMapResourcesSize + 8, mem.begin() + mapResourcesChunk->SizeOffset + 8);

            std::cout << "\tModified " << mapResourcesChunk->ResourceName.NormalizedFileName << std::endl;
            delete mapResourcesFile;
        }
    }
    
    if (fileCount > 0)
        std::cout << "Number of files replaced: " << GREEN << fileCount << " file(s) " << RESET << "in " << YELLOW << resourceContainer.Path << RESET << "." << std::endl;
}