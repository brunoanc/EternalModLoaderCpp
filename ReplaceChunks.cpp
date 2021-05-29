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

void ReplaceChunks(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, ResourceInfo &resourceInfo)
{
    int fileCount = 0;

    for (auto &mod : resourceInfo.ModList) {
        std::optional<ResourceChunk> chunk = std::nullopt;

        if (mod.IsAssetsInfoJson && mod.AssetsInfo.has_value()) {
            if (!mod.AssetsInfo.value().ExtraResources.empty()) {
                std::string packageMapSpecPath = BasePath + PackageMapSpecJsonFileName;
                FILE *packageMapSpecFile = fopen(packageMapSpecPath.c_str(), "rb");

                if (!packageMapSpecFile) {
                    std::cerr << RED << "ERROR: " << RESET << packageMapSpecPath << " not found while trying to add extra resources for level "
                        << resourceInfo.Name << std::endl;
                }
                else {
                    long filesize = std::filesystem::file_size(packageMapSpecPath);
                    std::vector<std::byte> packageMapSpecBytes(filesize);

                    if (fread(packageMapSpecBytes.data(), 1, filesize, packageMapSpecFile) != filesize) {
                        std::cerr << RED << "ERROR: " << RESET << "Failed to read data from " << packageMapSpecPath
                            << " while trying to add extra resources for level " << resourceInfo.Name << std::endl;
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

                    for (auto &extraResource : mod.AssetsInfo.value().ExtraResources) {
                        std::string extraResourcePath = PathToResource(extraResource.Name);

                        if (extraResourcePath.empty()) {
                            std::cerr << RED << "WARNING: " << RESET << "Trying to add non-existing extra resource " << extraResource.Name
                                << " to " << resourceInfo.Name << ", skipping" << std::endl;
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

                        std::string modNameStem = std::filesystem::path(mod.Name).stem();

                        for (int i = 0; i < packageMapSpec.Maps.size(); i++) {
                            if (packageMapSpec.Maps[i].Name.find(modNameStem) != std::string::npos) {
                                mapIndex = i;
                                break;
                            }
                        }

                        if (fileIndex == -1) {
                            std::cerr << RED << "ERROR: " << RESET << "Invalid extra resource " << extraResource.Name << ", skipping" << std::endl;
                            continue;
                        }

                        if (mapIndex == -1) {
                            std::cerr << RED << "ERROR: " << RESET << "Map reference not found for " << mod.Name << ", skipping" << std::endl;
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
                            std::string placeBeforeResourcePath = PathToResource(extraResource.Name);

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
                                << "while trying to add extra resources for level " << resourceInfo.Name << std::endl;
                        }

                        try {
                            std::string newPackageMapSpecJson = packageMapSpec.Dump();

                            if (fwrite(newPackageMapSpecJson.c_str(), 1, newPackageMapSpecJson.size(), packageMapSpecFile) != newPackageMapSpecJson.size()) {
                                throw;
                            }
                        }
                        catch (...) {
                            std::cerr << RED << "ERROR: " << RESET << "Failed to write to " << packageMapSpecPath
                                << "while trying to add extra resources for level " << resourceInfo.Name << std::endl;
                            continue;
                        }

                        fclose(packageMapSpecFile);

                        std::cout << "\tAdded extra resource " << packageMapSpec.Files[fileIndex].Name << " to be loaded in map "
                            << packageMapSpec.Maps[mapIndex].Name<< " in " << packageMapSpecPath << "" << std::endl;
                    }
                }
            }

            if (!mod.AssetsInfo.value().NewAssets.empty() || !mod.AssetsInfo.value().Maps.empty() || !mod.AssetsInfo.value().Layers.empty()) {
                std::vector<std::string> assetsInfoFilenameParts = SplitString(mod.Name, '/');
                std::string mapResourcesFileName = assetsInfoFilenameParts[assetsInfoFilenameParts.size() - 1];
                mapResourcesFileName = mapResourcesFileName.substr(0, mapResourcesFileName.size() - 4) + "mapresources";

                for (auto &resourceChunk : resourceInfo.ChunkList) {
                    std::vector<std::string> nameParts = SplitString(resourceChunk.ResourceName.FullFileName, '/');

                    if (nameParts[nameParts.size() - 1] == mapResourcesFileName) {
                        chunk = resourceChunk;
                        break;
                    }
                }

                if (!chunk.has_value()) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to find the .mapresources counterpart for AssetsInfo file: "
                        << mod.Name << " - please check that the name for the AssetsInfo file is correct" << std::endl;
                    continue;
                }

                std::vector<std::byte> mapResourcesBytes(chunk.value().SizeZ);

                long mapResourcesFileOffset;
                std::copy(mem.begin() + chunk.value().FileOffset, mem.begin() + chunk.value().FileOffset + 8, (std::byte*)&mapResourcesFileOffset);

                std::copy(mem.begin() + mapResourcesFileOffset, mem.begin() + mapResourcesFileOffset + mapResourcesBytes.size(), mapResourcesBytes.begin());

                std::vector<std::byte> decompressedMapResources;

                try {
                    decompressedMapResources = OodleDecompress(mapResourcesBytes, chunk.value().Size);

                    if (decompressedMapResources.empty())
                        throw;
                }
                catch (...) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to decompress " << chunk.value().ResourceName.NormalizedFileName << std::endl;
                    continue;
                }

                if (decompressedMapResources.empty()) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to decompress " << chunk.value().ResourceName.NormalizedFileName
                        << " - are you trying to add assets in the wrong .resources archive?" << std::endl;
                    continue;
                }

                MapResourcesFile mapResourcesFile;

                try {
                    mapResourcesFile = MapResourcesFile(decompressedMapResources);
                }
                catch (...) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to parse " << chunk.value().ResourceName.NormalizedFileName << std::endl;
                    continue;
                }

                if (!mod.AssetsInfo.value().Layers.empty()) {
                    for (auto &newLayers : mod.AssetsInfo.value().Layers) {
                        if (std::find(mapResourcesFile.Layers.begin(), mapResourcesFile.Layers.end(), newLayers.Name) != mapResourcesFile.Layers.end()) {
                            if (Verbose) {
                                std::cerr << RED << "ERROR: " << RESET << "Trying to add layer " << newLayers.Name << " that has already been added in "
                                    << chunk.value().ResourceName.NormalizedFileName << ", skipping" << std::endl;
                            }

                            continue;
                        }

                        mapResourcesFile.Layers.push_back(newLayers.Name);
                        std::cout << "\tAdded layer " << newLayers.Name << " to " << chunk.value().ResourceName.NormalizedFileName
                            << " in " << resourceInfo.Name << "" << std::endl;
                    }
                }

                if (!mod.AssetsInfo.value().Maps.empty()) {
                    for (auto & newMaps : mod.AssetsInfo.value().Maps) {
                        if (std::find(mapResourcesFile.Maps.begin(), mapResourcesFile.Maps.end(), newMaps.Name) != mapResourcesFile.Maps.end()) {
                            if (Verbose) {
                                std::cerr << RED << "ERROR: " << RESET << "Trying to add map " << newMaps.Name <<" that has already been added in "
                                    << chunk.value().ResourceName.NormalizedFileName << ", skipping" << std::endl;
                            }

                            continue;
                        }

                        mapResourcesFile.Maps.push_back(newMaps.Name);
                        std::cout << "Added map " << newMaps.Name << " to " << chunk.value().ResourceName.NormalizedFileName << " in " << resourceInfo.Name << std::endl;
                    }
                }

                if (!mod.AssetsInfo.value().NewAssets.empty()) {
                    for (auto &newAsset : mod.AssetsInfo.value().NewAssets) {
                        if (RemoveWhitespace(newAsset.Name).empty() || RemoveWhitespace(newAsset.MapResourceType).empty()) {
                            if (Verbose)
                                std::cerr << "WARNING: " << "Skipping empty resource declaration in " << mod.Name << std::endl;

                            continue;
                        }

                        if (newAsset.Remove) {
                            std::vector<std::string>::iterator x = std::find(mapResourcesFile.AssetTypes.begin(), mapResourcesFile.AssetTypes.end(), newAsset.MapResourceType);

                            if (x == mapResourcesFile.AssetTypes.end()) {
                                if (Verbose) {
                                    std::cerr << RED << "WARNING: " << RESET << "Can't remove asset " << newAsset.Name << " with type " << newAsset.MapResourceType <<
                                        " because it doesn't exist in " << chunk.value().ResourceName.NormalizedFileName << std::endl;
                                    continue;
                                }
                            }

                            int newAssetTypeIndex = std::distance(mapResourcesFile.AssetTypes.begin(), x);
                            bool assetFound = false;

                            for (int i = 0; i < mapResourcesFile.Assets.size(); i++) {
                                if (mapResourcesFile.Assets[i].Name == newAsset.Name
                                    && mapResourcesFile.Assets[i].AssetTypeIndex == newAssetTypeIndex) {
                                        assetFound = true;
                                        mapResourcesFile.Assets.erase(mapResourcesFile.Assets.begin() + i);
                                        break;
                                }
                            }

                            if (assetFound) {
                                std::cout << "\tRemoved asset " << newAsset.Name << " with type " << newAsset.MapResourceType <<
                                    " from " << chunk.value().ResourceName.NormalizedFileName << " in " << resourceInfo.Name << std::endl;
                            }
                            else {
                                std::cerr << RED << "WARNING: " << RESET << "Can't remove asset " << newAsset.Name << " with type " << newAsset.MapResourceType <<
                                    " because it doesn't exist in " << chunk.value().ResourceName.NormalizedFileName << std::endl;
                            }

                            continue;
                        }

                        bool alreadyExists = false;

                        for (auto &existingAsset : mapResourcesFile.Assets) {
                            if (existingAsset.Name == newAsset.Name
                                && mapResourcesFile.AssetTypes[existingAsset.AssetTypeIndex] == newAsset.MapResourceType) {
                                    alreadyExists = true;
                                    break;
                            }
                        }

                        if (alreadyExists) {
                            if (Verbose) {
                                std::cerr << RED << "WARNING: " << RESET << "Failed to add asset " << newAsset.Name <<
                                    " that has already been added in " << chunk.value().ResourceName.NormalizedFileName << ", skipping" << std::endl;
                            }

                            continue;
                        }

                        int assetTypeIndex = -1;

                        for (int i = 0; i < mapResourcesFile.AssetTypes.size(); i++) {
                            if (mapResourcesFile.AssetTypes[i] == newAsset.MapResourceType) {
                                assetTypeIndex = i;
                                break;
                            }
                        }

                        if (assetTypeIndex == -1) {
                            mapResourcesFile.AssetTypes.push_back(newAsset.MapResourceType);
                            assetTypeIndex = mapResourcesFile.AssetTypes.size() - 1;

                            std::cout << "Added asset type " << newAsset.MapResourceType << " to " <<
                                chunk.value().ResourceName.NormalizedFileName << " in " << resourceInfo.Name << std::endl;
                        }

                        MapAsset placeByExistingAsset;
                        bool found = false;
                        int assetPosition = mapResourcesFile.Assets.size();

                        if (!newAsset.PlaceByName.empty()) {
                            if (!newAsset.PlaceByType.empty()) {
                                std::vector<std::string>::iterator x = std::find(mapResourcesFile.AssetTypes.begin(), mapResourcesFile.AssetTypes.end(), newAsset.PlaceByType);

                                if (x != mapResourcesFile.AssetTypes.end()) {
                                    int placeByTypeIndex = std::distance(mapResourcesFile.AssetTypes.begin(), x);

                                    for (auto &asset : mapResourcesFile.Assets) {
                                        if (asset.Name == newAsset.PlaceByName && asset.AssetTypeIndex == placeByTypeIndex) {
                                            placeByExistingAsset = asset;
                                            found = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            else {
                                for (auto &asset : mapResourcesFile.Assets) {
                                    if (asset.Name == newAsset.PlaceByName) {
                                        placeByExistingAsset = asset;
                                        found = true;
                                        break;
                                    }
                                }
                            }

                            if (found) {
                                std::vector<MapAsset>::iterator x = std::find(mapResourcesFile.Assets.begin(), mapResourcesFile.Assets.end(), placeByExistingAsset);

                                if (x != mapResourcesFile.Assets.end()) {
                                    assetPosition = std::distance(mapResourcesFile.Assets.begin(), x);

                                    if (!newAsset.PlaceBefore)
                                        assetPosition++;
                                }
                            }
                        }

                        if (Verbose && found) {
                            std::cout << "\tAsset " << newAsset.Name << " with type " << newAsset.MapResourceType << " will be added before asset " << placeByExistingAsset.Name << " with type "
                                << mapResourcesFile.AssetTypes[placeByExistingAsset.AssetTypeIndex] << " to " << chunk.value().ResourceName.NormalizedFileName << " in "<< resourceInfo.Name << std::endl;
                        }

                        MapAsset newMapAsset;
                        newMapAsset.AssetTypeIndex = assetTypeIndex;
                        newMapAsset.Name = newAsset.Name;
                        newMapAsset.UnknownData4 = 128;

                        mapResourcesFile.Assets.insert(mapResourcesFile.Assets.begin() + assetPosition, newMapAsset);

                        std::cout << "\tAdded asset " << newAsset.Name << " with type " << newAsset.MapResourceType << " to " << chunk.value().ResourceName.NormalizedFileName << " in " << resourceInfo.Name << std::endl;
                    }
                }

                decompressedMapResources = mapResourcesFile.ToByteVector();
                std::vector<std::byte> compressedMapResources;

                try {
                    compressedMapResources = OodleCompress(decompressedMapResources, Kraken, NormalCompression);

                    if (compressedMapResources.empty())
                        throw;
                }
                catch (...) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to compress " << chunk.value().ResourceName.NormalizedFileName << std::endl;
                    continue;
                }

                chunk.value().Size = decompressedMapResources.size();
                chunk.value().SizeZ = compressedMapResources.size();
                mod.UncompressedSize = decompressedMapResources.size();
                mod.FileBytes = compressedMapResources;
            }
            else {
                continue;
            }
        }
        else if (mod.IsBlangJson) {
            mod.Name = mod.Name.substr(mod.Name.find('/') + 1);
            mod.Name = std::filesystem::path(mod.Name).replace_extension(".blang");

            ResourceChunk *chunkptr = GetChunk(mod.Name, resourceInfo);

            if (chunkptr == NULL)
                continue;

            chunk = *chunkptr;
        }
        else {
            ResourceChunk *chunkptr = GetChunk(mod.Name, resourceInfo);

            if (chunkptr == NULL) {
                Mod newMod(mod.Name);
                newMod.FileBytes = mod.FileBytes;
                resourceInfo.ModListNew.push_back(newMod);

                std::map<unsigned long, ResourceDataEntry>::iterator x = ResourceDataMap.find(CalculateResourceFileNameHash(mod.Name));

                if (x == ResourceDataMap.end())
                    continue;

                ResourceDataEntry resourceData = x->second;

                if (RemoveWhitespace(resourceData.MapResourceName).empty())
                    resourceData.MapResourceName = mod.Name;

                for (auto &file : resourceInfo.ChunkList) {
                    if (EndsWith(file.ResourceName.NormalizedFileName, ".mapresources")) {
                        if (resourceInfo.Name.rfind("gameresources", 0) == 0
                            && EndsWith(file.ResourceName.NormalizedFileName, "init.mapresources"))
                                continue;

                        chunk = file;
                        break;
                    }
                }

                if (!chunk.has_value())
                    continue;

                long mapResourcesFileOffset;
                std::copy(mem.begin() + chunk.value().FileOffset, mem.begin() + chunk.value().FileOffset + 8, (std::byte*)&mapResourcesFileOffset);

                std::vector<std::byte> mapResourcesBytes(chunk.value().SizeZ);
                std::copy(mem.begin() + mapResourcesFileOffset, mem.begin() + mapResourcesFileOffset + mapResourcesBytes.size(), mapResourcesBytes.begin());

                std::vector<std::byte> decompressedMapResources;

                try {                    
                    decompressedMapResources = OodleDecompress(mapResourcesBytes, chunk.value().Size);

                    if (decompressedMapResources.empty())
                        throw;
                }
                catch (...) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to decompress " << chunk.value().ResourceName.NormalizedFileName << std::endl;
                    continue;
                }

                if (decompressedMapResources.empty()) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to decompress " << chunk.value().ResourceName.NormalizedFileName
                        << " - are you trying to add assets in the wrong .resources archive?" << std::endl;
                    continue;
                }

                MapResourcesFile mapResourcesFile(decompressedMapResources);

                bool alreadyExists = false;

                for (auto &existingAsset : mapResourcesFile.Assets) {
                    if (existingAsset.Name == resourceData.MapResourceName
                        && mapResourcesFile.AssetTypes[existingAsset.AssetTypeIndex] == resourceData.MapResourceType) {
                            alreadyExists = true;
                            break;
                    }
                }

                if (alreadyExists) {
                    if (Verbose)
                        std::cerr << RED << "WARNING: " << RESET << "Trying to add asset " << resourceData.MapResourceName
                            << " that has already been added in " << chunk.value().ResourceName.NormalizedFileName << ", skipping" << std::endl;

                    continue;
                }

                int assetTypeIndex = -1;

                for (int i = 0; i < mapResourcesFile.AssetTypes.size(); i++) {
                    if (mapResourcesFile.AssetTypes[i] == resourceData.MapResourceType) {
                        assetTypeIndex = i;
                        break;
                    }
                }

                if (assetTypeIndex == -1) {
                    mapResourcesFile.AssetTypes.push_back(resourceData.MapResourceType);
                    assetTypeIndex = mapResourcesFile.AssetTypes.size() - 1;

                    std::cout << "\tAdded asset type " << resourceData.MapResourceType << " to "
                        << chunk.value().ResourceName.NormalizedFileName << " in " << resourceInfo.Name << std::endl;
                }

                MapAsset newMapAsset;
                newMapAsset.AssetTypeIndex = assetTypeIndex;
                newMapAsset.Name = resourceData.MapResourceName;
                newMapAsset.UnknownData4 = 128;
                mapResourcesFile.Assets.push_back(newMapAsset);

                std::cout << "\tAdded asset " << resourceData.MapResourceName << " with type " << resourceData.MapResourceType
                    << " to " << chunk.value().ResourceName.NormalizedFileName << " in " << resourceInfo.Name << std::endl;

                decompressedMapResources = mapResourcesFile.ToByteVector();
                std::vector<std::byte> compressedMapResources;

                try {
                    compressedMapResources = OodleCompress(decompressedMapResources, Kraken, NormalCompression);

                    if (compressedMapResources.empty())
                        throw;
                }
                catch (...) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to compress " << chunk.value().ResourceName.NormalizedFileName << std::endl;
                    continue;
                }

                if (compressedMapResources.empty()) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to compress " << chunk.value().ResourceName.NormalizedFileName << std::endl;
                    continue;
                }

                chunk.value().Size = decompressedMapResources.size();
                chunk.value().SizeZ = compressedMapResources.size();

                mod.IsAssetsInfoJson = true;
                mod.UncompressedSize = decompressedMapResources.size();
                mod.FileBytes = compressedMapResources;
            }
            else {
                chunk = *chunkptr;
            }
        }

        long fileOffset, size;
        std::copy(mem.begin() + chunk.value().FileOffset, mem.begin() + chunk.value().FileOffset + 8, (std::byte*)&fileOffset);
        std::copy(mem.begin() + chunk.value().FileOffset + 8, mem.begin() + chunk.value().FileOffset + 16, (std::byte*)&size);

        long sizeDiff = (long)mod.FileBytes.size() - size;

        if (mod.IsBlangJson) {
            nlohmann::ordered_json blangJson;

            try {
                blangJson = nlohmann::ordered_json::parse(std::string((char*)mod.FileBytes.data(), mod.FileBytes.size()));

                if (blangJson == NULL || blangJson["strings"].empty())
                    throw;

                for (auto &blangJsonString : blangJson["strings"]) {
                    if (blangJsonString == NULL || blangJsonString["name"].empty())
                        throw;
                }
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to parse EternalMod/strings/" << std::filesystem::path(mod.Name).replace_extension(".json").string() << std::endl;
                continue;
            }

            std::vector<std::byte> blangFileBytes(mem.begin() + fileOffset, mem.begin() + fileOffset + size);

            std::vector<std::byte> decryptedBlangFileBytes = IdCrypt(blangFileBytes, mod.Name, true);

            if (decryptedBlangFileBytes.empty()) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to decrypt " << resourceInfo.Name << "/" << mod.Name << std::endl;
                continue;
            }

            BlangFile blangFile;

            try {
                blangFile = ParseBlang(decryptedBlangFileBytes, resourceInfo.Name);
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to parse " << resourceInfo.Name << "/" << mod.Name << std::endl;
                continue;
            }

            for (auto &blangJsonString : blangJson["strings"]) {
                bool stringFound = false;

                for (auto &blangString : blangFile.Strings) {
                    if (blangJsonString["name"] == blangString.Identifier) {
                        stringFound = true;
                        blangString.Text = blangJsonString["text"];

                        std::cout << "\tReplaced " << blangString.Identifier << " in " << mod.Name << std::endl;
                        break;
                    }
                }

                if (stringFound)
                    continue;

                BlangString newBlangString;
                newBlangString.Identifier = blangJsonString["name"];
                newBlangString.Text = blangJsonString["text"];

                std::cout << "\tAdded " << blangJsonString["name"].get<std::string>() << " in " << mod.Name << std::endl;
            }

            std::vector<std::byte> cryptDataBuffer = WriteBlangToVector(blangFile, resourceInfo.Name);
            std::vector<std::byte> encryptedBlangFileBytes = IdCrypt(cryptDataBuffer, mod.Name, false);

            if (encryptedBlangFileBytes.empty()) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to re-encrypt " << resourceInfo.Name << "/" << mod.Name << std::endl;
                continue;
            }

            mod.FileBytes = encryptedBlangFileBytes;
        }

        if (sizeDiff > 0) {
            long length = (long)std::filesystem::file_size(resourceInfo.Path);
            mem.munmap_file();
            std::filesystem::resize_file(resourceInfo.Path, length + sizeDiff);

            try {
                mem.mmap_file(resourceInfo.Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, length + sizeDiff,
                    mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);
                
                if (mem.empty())
                    throw;
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to load " << resourceInfo.Path << " into memory for writing"<< std::endl;
                return;
            }

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

            if (sizeDiff < 0) {
                std::byte *emptyArray = new std::byte[-sizeDiff];
                std::memset(emptyArray, 0, -sizeDiff);

                std::copy(emptyArray, emptyArray - sizeDiff, mem.begin() + fileOffset + (long)mod.FileBytes.size());

                delete[] emptyArray;
            }
        }

        long modFileBytesSize = mod.FileBytes.size();
        std::copy((std::byte*)&modFileBytesSize, (std::byte*)&modFileBytesSize + 8, mem.begin() + chunk.value().SizeOffset);
        
        bool isMapResources = mod.IsAssetsInfoJson && mod.UncompressedSize != 0 && !mod.FileBytes.empty();
        long uncompressedSize = isMapResources ? mod.UncompressedSize : mod.FileBytes.size();
        std::copy((std::byte*)&uncompressedSize, (std::byte*)&uncompressedSize + 8, mem.begin() + chunk.value().SizeOffset + 8);

        mem[chunk.value().SizeOffset + 0x30] = isMapResources ? chunk.value().CompressionMode : (std::byte)0;

        if (sizeDiff > 0) {
            std::vector<ResourceChunk>::iterator x = std::find(resourceInfo.ChunkList.begin(), resourceInfo.ChunkList.end(), chunk.value());
            int index = (int)std::distance(resourceInfo.ChunkList.begin(), x);

            for (int i = index + 1; i < resourceInfo.ChunkList.size(); i++) {
                std::copy(mem.begin() + resourceInfo.ChunkList[i].FileOffset, mem.begin() + resourceInfo.ChunkList[i].FileOffset + 8, (std::byte*)&fileOffset);

                long fileOffsetSizeDir = fileOffset + sizeDiff;
                std::copy((std::byte*)&fileOffsetSizeDir, (std::byte*)&fileOffsetSizeDir + 8, mem.begin() + resourceInfo.ChunkList[i].FileOffset);
            }
        }

        if (!mod.IsBlangJson && !mod.IsAssetsInfoJson) {
            std::cout << "\tReplaced " << mod.Name << std::endl;
            fileCount++;
        }
    }
    
    if (fileCount > 0)
        std::cout << "Number of files replaced: " << GREEN << fileCount << " file(s) " << RESET << "in " << YELLOW << resourceInfo.Path << RESET << "." << std::endl;
}