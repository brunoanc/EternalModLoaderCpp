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

/**
 * @brief MapResources file map asset class
 * 
 */
class MapAsset {
public:
    int32_t AssetTypeIndex;
    std::string Name;
    int32_t UnknownData1 = 0;
    int32_t UnknownData2 = 0;
    int64_t UnknownData3 = 0;
    int64_t UnknownData4 = 0;
};

/**
 * @brief MapResources file class
 * 
 */
class MapResourcesFile {
public:
    int32_t Magic = 0;
    std::vector<std::string> Layers;
    std::vector<std::string> AssetTypes;
    std::vector<MapAsset> Assets;
    std::vector<std::string> Maps;

    MapResourcesFile() {}
    MapResourcesFile(std::vector<std::byte> &rawData);

    std::vector<std::byte> ToByteVector() const;
};

/**
 * @brief Equality operator for MapAsset objects
 * 
 * @param mapAsset1 First MapAsset object
 * @param mapAsset2 Second MapAsset object
 * @return True if equal, false otherwise
 */
inline bool operator==(MapAsset& mapAsset1, const MapAsset& mapAsset2)
{
    return mapAsset1.Name == mapAsset2.Name;
}

#endif