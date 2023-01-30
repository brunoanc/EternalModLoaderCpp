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

#ifndef BLANGFILEENTRY_HPP
#define BLANGFILEENTRY_HPP

#include "BlangFile.hpp"
#include "ResourceContainer.hpp"

class BlangFileEntry
{
public:
    class BlangFile BlangFile;
    ResourceChunk Chunk;
    bool WasModified{false};
    bool Announce{false};

    BlangFileEntry() {}
    BlangFileEntry(class BlangFile blangFile, ResourceChunk chunk) : BlangFile(blangFile), Chunk(chunk) {}
};

#endif
