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

#include "json/json.hpp"
#include "EternalModLoader.hpp"

AssetsInfo::AssetsInfo(std::string &json)
{
    nlohmann::json assetsInfoJson = nlohmann::json::parse(json, nullptr, true, true);

    if (assetsInfoJson.contains("layers"))
        Layers.reserve(assetsInfoJson["layers"].size());

    for (auto &layer : assetsInfoJson["layers"]) {
        AssetsInfoLayer assetsInfoLayer;

        if (layer.contains("name"))
            assetsInfoLayer.Name = layer["name"].get<std::string>();

        Layers.push_back(assetsInfoLayer);
    }

    if (assetsInfoJson.contains("maps"))
        Maps.reserve(assetsInfoJson["maps"].size());

    for (auto &map : assetsInfoJson["maps"]) {
        AssetsInfoMap assetsInfoMap;

        if (map.contains("name"))
            assetsInfoMap.Name = map["name"].get<std::string>();

        Maps.push_back(assetsInfoMap);
    }

    if (assetsInfoJson.contains("resources"))
        Resources.reserve(assetsInfoJson["resources"].size());

    for (auto &resource : assetsInfoJson["resources"]) {
        AssetsInfoResource assetsInfoResource;

        if (resource.contains("name"))
            assetsInfoResource.Name = resource["name"].get<std::string>();

        if (resource.contains("remove"))
            assetsInfoResource.Remove = resource["remove"].get<bool>();

        if (resource.contains("placeBefore"))
            assetsInfoResource.PlaceBefore = resource["placeBefore"].get<bool>();

        if (resource.contains("placeByName"))
            assetsInfoResource.PlaceByName = resource["placeByName"].get<std::string>();

        Resources.push_back(assetsInfoResource);
    }

    if (assetsInfoJson.contains("assets"))
        Assets.reserve(assetsInfoJson["assets"].size());

    for (auto &asset : assetsInfoJson["assets"]) {
        AssetsInfoAsset assetsInfoAsset;

        if (asset.contains("streamDbHash"))
            assetsInfoAsset.StreamDbHash = asset["streamDbHash"].get<uint64_t>();

        if (asset.contains("resourceType"))
            assetsInfoAsset.ResourceType = asset["resourceType"].get<std::string>();

        if (asset.contains("version"))
            assetsInfoAsset.Version = asset["version"].get<std::byte>();

        if (asset.contains("name"))
            assetsInfoAsset.Name = asset["name"].get<std::string>();

        if (asset.contains("mapResourceType"))
            assetsInfoAsset.MapResourceType = asset["mapResourceType"].get<std::string>();

        if (asset.contains("remove"))
            assetsInfoAsset.Remove = asset["remove"].get<bool>();

        if (asset.contains("placeBefore"))
            assetsInfoAsset.PlaceBefore = asset["placeBefore"].get<bool>();

        if (asset.contains("placeByName"))
            assetsInfoAsset.PlaceByName = asset["placeByName"].get<std::string>();

        if (asset.contains("placeByType"))
            assetsInfoAsset.PlaceByType = asset["placeByType"].get<std::string>();

        if (asset.contains("specialByte1"))
            assetsInfoAsset.SpecialByte1 = asset["specialByte1"].get<std::byte>();

        if (asset.contains("specialByte2"))
            assetsInfoAsset.SpecialByte2 = asset["specialByte2"].get<std::byte>();

        if (asset.contains("specialByte3"))
            assetsInfoAsset.SpecialByte3 = asset["specialByte3"].get<std::byte>();

        Assets.push_back(assetsInfoAsset);
    }
}