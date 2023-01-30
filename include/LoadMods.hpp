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

#ifndef LOADMODS_HPP
#define LOADMODS_HPP

#include <memory>
#include "ResourceContainer.hpp"
#include "ResourceData.hpp"
#include "SoundContainer.hpp"
#include "StreamDBContainer.hpp"

// String stream output operations
void InitStringStreams(size_t count);
void OutputStringStream(int index);

// Load mods
void LoadResourceMods(ResourceContainer& resourceContainer,
    std::map<uint64_t, ResourceDataEntry>& resourceDataMap,
    std::unique_ptr<std::byte[]>& buffer, int bufferSize);
void LoadSoundMods(SoundContainer& soundContainer);
void LoadStreamDBMods(StreamDBContainer& streamDBContainer, std::vector<StreamDBContainer>& streamDBContainerList);

#endif
