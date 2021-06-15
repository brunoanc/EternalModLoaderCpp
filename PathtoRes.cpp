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

std::string PathToResourceContainer(std::string name)
{
    std::string searchPath = BasePath;
    std::string resourcePath = name;
    bool recursive = true;

    if (StartsWith(ToLower(name), "dlc_hub")) {
        resourcePath = resourcePath.substr(4, name.size() - 4);
        return BasePath + "game" + separator + "dlc" + separator + "hub" + separator + resourcePath;
    }
    else if (StartsWith(ToLower(name), "hub")) {
        return BasePath + "game" + separator + "hub" + separator + resourcePath;
    }
    else {
        resourcePath = name;

        if (resourcePath.find("gameresources") != std::string::npos
            || resourcePath.find("warehouse") != std::string::npos
            || resourcePath.find("meta") != std::string::npos
            || resourcePath.find(".streamdb") != std::string::npos) {
                recursive = false;
        }
        else {
            searchPath = BasePath + "game" + separator;
        }
    }

    if (std::filesystem::is_regular_file(searchPath + resourcePath)) {
        return searchPath + resourcePath;
    }

    if (recursive) {
        for (auto &file : std::filesystem::recursive_directory_iterator(searchPath)) {
            if (file.path().filename() == resourcePath)
                return file.path().string();
        }
    }

    return "";
}

std::string PathToSoundContainer(std::string name)
{
    std::string searchPath = BasePath + "sound" + separator + "soundbanks" + separator + "pc" + separator;
    std::string sndPath = name + ".snd";

    if (std::filesystem::is_regular_file(searchPath + sndPath))
        return searchPath + sndPath;

    return "";
}