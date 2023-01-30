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
#include "MapResourcesFile.hpp"

MapResourcesFile::MapResourcesFile(std::vector<std::byte>& rawData)
{
    size_t pos = 0;

    // Read the magic
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, reinterpret_cast<std::byte*>(&Magic));
    pos += 4;

    // Read layer count (big endian)
    unsigned int layerCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, reinterpret_cast<std::byte*>(&layerCount));
    std::reverse(reinterpret_cast<std::byte*>(&layerCount), reinterpret_cast<std::byte*>(&layerCount) + 4);
    pos += 4;

    Layers.reserve(layerCount);

    // Read layers
    for (unsigned int i = 0; i < layerCount; i++) {
        unsigned int stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, reinterpret_cast<std::byte*>(&stringSize));
        pos += 4;

        std::string layer(reinterpret_cast<char*>(rawData.data()) + pos, stringSize);
        Layers.push_back(layer);
        pos += stringSize;
    }

    // Read asset type count (big endian)
    uint64_t assetTypeCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 8, reinterpret_cast<std::byte*>(&assetTypeCount));
    std::reverse(reinterpret_cast<std::byte*>(&assetTypeCount), reinterpret_cast<std::byte*>(&assetTypeCount) + 8);
    pos += 8;

    // Read asset types
    AssetTypes.reserve(assetTypeCount);

    for (size_t i = 0; i < assetTypeCount; i++) {
        unsigned int stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, reinterpret_cast<std::byte*>(&stringSize));
        pos += 4;

        std::string assetType(reinterpret_cast<char*>(rawData.data()) + pos, stringSize);
        AssetTypes.push_back(assetType);
        pos += stringSize;
    }

    // Read assets count (big endian)
    unsigned int assetCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, reinterpret_cast<std::byte*>(&assetCount));
    std::reverse(reinterpret_cast<std::byte*>(&assetCount), reinterpret_cast<std::byte*>(&assetCount) + 4);
    pos += 4;

    Assets.reserve(assetCount);

    // Read assets
    for (unsigned int i = 0; i < assetCount; i++) {
        MapAsset mapAsset;

        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, reinterpret_cast<std::byte*>(&mapAsset.AssetTypeIndex));
        std::reverse(reinterpret_cast<std::byte*>(&mapAsset.AssetTypeIndex), reinterpret_cast<std::byte*>(&mapAsset.AssetTypeIndex) + 4);
        pos += 4;

        unsigned int stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, reinterpret_cast<std::byte*>(&stringSize));
        pos += 4;

        mapAsset.Name = std::string(reinterpret_cast<char*>(rawData.data()) + pos, stringSize);
        pos += stringSize;

        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, reinterpret_cast<std::byte*>(&mapAsset.UnknownData1));
        pos += 4;

        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, reinterpret_cast<std::byte*>(&mapAsset.UnknownData2));
        std::reverse(reinterpret_cast<std::byte*>(&mapAsset.UnknownData2), reinterpret_cast<std::byte*>(&mapAsset.UnknownData2) + 4);
        pos += 4;

        std::copy(rawData.begin() + pos, rawData.begin() + pos + 8, reinterpret_cast<std::byte*>(&mapAsset.UnknownData3));
        std::reverse(reinterpret_cast<std::byte*>(&mapAsset.UnknownData3), reinterpret_cast<std::byte*>(&mapAsset.UnknownData3) + 8);
        pos += 8;

        std::copy(rawData.begin() + pos, rawData.begin() + pos + 8, reinterpret_cast<std::byte*>(&mapAsset.UnknownData4));
        std::reverse(reinterpret_cast<std::byte*>(&mapAsset.UnknownData4), reinterpret_cast<std::byte*>(&mapAsset.UnknownData4) + 8);
        pos += 8;

        Assets.push_back(mapAsset);
    }

    // Read map count (big endian)
    unsigned int mapCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, reinterpret_cast<std::byte*>(&mapCount));
    std::reverse(reinterpret_cast<std::byte*>(&mapCount), reinterpret_cast<std::byte*>(&mapCount) + 4);
    pos += 4;

    Maps.reserve(mapCount);

    // Read maps
    for (unsigned int i = 0; i < mapCount; i++) {
        unsigned int stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, reinterpret_cast<std::byte*>(&stringSize));
        pos += 4;

        std::string map(reinterpret_cast<char*>(rawData.data()) + pos, stringSize);
        Maps.push_back(map);
        pos += stringSize;
    }
}

