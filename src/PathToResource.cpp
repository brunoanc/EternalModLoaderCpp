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

#include <vector>
#include <algorithm>
#include <filesystem>
#include "ProgramOptions.hpp"
#include "Utils.hpp"
#include "PathToResource.hpp"

namespace fs = std::filesystem;

std::vector<fs::path> ResourceContainerPathList;

void GetResourceContainerPathList()
{
    for (auto& file : fs::recursive_directory_iterator(ProgramOptions::BasePath + "game" + SEPARATOR)) {
        if (file.path().extension().string() == ".resources") {
            ResourceContainerPathList.push_back(file.path());
        }
    }
}

std::string PathToResourceContainer(const std::string& name)
{
    // Check resource filename
    if (StartsWith(name, "dlc_hub")) {
        // dlc hub, remove the "dlc_" prefix, build the path and return it
        std::string resourcePath = name.substr(4, name.size() - 4);
        resourcePath = ProgramOptions::BasePath + "game" + SEPARATOR + "dlc" + SEPARATOR + "hub" + SEPARATOR + resourcePath;
        return fs::is_regular_file(resourcePath) ? resourcePath : "";
    }
    else if (StartsWith(name, "hub")) {
        // Regular hub, build the path and return it
        std::string resourcePath = ProgramOptions::BasePath + "game" + SEPARATOR + "hub" + SEPARATOR + name;
        return fs::is_regular_file(resourcePath) ? resourcePath : "";
    }
    else if (StartsWith(name, "gameresources")
        || StartsWith(name, "warehouse")
        || StartsWith(name, "meta")) {
            // Resource located in /base/, build path and return it
            return fs::is_regular_file(ProgramOptions::BasePath + name) ? (ProgramOptions::BasePath + name) : "";
    }

    // Find resource iterating through directories
    std::string searchPath = ProgramOptions::BasePath + "game" + SEPARATOR;

    if (fs::is_regular_file(searchPath + name)) {
        return searchPath + name;
    }

    for (auto& file : ResourceContainerPathList) {
        if (file.filename().string() == name) {
            return file.string();
        }
    }

    return "";
}

std::string PathToSoundContainer(const std::string& name)
{
    // Assemble snd path and return it
    std::string sndPath = ProgramOptions::BasePath + "sound" + SEPARATOR + "soundbanks" + SEPARATOR + "pc" + SEPARATOR + name + ".snd";
    return fs::is_regular_file(sndPath) ? sndPath : "";
}
