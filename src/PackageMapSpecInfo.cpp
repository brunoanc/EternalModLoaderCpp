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
#include <filesystem>
#include "Utils.hpp"
#include "PackageMapSpecInfo.hpp"

namespace fs = std::filesystem;

bool PackageMapSpecInfo::ReadPackageMapSpec(const std::string& basePath)
{
    // Read JSON
    PackageMapSpecPath = basePath + "packagemapspec.json";
    FILE *packageMapSpecFile = fopen(PackageMapSpecPath.c_str(), "rb");

    if (!packageMapSpecFile) {
        InvalidPackageMapSpec = true;
        return false;
    }

    size_t filesize = fs::file_size(PackageMapSpecPath);
    std::vector<std::byte> packageMapSpecBytes(filesize);

    if (fread(packageMapSpecBytes.data(), 1, filesize, packageMapSpecFile) != filesize) {
        InvalidPackageMapSpec = true;
        return false;
    }

    fclose(packageMapSpecFile);

    // Try to parse the JSON
    try {
        std::string packageMapSpecJson(reinterpret_cast<char*>(packageMapSpecBytes.data()), packageMapSpecBytes.size());
        PackageMapSpec = std::make_unique<class PackageMapSpec>(packageMapSpecJson);
    }
    catch (...) {
        InvalidPackageMapSpec = true;
        return false;
    }

    if (PackageMapSpec == nullptr) {
        InvalidPackageMapSpec = true;
        return false;
    }

    return true;
}

bool PackageMapSpecInfo::ModifyPackageMapSpec(const std::string& basePath, const std::vector<StreamDBContainer>& streamDBContainerList)
{
    // Add custom streamdb if needed
    auto x = std::find_if(streamDBContainerList.begin(), streamDBContainerList.end(),
        [](const StreamDBContainer& streamDBContainer){ return streamDBContainer.Name == "EternalMod.streamdb"; });

    if (x != streamDBContainerList.end()) {
        // Get custom streamdb
        size_t streamDBContainerIndex = std::distance(streamDBContainerList.begin(), x);
        auto streamDBContainer = streamDBContainerList[streamDBContainerIndex];

        if (!streamDBContainer.StreamDBEntries.empty()) {
            if (PackageMapSpec == nullptr && !InvalidPackageMapSpec) {
                // Read packagemapspec JSON
                if (!ReadPackageMapSpec(basePath)) {
                    throw std::exception();
                }
            }

            AddCustomStreamDB("EternalMod.streamdb");
        }
    }

    // Check if PackageMapSpec was modified
    if (PackageMapSpec != nullptr && WasPackageMapSpecModified) {
        // Open packagemapspec.json for writing
        FILE *packageMapSpecFile = fopen(PackageMapSpecPath.c_str(), "wb");

        if (!packageMapSpecFile) {
            return false;
        }

        try {
            // Convert PackageMapSpec object back to JSON
            std::string newPackageMapSpecJson = PackageMapSpec->Dump();

            // Write new JSON
            if (fwrite(newPackageMapSpecJson.c_str(), 1, newPackageMapSpecJson.size(), packageMapSpecFile) != newPackageMapSpecJson.size()) {
                throw std::exception();
            }
        }
        catch (...) {
            return false;
        }

        fclose(packageMapSpecFile);
    }

    return true;
}

void PackageMapSpecInfo::AddCustomStreamDB(std::string fileName)
{
    if (PackageMapSpec == nullptr) {
        return;
    }

    // Find the first streamdb file in the files array
    auto x = std::find_if(PackageMapSpec->Files.begin(), PackageMapSpec->Files.end(),
        [](const PackageMapSpecFile& packageMapSpecFile) { return EndsWith(packageMapSpecFile.Name, ".streamdb"); });

    if (x == PackageMapSpec->Files.end()) {
        // This really shouldn't happen
        x = PackageMapSpec->Files.end() - 1;
    }

    int firstStreamDBIndex = std::distance(PackageMapSpec->Files.begin(), x);

    // Insert the modded streamdb file before the rest
    PackageMapSpecFile streamDBFile;
    streamDBFile.Name = fileName;
    PackageMapSpec->Files.insert(x, streamDBFile);

    // Fix the file indexes in the mapFileRefs
    for (auto& mapFileRef : PackageMapSpec->MapFileRefs) {
        if (mapFileRef.File >= firstStreamDBIndex) {
            mapFileRef.File += 1;
        }
    }

    // Get common map index
    int commonMapIndex = -1;

    for (size_t i = 0; i < PackageMapSpec->Maps.size(); i++) {
        if (PackageMapSpec->Maps[i].Name == "common") {
            commonMapIndex = i;
            break;
        }
    }

    if (commonMapIndex == -1) {
        // This really shouldn't happen either
        PackageMapSpecMap commonMap;
        commonMap.Name = "common";
        PackageMapSpec->Maps.insert(PackageMapSpec->Maps.begin(), commonMap);
        commonMapIndex = 0;
    }

    // Add custom streamdb to common map
    PackageMapSpec->MapFileRefs.push_back(PackageMapSpecMapFileRef(firstStreamDBIndex, commonMapIndex));
    WasPackageMapSpecModified = true;
}
