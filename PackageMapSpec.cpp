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
#include "EternalModLoader.hpp"

PackageMapSpec::PackageMapSpec(std::string &json)
{
    jsonxx::Object packageMapSpecJson;
    packageMapSpecJson.parse(json);

    PackageMapSpecFile packageMapSpecFile;
    jsonxx::Array files = packageMapSpecJson.get<jsonxx::Array>("files");
    Files.reserve(files.size());

    for (int i = 0; i < files.size(); i++) {
        jsonxx::Object file = files.get<jsonxx::Object>(i);
        packageMapSpecFile.Name = file.get<jsonxx::String>("name");
        Files.push_back(packageMapSpecFile);
    }

    PackageMapSpecMapFileRef packageMapSpecMapFileRef;
    jsonxx::Array mapFileRefs = packageMapSpecJson.get<jsonxx::Array>("mapFileRefs");
    MapFileRefs.reserve(mapFileRefs.size());

    for (int i = 0; i < mapFileRefs.size(); i++) {
        jsonxx::Object mapFileRef = mapFileRefs.get<jsonxx::Object>(i);
        packageMapSpecMapFileRef.File = mapFileRef.get<jsonxx::Number>("file");
        packageMapSpecMapFileRef.Map = mapFileRef.get<jsonxx::Number>("map");
        MapFileRefs.push_back(packageMapSpecMapFileRef);
    }

    PackageMapSpecMap packageMapSpecMap;
    jsonxx::Array maps = packageMapSpecJson.get<jsonxx::Array>("maps");
    Maps.reserve(maps.size());

    for (int i = 0; i < maps.size(); i++) {
        jsonxx::Object map = maps.get<jsonxx::Object>(i);
        packageMapSpecMap.Name = map.get<jsonxx::String>("name");
        Maps.push_back(packageMapSpecMap);
    }
}

std::string PackageMapSpec::Dump()
{
    jsonxx::Object packageMapSpecJson;
    jsonxx::Array files;
    jsonxx::Array mapFileRefs;
    jsonxx::Array maps;

    for (auto &file : Files) {
        jsonxx::Object jsonFile;
        jsonFile << "name" << file.Name;
        files << jsonFile;
    }

    for (auto &mapFileRef : MapFileRefs) {
        jsonxx::Object jsonMapFileRef;
        jsonMapFileRef << "file" << mapFileRef.File;
        jsonMapFileRef << "map" << mapFileRef.Map;
        mapFileRefs << jsonMapFileRef;
    }

    for (auto &map : Maps) {
        jsonxx::Object jsonMap;
        jsonMap << "name" << map.Name;
        maps << jsonMap;
    }

    packageMapSpecJson << "files" << files;
    packageMapSpecJson << "mapFileRefs" << mapFileRefs;
    packageMapSpecJson << "maps" << maps;

    return packageMapSpecJson.json();
}