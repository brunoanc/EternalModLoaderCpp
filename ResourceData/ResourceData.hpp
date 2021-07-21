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

#ifndef RESOURCEDATA_HPP
#define RESOURCEDATA_HPP

/**
 * @brief ResourceData entry class
 * 
 */
class ResourceDataEntry {
public:
    uint64_t StreamDbHash = 0;
    std::string ResourceType;
    std::string MapResourceType;
    std::string MapResourceName;
    std::byte Version = (std::byte)0;
    std::byte SpecialByte1 = (std::byte)0;
    std::byte SpecialByte2 = (std::byte)0;
    std::byte SpecialByte3 = (std::byte)0;
};

std::map<uint64_t, ResourceDataEntry> ParseResourceData(const std::string &fileName);
uint64_t CalculateResourceFileNameHash(const std::string &input);

#endif