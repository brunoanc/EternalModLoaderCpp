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
#include <algorithm>
#include "MapResourcesFile/MapResourcesFile.hpp"

/**
 * @brief Construct a new MapResourcesFile object
 * 
 * @param rawData Vector containing the MapResources file's bytes
 */
MapResourcesFile::MapResourcesFile(std::vector<std::byte> &rawData)
{
    size_t pos = 0;

    // Read the magic
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&Magic);
    pos += 4;

    // Read layer count (big endian)
    int32_t layerCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&layerCount);
    std::reverse((std::byte*)&layerCount, (std::byte*)&layerCount + 4);
    pos += 4;

    Layers.reserve(layerCount);

    // Read layers
    for (int32_t i = 0; i < layerCount; i++) {
        int32_t stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&stringSize);
        pos += 4;

        std::string layer((char*)rawData.data() + pos, stringSize);
        Layers.push_back(layer);
        pos += stringSize;
    }

    // Read asset type count (big endian)
    int64_t assetTypeCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 8, (std::byte*)&assetTypeCount);
    std::reverse((std::byte*)&assetTypeCount, (std::byte*)&assetTypeCount + 8);
    pos += 8;

    // Read asset types
    AssetTypes.reserve(assetTypeCount);

    for (int32_t i = 0; i < assetTypeCount; i++) {
        int32_t stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&stringSize);
        pos += 4;

        std::string assetType((char*)rawData.data() + pos, stringSize);
        AssetTypes.push_back(assetType);
        pos += stringSize;
    }

    // Read assets count (big endian)
    int32_t assetCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&assetCount);
    std::reverse((std::byte*)&assetCount, (std::byte*)&assetCount + 4);
    pos += 4;

    Assets.reserve(assetCount);

    // Read assets
    for (int32_t i = 0; i < assetCount; i++) {
        MapAsset mapAsset;

        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&mapAsset.AssetTypeIndex);
        std::reverse((std::byte*)&mapAsset.AssetTypeIndex, (std::byte*)&mapAsset.AssetTypeIndex + 4);
        pos += 4;

        int32_t stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&stringSize);
        pos += 4;

        mapAsset.Name = std::string((char*)rawData.data() + pos, stringSize);
        pos += stringSize;

        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&mapAsset.UnknownData1);
        pos += 4;

        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&mapAsset.UnknownData2);
        std::reverse((std::byte*)&mapAsset.UnknownData2, (std::byte*)&mapAsset.UnknownData2 + 4);
        pos += 4;

        std::copy(rawData.begin() + pos, rawData.begin() + pos + 8, (std::byte*)&mapAsset.UnknownData3);
        std::reverse((std::byte*)&mapAsset.UnknownData3, (std::byte*)&mapAsset.UnknownData3 + 8);
        pos += 8;

        std::copy(rawData.begin() + pos, rawData.begin() + pos + 8, (std::byte*)&mapAsset.UnknownData4);
        std::reverse((std::byte*)&mapAsset.UnknownData4, (std::byte*)&mapAsset.UnknownData4 + 8);
        pos += 8;

        Assets.push_back(mapAsset);
    }

    // Read map count (big endian)
    int32_t mapCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&mapCount);
    std::reverse((std::byte*)&mapCount, (std::byte*)&mapCount + 4);
    pos += 4;

    Maps.reserve(mapCount);

    // Read maps
    for (int32_t i = 0; i < mapCount; i++) {
        int32_t stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&stringSize);
        pos += 4;

        std::string map((char*)rawData.data() + pos, stringSize);
        Maps.push_back(map);
        pos += stringSize;
    }
}

/**
 * @brief Convert MapResourcesFile object back to bytes
 * 
 * @return Vector containing the MapResources file's bytes
 */
std::vector<std::byte> MapResourcesFile::ToByteVector() const
{
    std::vector<std::byte> mapResourcesBytes;

    // Write magic
    mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&Magic, (std::byte*)&Magic + 4);

    // Write layer count (big endian)
    int32_t layerCount = Layers.size();
    std::reverse((std::byte*)&layerCount, (std::byte*)&layerCount + 4);
    mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&layerCount, (std::byte*)&layerCount + 4);

    // Write layers
    for (auto &layer : Layers) {
        int32_t layerSize = layer.size();
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&layerSize, (std::byte*)&layerSize + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)layer.c_str(), (std::byte*)layer.c_str() + layerSize);
    }

    // Write asset type count (big endian)
    int64_t assetTypeCount = AssetTypes.size();
    std::reverse((std::byte*)&assetTypeCount, (std::byte*)&assetTypeCount + 8);
    mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&assetTypeCount, (std::byte*)&assetTypeCount + 8);

    // Write asset types
    for (auto &assetType : AssetTypes) {
        int32_t assetTypeSize = assetType.size();
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&assetTypeSize, (std::byte*)&assetTypeSize + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)assetType.c_str(), (std::byte*)assetType.c_str() + assetTypeSize);
    }

    // Write asset count (big endian)
    int32_t assetCount = Assets.size();
    std::reverse((std::byte*)&assetCount, (std::byte*)&assetCount + 4);
    mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&assetCount, (std::byte*)&assetCount + 4);

    // Write assets
    for (auto &asset : Assets) {
        int32_t assetTypeIndex = asset.AssetTypeIndex;
        std::reverse((std::byte*)&assetTypeIndex, (std::byte*)&assetTypeIndex + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&assetTypeIndex, (std::byte*)&assetTypeIndex + 4);

        int32_t assetNameSize = asset.Name.size();
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&assetNameSize, (std::byte*)&assetNameSize + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)asset.Name.c_str(), (std::byte*)asset.Name.c_str() + assetNameSize);

        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&asset.UnknownData1, (std::byte*)&asset.UnknownData1 + 4);

        std::reverse((std::byte*)&asset.UnknownData2, (std::byte*)&asset.UnknownData2 + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&asset.UnknownData2, (std::byte*)&asset.UnknownData2 + 4);

        std::reverse((std::byte*)&asset.UnknownData3, (std::byte*)&asset.UnknownData3 + 8);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&asset.UnknownData3, (std::byte*)&asset.UnknownData3 + 8);

        std::reverse((std::byte*)&asset.UnknownData4, (std::byte*)&asset.UnknownData4 + 8);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&asset.UnknownData4, (std::byte*)&asset.UnknownData4 + 8);
    }

    // Write map count (big endian)
    int32_t mapCount = Maps.size();
    std::reverse((std::byte*)&mapCount, (std::byte*)&mapCount + 4);
    mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&mapCount, (std::byte*)&mapCount + 4);

    // Write maps
    for (auto &map : Maps) {
        int32_t mapSize = map.size();
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&mapSize, (std::byte*)&mapSize + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)map.c_str(), (std::byte*)map.c_str() + mapSize);
    }

    return mapResourcesBytes;
}
