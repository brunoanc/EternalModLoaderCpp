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

#ifndef PACKAGEMAPSPEC_HPP
#define PACKAGEMAPSPEC_HPP

#include <string>
#include <vector>

class PackageMapSpecFile
{
public:
    std::string Name;
};

class PackageMapSpecMapFileRef
{
public:
    int File{-1};
    int Map{-1};

    PackageMapSpecMapFileRef() {}
    PackageMapSpecMapFileRef(int file, int map) : File(file), Map(map) {}
};

class PackageMapSpecMap
{
public:
    std::string Name;
};

class PackageMapSpec
{
public:
    std::vector<PackageMapSpecFile> Files;
    std::vector<PackageMapSpecMapFileRef> MapFileRefs;
    std::vector<PackageMapSpecMap> Maps;

    PackageMapSpec() {}
    PackageMapSpec(const std::string& json);

    std::string Dump() const;
};

#endif
