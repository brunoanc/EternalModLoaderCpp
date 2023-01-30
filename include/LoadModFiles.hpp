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

#ifndef LOADMODFILES_HPP
#define LOADMODFILES_HPP

#include <map>
#include <atomic>
#include "ResourceContainer.hpp"
#include "SoundContainer.hpp"
#include "StreamDBContainer.hpp"

// Load zipped mods into container list
void LoadZippedMod(std::string zippedMod,
    std::vector<ResourceContainer>& resourceContainerList, std::vector<SoundContainer>& soundContainerList,
    std::vector<StreamDBContainer>& streamDBContainerList, std::vector<std::string>& notFoundContainers);

// Load loose mods into container list
void LoadUnzippedMod(std::string unzippedMod, Mod& globalLooseMod, std::atomic<size_t>& unzippedModCount,
    std::map<size_t, std::vector<ResourceModFile>>& resourceModFiles,
    std::map<size_t, std::vector<SoundModFile>>& soundModFiles,
    std::map<size_t, std::vector<StreamDBModFile>>& streamDBModFiles,
    std::vector<ResourceContainer>& resourceContainerList, std::vector<SoundContainer>& soundContainerList,
    std::vector<StreamDBContainer>& streamDBContainerList, std::vector<std::string>& notFoundContainers);

#endif
