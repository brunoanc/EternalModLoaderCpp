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

#include "GetObject.hpp"

ssize_t GetResourceContainer(const std::string& resourceContainerName, std::vector<ResourceContainer>& resourceContainerList)
{
    for (size_t i = 0; i < resourceContainerList.size(); i++) {
        if (resourceContainerList[i].Name == resourceContainerName) {
            return i;
        }
    }

    return -1;
}

ssize_t GetSoundContainer(const std::string& soundContainerName, std::vector<SoundContainer>& soundContainerList)
{
    for (size_t i = 0; i < soundContainerList.size(); i++) {
        if (soundContainerList[i].Name == soundContainerName) {
            return i;
        }
    }

    return -1;
}

ResourceChunk *GetChunk(const std::string name, ResourceContainer& resourceContainer)
{
    for (auto& chunk : resourceContainer.ChunkList) {
        if (chunk.ResourceName.FullFileName == name
            || chunk.ResourceName.NormalizedFileName == name) {
                return &chunk;
        }
    }

    return nullptr;
}
