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

/**
 * @brief PackageMapSpec File class for deserialization
 * 
 */
class PackageMapSpecFile {
public:
    std::string Name;
};

/**
 * @brief PackageMapSpec MapFileRef class for deserialization
 * 
 */
class PackageMapSpecMapFileRef {
public:
    int32_t File;
    int32_t Map;

    /**
     * @brief Construct a new PackageMapSpecMapFileRef object
     * 
     */
    PackageMapSpecMapFileRef()
    {
        File = -1;
        Map = -1;
    }

    /**
     * @brief Construct a new Package MapSpecMapFileRef object
     * 
     * @param file File index
     * @param map Map index
     */
    PackageMapSpecMapFileRef(int32_t file, int32_t map)
    {
        File = file;
        Map = map;
    }
};

/**
 * @brief PackageMapSpec Map class for deserialization
 * 
 */
class PackageMapSpecMap {
public:
    std::string Name;
};

/**
 * @brief PackageMapSpec JSON class for deserialization
 * 
 */
class PackageMapSpec {
public:
    std::vector<PackageMapSpecFile> Files;
    std::vector<PackageMapSpecMapFileRef> MapFileRefs;
    std::vector<PackageMapSpecMap> Maps;

    PackageMapSpec() {}
    PackageMapSpec(const std::string &json);

    std::string Dump() const;
};

#endif
