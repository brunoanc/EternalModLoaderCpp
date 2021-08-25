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
#include <filesystem>

#include "EternalModLoader.hpp"

std::vector<std::filesystem::path> ResourceContainerPathList;

/**
 * @brief Get the resource container paths
 * 
 */
void GetResourceContainerPathList()
{
    for (auto &file : std::filesystem::recursive_directory_iterator(BasePath + "game" + Separator)) {
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
    std::string searchPath = BasePath;
    bool recursive = true;

    if (StartsWith(ToLower(name), "dlc_hub")) {
        std::string resourcePath = name.substr(4, name.size() - 4);
        resourcePath = BasePath + "game" + Separator + "dlc" + Separator + "hub" + Separator + resourcePath;
        return std::filesystem::is_regular_file(searchPath + resourcePath) ? resourcePath : "";
    }
    else if (StartsWith(ToLower(name), "hub")) {
        std::string resourcePath = BasePath + "game" + Separator + "hub" + Separator + name;
        return std::filesystem::is_regular_file(searchPath + resourcePath) ? resourcePath : "";
    }
    else {
        if (name.find("gameresources") != std::string::npos
            || name.find("warehouse") != std::string::npos
            || name.find("meta") != std::string::npos
            || name.find(".streamdb") != std::string::npos) {
                recursive = false;
        }
        else {
            searchPath = BasePath + "game" + Separator;
        }
    }

    if (std::filesystem::is_regular_file(searchPath + name)) {
        return searchPath + name;
    }

    if (recursive) {
        for (auto &file : ResourceContainerPathList) {
            if (file.filename().string() == name) {
                return file;
            }
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
std::string PathToSoundContainer(const std::string name)
{
    std::string searchPath = BasePath + "sound" + Separator + "soundbanks" + Separator + "pc" + Separator;
    std::string sndPath = name + ".snd";

    if (std::filesystem::is_regular_file(searchPath + sndPath)) {
        return searchPath + sndPath;
    }

    return "";
}
