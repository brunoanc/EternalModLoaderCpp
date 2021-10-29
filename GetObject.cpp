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

#include <iostream>

#include "EternalModLoader.hpp"

/**
 * @brief Get the ResourceContainer object
 * 
 * @param resourceContainerName Name of the resource container to find
 * @return Index of the resource container, or -1 if not found
 */
int32_t GetResourceContainer(const std::string &resourceContainerName)
{
    for (int32_t i = 0; i < ResourceContainerList.size(); i++) {
        if (ResourceContainerList[i].Name == resourceContainerName) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Get the SoundContainer object
 * 
 * @param soundContainerName Name of the sound container to find
 * @return Index of the sound container, or -1 if not found
 */
int32_t GetSoundContainer(const std::string &soundContainerName)
{
    for (int32_t i = 0; i < SoundContainerList.size(); i++) {
        if (SoundContainerList[i].Name == soundContainerName) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Get the ResourceChunk object
 * 
 * @param name Name of the resource chunk to find
 * @param resourceContainer ResourceContainer object containing the resource to search in
 * @return Pointer to the ResourceChunk object, or nullptr if not found 
 */
ResourceChunk *GetChunk(const std::string name, ResourceContainer &resourceContainer)
{
    for (auto &chunk : resourceContainer.ChunkList) {
        if (chunk.ResourceName.FullFileName == name
            || chunk.ResourceName.NormalizedFileName == name) {
                return &chunk;
        }
    }

    return nullptr;
}
