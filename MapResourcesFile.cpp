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
#include <memory>

#include "EternalModLoader.hpp"

std::vector<std::byte> MapResourcesFile::ToByteVector()
{
    std::vector<std::byte> mapResourcesBytes;

    mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&Magic, (std::byte*)&Magic + 4);

    int layerCount = Layers.size();
    std::reverse((std::byte*)&layerCount, (std::byte*)&layerCount + 4);
    mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&layerCount, (std::byte*)&layerCount + 4);

    for (auto &layer : Layers) {
        int layerSize = layer.size();
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&layerSize, (std::byte*)&layerSize + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)layer.c_str(), (std::byte*)layer.c_str() + layerSize);
    }

    long assetTypesCount = AssetTypes.size();
    std::reverse((std::byte*)&assetTypesCount, (std::byte*)&assetTypesCount + 8);
    mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&assetTypesCount, (std::byte*)&assetTypesCount + 8);

    for (auto &assetType : AssetTypes) {
        int assetTypeSize = assetType.size();
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&assetTypeSize, (std::byte*)&assetTypeSize + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)assetType.c_str(), (std::byte*)assetType.c_str() + assetTypeSize);
    }

    int assetCount = Assets.size();
    std::reverse((std::byte*)&assetCount, (std::byte*)&assetCount + 4);
    mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&assetCount, (std::byte*)&assetCount + 4);

    for (auto &asset : Assets) {
        int assetTypeIndex = asset.AssetTypeIndex;
        std::reverse((std::byte*)&assetTypeIndex, (std::byte*)&assetTypeIndex + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&assetTypeIndex, (std::byte*)&assetTypeIndex + 4);

        int assetNameSize = asset.Name.size();
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

    int mapCount = Maps.size();
    std::reverse((std::byte*)&mapCount, (std::byte*)&mapCount + 4);
    mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&mapCount, (std::byte*)&mapCount + 4);

    for (auto &map : Maps) {
        int mapSize = map.size();
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)&mapSize, (std::byte*)&mapSize + 4);
        mapResourcesBytes.insert(mapResourcesBytes.end(), (std::byte*)map.c_str(), (std::byte*)map.c_str() + mapSize);
    }

    return mapResourcesBytes;
}

MapResourcesFile::MapResourcesFile(std::vector<std::byte> &rawData)
{
    long pos = 0;

    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&Magic);
    pos += 4;

    std::reverse(rawData.begin() + pos, rawData.begin() + pos + 4);
    int layerCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&layerCount);
    pos += 4;

    Layers.reserve(layerCount);

    for (int i = 0; i < layerCount; i++) {
        int stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&stringSize);
        pos += 4;

        std::string layer((char*)rawData.data() + pos, stringSize);
        Layers.push_back(layer);
        pos += stringSize;
    }

    std::reverse(rawData.begin() + pos, rawData.begin() + pos + 8);
    long assetTypeCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 8, (std::byte*)&assetTypeCount);
    pos += 8;

    AssetTypes.reserve(assetTypeCount);

    for (int i = 0; i < assetTypeCount; i++) {
        int stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&stringSize);
        pos += 4;

        std::string assetType((char*)rawData.data() + pos, stringSize);
        AssetTypes.push_back(assetType);
        pos += stringSize;
    }

    std::reverse(rawData.begin() + pos, rawData.begin() + pos + 4);
    int assetCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&assetCount);
    pos += 4;

    Assets.reserve(assetCount);

    for (int i = 0; i < assetCount; i++) {
        MapAsset mapAsset;

        std::reverse(rawData.begin() + pos, rawData.begin() + pos + 4);
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&mapAsset.AssetTypeIndex);
        pos += 4;

        int stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&stringSize);
        pos += 4;

        mapAsset.Name = std::string((char*)rawData.data() + pos, stringSize);
        pos += stringSize;

        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&mapAsset.UnknownData1);
        pos += 4;

        std::reverse(rawData.begin() + pos, rawData.begin() + pos + 4);
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&mapAsset.UnknownData2);
        pos += 4;

        std::reverse(rawData.begin() + pos, rawData.begin() + pos + 8);
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 8, (std::byte*)&mapAsset.UnknownData3);
        pos += 8;

        std::reverse(rawData.begin() + pos, rawData.begin() + pos + 8);
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 8, (std::byte*)&mapAsset.UnknownData4);
        pos += 8;

        Assets.push_back(mapAsset);
    }

    std::reverse(rawData.begin() + pos, rawData.begin() + pos + 4);
    int mapCount;
    std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&mapCount);
    pos += 4;

    Maps.reserve(mapCount);

    for (int i = 0; i < mapCount; i++) {
        int stringSize;
        std::copy(rawData.begin() + pos, rawData.begin() + pos + 4, (std::byte*)&stringSize);
        pos += 4;

        std::string map((char*)rawData.data() + pos, stringSize);
        Maps.push_back(map);
        pos += stringSize;
    }
}