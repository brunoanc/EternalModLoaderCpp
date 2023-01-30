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

#ifndef ASSETSINFO_HPP
#define ASSETSINFO_HPP

#include <string>
#include <vector>

class AssetsInfoLayer
{
public:
    std::string Name;
};

class AssetsInfoMap
{
public:
    std::string Name;
};

class AssetsInfoResource
{
public:
    std::string Name;
    bool Remove{false};
    bool PlaceFirst{false};
    bool PlaceBefore{false};
    std::string PlaceByName;
};

class AssetsInfoAsset
{
public:
    uint64_t StreamDbHash{0};
    std::string ResourceType;
    std::byte Version{0};
    std::string Name;
    std::string MapResourceType;
    bool Remove{false};
    bool PlaceBefore{false};
    std::string PlaceByName;
    std::string PlaceByType;
    std::byte SpecialByte1{0};
    std::byte SpecialByte2{0};
    std::byte SpecialByte3{0};
};

class AssetsInfo
{
public:
    std::vector<AssetsInfoLayer> Layers;
    std::vector<AssetsInfoMap> Maps;
    std::vector<AssetsInfoResource> Resources;
    std::vector<AssetsInfoAsset> Assets;

    AssetsInfo() {}
    AssetsInfo(const std::string& json);
};

#endif
