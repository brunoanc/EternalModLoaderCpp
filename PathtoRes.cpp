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

std::string PathToResource(std::string name)
{
    std::string searchPath = BasePath;
    std::string resourcePath;
    bool recursive = true;

    if (ToLower(name).find("dlc_hub", 0) == 0) {
        std::string dlcHubFileName = name.substr(4, name.size() - 4);
        resourcePath = BasePath + "game/dlc/hub/" + dlcHubFileName;
    }
    else if (ToLower(name).find("hub", 0) == 0) {
        resourcePath = BasePath + "game/hub/" + name;
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
            searchPath = BasePath + "game/";
        }
    }

    if (recursive) {
        for (auto &file : std::filesystem::recursive_directory_iterator(searchPath)) {
            if (std::filesystem::equivalent(file.path(), resourcePath) || file.path().filename() == resourcePath)
                return file.path().string();
        }
    }
    else {
        for (auto &file : std::filesystem::directory_iterator(searchPath)) {
            if (std::filesystem::equivalent(file.path(), resourcePath) || file.path().filename() == resourcePath)
                return file.path().string();
        }
    }

    return "";
}

std::string PathToSoundBank(std::string name)
{
    std::string searchPath = BasePath + "sound/soundbanks/pc";
    std::string searchPattern = name + ".snd";

    for (auto &file : std::filesystem::recursive_directory_iterator(searchPath)) {
        if (std::filesystem::equivalent(file.path(), searchPattern) || file.path().filename() == searchPattern)
            return file.path().string();
    }

    return "";
}