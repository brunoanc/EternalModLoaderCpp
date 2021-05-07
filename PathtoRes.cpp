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

std::string PathToRes(std::string name, bool &isSnd)
{
    std::string resourcePath;

    if (ToLower(name).find("dlc_hub", 0) == 0) {
        std::string dlcHubFileName = name.substr(4, name.size() - 4);
        resourcePath = "./base/game/dlc/hub/" + dlcHubFileName;
    }
    else if (ToLower(name).find("hub", 0) == 0) {
        resourcePath = "./base/game/hub/" + name;
    }
    else {
        resourcePath = name;
    }

    for (auto &file : std::filesystem::recursive_directory_iterator(BasePath)) {
        std::string path = file.path();
        std::string base_filename = path.substr(path.find_last_of('/') + 1);

        if (base_filename == resourcePath + ".resources" || path == resourcePath + ".resources") {
            isSnd = false;
            return path;
        }
        else if (base_filename == resourcePath + ".snd" || path == resourcePath + ".snd") {
            isSnd = true;
            return path;
        }
    }

    return "";
}