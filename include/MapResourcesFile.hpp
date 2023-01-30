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

#ifndef MAPRESOURCESFILE_HPP
#define MAPRESOURCESFILE_HPP

#include <string>
#include <vector>

class MapAsset
{
public:
    int AssetTypeIndex;
    std::string Name;
    unsigned int UnknownData1{0};
    unsigned int UnknownData2{0};
    uint64_t UnknownData3{0};
    uint64_t UnknownData4{0};

    bool operator==(const MapAsset& mapAsset)
    {
        return Name == mapAsset.Name;
    }
};

class MapResourcesFile
{
public:
    int Magic{0};
    std::vector<std::string> Layers;
    std::vector<std::string> AssetTypes;
    std::vector<MapAsset> Assets;
    std::vector<std::string> Maps;

    MapResourcesFile() {}
    MapResourcesFile(std::vector<std::byte>& rawData);

    std::vector<std::byte> ToByteVector();
};

#endif
