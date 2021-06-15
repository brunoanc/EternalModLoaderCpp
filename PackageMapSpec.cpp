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

PackageMapSpec::PackageMapSpec(std::string &json)
{
    nlohmann::ordered_json packageMapSpecJson = nlohmann::ordered_json::parse(json, nullptr, true, true);

    for (auto &file : packageMapSpecJson["files"]) {
        PackageMapSpecFile packageMapSpecFile;
        packageMapSpecFile.Name = file["name"].get<std::string>();
        Files.push_back(packageMapSpecFile);
    }

    for (auto &mapFileRef : packageMapSpecJson["mapFileRefs"]) {
        PackageMapSpecMapFileRef packageMapSpecMapFileRef;
        packageMapSpecMapFileRef.File = mapFileRef["file"].get<int32_t>();
        packageMapSpecMapFileRef.Map = mapFileRef["map"].get<int32_t>();
        MapFileRefs.push_back(packageMapSpecMapFileRef);
    }

    for (auto &map : packageMapSpecJson["maps"]) {
        PackageMapSpecMap packageMapSpecMap;
        packageMapSpecMap.Name = map["name"].get<std::string>();
        Maps.push_back(packageMapSpecMap);
    }
}

std::string PackageMapSpec::Dump()
{
    nlohmann::ordered_json json;

    json["files"] = nlohmann::ordered_json::array();
    json["mapFileRefs"] = nlohmann::ordered_json::array();

    for (auto &file : Files)
        json["files"].push_back(nlohmann::ordered_json::object({{"name", file.Name}}));

    for (auto &mapFileRef : MapFileRefs)
        json["mapFileRefs"].push_back(nlohmann::ordered_json::object({{"file", mapFileRef.File}, {"map", mapFileRef.Map}}));

    for (auto &map : Maps)
        json["maps"].push_back(nlohmann::ordered_json::object({{"name", map.Name}}));

    return json.dump(4);
}