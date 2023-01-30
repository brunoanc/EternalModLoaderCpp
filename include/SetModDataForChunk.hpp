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

#ifndef SETMODDATAFORCHUNK_HPP
#define SETMODDATAFORCHUNK_HPP

#include <memory>
#include "MemoryMappedFile.hpp"
#include "ResourceContainer.hpp"

// Write mod into chunk
bool SetModDataForChunk(
    MemoryMappedFile& memoryMappedFile,
    ResourceContainer& resourceContainer,
    ResourceChunk& chunk,
    ResourceModFile& modFile,
    const uint64_t compressedSize,
    const uint64_t uncompressedSize,
    const std::byte *compressionMode,
    std::unique_ptr<std::byte[]>& buffer,
    int bufferSize);

#endif
