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

#ifndef REPLACECHUNKS_HPP
#define REPLACECHUNKS_HPP

#include <memory>
#include "MemoryMappedFile.hpp"
#include "ResourceContainer.hpp"
#include "ResourceData.hpp"

// Replace chunks with mods in resource file
void ReplaceChunks(MemoryMappedFile& memoryMappedFile, ResourceContainer& resourceContainer,
    std::map<uint64_t, ResourceDataEntry>& resourceDataMap, std::stringstream& os,
    std::unique_ptr<std::byte[]>& buffer, int bufferSize);

#endif
