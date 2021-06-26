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

#include "jsonxx/jsonxx.h"
#include "AssetsInfo/AssetsInfo.hpp"

/**
 * @brief Construct a new AssetsInfo object from a JSON string
 * 
 * @param json JSON string to deserialize
 */
AssetsInfo::AssetsInfo(std::string &json)
{
    jsonxx::Object assetsInfoJson;
    assetsInfoJson.parse(json);

    if (assetsInfoJson.has<jsonxx::Array>("layers")) {
        AssetsInfoLayer assetsInfoLayer;
        jsonxx::Array layers = assetsInfoJson.get<jsonxx::Array>("layers");
        Layers.reserve(layers.size());

        for (int32_t i = 0; i < layers.size(); i++) {
            jsonxx::Object layer = layers.get<jsonxx::Object>(i);

            if (layer.has<jsonxx::String>("name"))
                assetsInfoLayer.Name = layer.get<jsonxx::String>("name");

            Layers.push_back(assetsInfoLayer);
        }
    }

    if (assetsInfoJson.has<jsonxx::Array>("maps")) {
        AssetsInfoMap assetsInfoMap;
        jsonxx::Array maps = assetsInfoJson.get<jsonxx::Array>("maps");
        Maps.reserve(maps.size());

        for (int32_t i = 0; i < maps.size(); i++) {
            jsonxx::Object map = maps.get<jsonxx::Object>(i);

            if (map.has<jsonxx::String>("name"))
                assetsInfoMap.Name = map.get<jsonxx::String>("name");

            Maps.push_back(assetsInfoMap);
        }
    }

    if (assetsInfoJson.has<jsonxx::Array>("resources")) {
        AssetsInfoResource assetsInfoResource;
        jsonxx::Array resources = assetsInfoJson.get<jsonxx::Array>("resources");
        Resources.reserve(resources.size());

        for (int32_t i = 0; i < resources.size(); i++) {
            jsonxx::Object resource = resources.get<jsonxx::Object>(i);

            if (resource.has<jsonxx::String>("name"))
                assetsInfoResource.Name = resource.get<jsonxx::String>("name");

            if (resource.has<jsonxx::Boolean>("remove"))
                assetsInfoResource.Remove = resource.get<jsonxx::Boolean>("remove");

            if (resource.has<jsonxx::Boolean>("placeBefore"))
                assetsInfoResource.PlaceBefore = resource.get<jsonxx::Boolean>("placeBefore");

            if (resource.has<jsonxx::String>("placeByName"))
                assetsInfoResource.PlaceByName = resource.get<jsonxx::String>("placeByName");

            Resources.push_back(assetsInfoResource);
        }
    }

    if (assetsInfoJson.has<jsonxx::Array>("assets")) {
        AssetsInfoAsset assetsInfoAsset;
        jsonxx::Array assets = assetsInfoJson.get<jsonxx::Array>("assets");
        Assets.reserve(assets.size());

        for (int32_t i = 0; i < assets.size(); i++) {
            jsonxx::Object asset = assets.get<jsonxx::Object>(i);

            if (asset.has<jsonxx::Number>("streamDbHash"))
                assetsInfoAsset.StreamDbHash = asset.get<jsonxx::Number>("streamDbHash");

            if (asset.has<jsonxx::String>("resourceType"))
                assetsInfoAsset.ResourceType = asset.get<jsonxx::String>("resourceType");

            if (asset.has<jsonxx::Number>("version"))
                assetsInfoAsset.Version = (std::byte)asset.get<jsonxx::Number>("version");

            if (asset.has<jsonxx::String>("name"))
                assetsInfoAsset.Name = asset.get<jsonxx::String>("name");

            if (asset.has<jsonxx::String>("mapResourceType"))
                assetsInfoAsset.MapResourceType = asset.get<jsonxx::String>("mapResourceType");

            if (asset.has<jsonxx::Boolean>("remove"))
                assetsInfoAsset.Remove = asset.get<jsonxx::Boolean>("remove");

            if (asset.has<jsonxx::Boolean>("placeBefore"))
                assetsInfoAsset.PlaceBefore = asset.get<jsonxx::Boolean>("placeBefore");

            if (asset.has<jsonxx::String>("placeByName"))
                assetsInfoAsset.PlaceByName = asset.get<jsonxx::String>("placeByName");

            if (asset.has<jsonxx::String>("placeByType"))
                assetsInfoAsset.PlaceByType = asset.get<jsonxx::String>("placeByType");

            if (asset.has<jsonxx::Number>("specialByte1"))
                assetsInfoAsset.SpecialByte1 = (std::byte)asset.get<jsonxx::Number>("specialByte1");

            if (asset.has<jsonxx::Number>("specialByte2"))
                assetsInfoAsset.SpecialByte2 = (std::byte)asset.get<jsonxx::Number>("specialByte2");

            if (asset.has<jsonxx::Number>("specialByte3"))
                assetsInfoAsset.SpecialByte3 = (std::byte)asset.get<jsonxx::Number>("specialByte3");

            Assets.push_back(assetsInfoAsset);
        }
    }
}