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
    for (auto& file : fs::recursive_directory_iterator(ProgramOptions::BasePath + "game" + fs::path::preferred_separator)) {
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
        resourcePath = ProgramOptions::BasePath + "game" + fs::path::preferred_separator + "dlc" + fs::path::preferred_separator + "hub" + fs::path::preferred_separator + resourcePath;
        return fs::is_regular_file(resourcePath) ? resourcePath : "";
    }
    else if (StartsWith(name, "hub")) {
        // Regular hub, build the path and return it
        std::string resourcePath = ProgramOptions::BasePath + "game" + fs::path::preferred_separator + "hub" + fs::path::preferred_separator + name;
        return fs::is_regular_file(resourcePath) ? resourcePath : "";
    }
    else if (StartsWith(name, "gameresources")
        || StartsWith(name, "warehouse")
        || StartsWith(name, "meta")) {
            // Resource located in /base/, build path and return it
            return fs::is_regular_file(ProgramOptions::BasePath + name) ? (ProgramOptions::BasePath + name) : "";
    }

    // Find resource iterating through directories
    std::string searchPath = ProgramOptions::BasePath + "game" + fs::path::preferred_separator;

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
    std::string sndPath = ProgramOptions::BasePath + "sound" + fs::path::preferred_separator + "soundbanks" + fs::path::preferred_separator + "pc" + fs::path::preferred_separator + name + ".snd";
    return fs::is_regular_file(sndPath) ? sndPath : "";
}