std::vector<std::byte> MapResourcesFile::ToByteVector()
{
    std::vector<std::byte> mapResourcesBytes;

    // Write magic
    mapResourcesBytes.insert(mapResourcesBytes.end(), reinterpret_cast<std::byte*>(&Magic), reinterpret_cast<std::byte*>(&Magic) + 4);

    // Write layer count (big endian)
    auto layerCount = static_cast<unsigned int>(Layers.size());
    std::reverse(reinterpret_cast<std::byte*>(&layerCount), reinterpret_cast<std::byte*>(&layerCount) + 4);
    mapResourcesBytes.insert(mapResourcesBytes.end(), reinterpret_cast<std::byte*>(&layerCount), reinterpret_cast<std::byte*>(&layerCount) + 4);

    // Write layers
    for (auto& layer : Layers) {
        auto layerSize = static_cast<unsigned int>(layer.size());
        mapResourcesBytes.insert(mapResourcesBytes.end(), reinterpret_cast<std::byte*>(&layerSize), reinterpret_cast<std::byte*>(&layerSize) + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(),
            reinterpret_cast<const std::byte*>(layer.c_str()), reinterpret_cast<const std::byte*>(layer.c_str()) + layerSize);
    }

    // Write asset type count (big endian)
    uint64_t assetTypeCount = AssetTypes.size();
    std::reverse(reinterpret_cast<std::byte*>(&assetTypeCount), reinterpret_cast<std::byte*>(&assetTypeCount) + 8);
    mapResourcesBytes.insert(mapResourcesBytes.end(), reinterpret_cast<std::byte*>(&assetTypeCount), reinterpret_cast<std::byte*>(&assetTypeCount) + 8);

    // Write asset types
    for (auto& assetType : AssetTypes) {
        auto assetTypeSize = static_cast<unsigned int>(assetType.size());
        mapResourcesBytes.insert(mapResourcesBytes.end(), reinterpret_cast<std::byte*>(&assetTypeSize), reinterpret_cast<std::byte*>(&assetTypeSize) + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(),
            reinterpret_cast<const std::byte*>(assetType.c_str()), reinterpret_cast<const std::byte*>(assetType.c_str()) + assetTypeSize);
    }

    // Write asset count (big endian)
    auto assetCount = static_cast<unsigned int>(Assets.size());
    std::reverse(reinterpret_cast<std::byte*>(&assetCount), reinterpret_cast<std::byte*>(&assetCount) + 4);
    mapResourcesBytes.insert(mapResourcesBytes.end(), reinterpret_cast<std::byte*>(&assetCount), reinterpret_cast<std::byte*>(&assetCount) + 4);

    // Write assets
    for (auto& asset : Assets) {
        int assetTypeIndex = asset.AssetTypeIndex;
        std::reverse(reinterpret_cast<std::byte*>(&assetTypeIndex), reinterpret_cast<std::byte*>(&assetTypeIndex) + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(),
            reinterpret_cast<std::byte*>(&assetTypeIndex), reinterpret_cast<std::byte*>(&assetTypeIndex) + 4);

        auto assetNameSize = static_cast<unsigned int>(asset.Name.size());
        mapResourcesBytes.insert(mapResourcesBytes.end(), reinterpret_cast<std::byte*>(&assetNameSize), reinterpret_cast<std::byte*>(&assetNameSize) + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(),
            reinterpret_cast<const std::byte*>(asset.Name.c_str()), reinterpret_cast<const std::byte*>(asset.Name.c_str()) + assetNameSize);

        mapResourcesBytes.insert(mapResourcesBytes.end(),
            reinterpret_cast<std::byte*>(&asset.UnknownData1), reinterpret_cast<std::byte*>(&asset.UnknownData1) + 4);

        std::reverse(reinterpret_cast<std::byte*>(&asset.UnknownData2), reinterpret_cast<std::byte*>(&asset.UnknownData2) + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(),
            reinterpret_cast<std::byte*>(&asset.UnknownData2), reinterpret_cast<std::byte*>(&asset.UnknownData2) + 4);

        std::reverse(reinterpret_cast<std::byte*>(&asset.UnknownData3), reinterpret_cast<std::byte*>(&asset.UnknownData3) + 8);
        mapResourcesBytes.insert(mapResourcesBytes.end(),
            reinterpret_cast<std::byte*>(&asset.UnknownData3), reinterpret_cast<std::byte*>(&asset.UnknownData3) + 8);

        std::reverse(reinterpret_cast<std::byte*>(&asset.UnknownData4), reinterpret_cast<std::byte*>(&asset.UnknownData4) + 8);
        mapResourcesBytes.insert(mapResourcesBytes.end(),
            reinterpret_cast<std::byte*>(&asset.UnknownData4), reinterpret_cast<std::byte*>(&asset.UnknownData4) + 8);
    }

    // Write map count (big endian)
    auto mapCount = static_cast<unsigned int>(Maps.size());
    std::reverse(reinterpret_cast<std::byte*>(&mapCount), reinterpret_cast<std::byte*>(&mapCount) + 4);
    mapResourcesBytes.insert(mapResourcesBytes.end(), reinterpret_cast<std::byte*>(&mapCount), reinterpret_cast<std::byte*>(&mapCount) + 4);

    // Write maps
    for (auto& map : Maps) {
        auto mapSize = static_cast<unsigned int>(map.size());
        mapResourcesBytes.insert(mapResourcesBytes.end(), reinterpret_cast<std::byte*>(&mapSize), reinterpret_cast<std::byte*>(&mapSize) + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(), reinterpret_cast<const std::byte*>(map.c_str()), reinterpret_cast<const std::byte*>(map.c_str()) + mapSize);
    }

    return mapResourcesBytes;
}
