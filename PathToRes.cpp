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
#include <algorithm>
#include "EternalModLoader.hpp"

std::vector<fs::path> ResourceContainerPathList;

/**
 * @brief Get the resource container paths
 * 
 */
void GetResourceContainerPathList()
{
    for (auto &file : fs::recursive_directory_iterator(BasePath + "game" + Separator)) {
        if (file.path().extension().string() == ".resources") {
            ResourceContainerPathList.push_back(file.path());
        }
    }
}

/**
 * @brief Get the path to the resource container
 * 
 * @param name Name of the resource container to find
 * @return Path to the resource container, or an empty string if not found
 */
std::string PathToResourceContainer(const std::string &name)
{
    // Check resource filename
    if (StartsWith(name, "dlc_hub")) {
        // dlc hub, remove the "dlc_" prefix, build the path and return it
        std::string resourcePath = name.substr(4, name.size() - 4);
        resourcePath = BasePath + "game" + Separator + "dlc" + Separator + "hub" + Separator + resourcePath;
        return fs::is_regular_file(resourcePath) ? resourcePath : "";
    }
    else if (StartsWith(name, "hub")) {
        // Regular hub, build the path and return it
        std::string resourcePath = BasePath + "game" + Separator + "hub" + Separator + name;
        return fs::is_regular_file(resourcePath) ? resourcePath : "";
    }
    else if (StartsWith(name, "gameresources")
        || StartsWith(name, "warehouse")
        || StartsWith(name, "meta")) {
            // Resource located in /base/, build path and return it
            return fs::is_regular_file(BasePath + name) ? (BasePath + name) : "";
    }

    // Find resource iterating through directories
    std::string searchPath = BasePath + "game" + Separator;

    if (fs::is_regular_file(searchPath + name)) {
        return searchPath + name;
    }

    for (auto &file : ResourceContainerPathList) {
        if (file.filename().string() == name) {
            return file.string();
        }
    }

    return "";
}

/**
 * @brief Get the path to the sound container
 * 
 * @param name Name of the sound container to find
 * @return Path to the sound container, or an empty string if not found
 */
std::string PathToSoundContainer(const std::string &name)
{
    // Assemble snd path and return it
    std::string sndPath = BasePath + "sound" + Separator + "soundbanks" + Separator + "pc" + Separator + name + ".snd";
    return fs::is_regular_file(sndPath) ? sndPath : "";
}
