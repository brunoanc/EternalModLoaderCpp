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
#include <filesystem>
#include <sstream>
#include <cstring>
#include "Colors.hpp"
#include "ProgramOptions.hpp"

namespace fs = std::filesystem;

std::stringstream ProgramOptions::GetProgramOptions(char **arguments, int count)
{
    std::stringstream output;

    // Get base path
    BasePath = std::string(arguments[1]) + SEPARATOR + "base" + SEPARATOR;

    if (!fs::exists(BasePath)) {
        std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Game directory does not exist!" << std::endl;
        BasePath = "";
        return output;
    }

    // Check arguments passed to program
    if (count > 2) {
        for (int i = 2; i < count; i++) {
            if (!strcmp(arguments[i], "--list-res")) {
                ListResources = true;
            }
            else if (!strcmp(arguments[i], "--verbose")) {
                Verbose = true;
                output << Colors::Yellow << "INFO: Verbose logging is enabled." << Colors::Reset << '\n';
            }
            else if (!strcmp(arguments[i], "--slow")) {
                SlowMode = true;
                output << Colors::Yellow << "INFO: Slow mod loading mode is enabled." << Colors::Reset << '\n';
            }
            else if (!strcmp(arguments[i], "--online-safe")) {
                LoadOnlineSafeModsOnly = true;
                output << Colors::Yellow << "INFO: Only online-safe mods will be loaded." << Colors::Reset << '\n';
            }
            else if (!strcmp(arguments[i], "--compress-textures")) {
                CompressTextures = true;
                output << Colors::Yellow << "INFO: Texture compression is enabled." << Colors::Reset << '\n';
            }
            else if (!strcmp(arguments[i], "--disable-multithreading")) {
                MultiThreading = false;
                output << Colors::Yellow << "INFO: Multi-threading is disabled." << Colors::Reset << '\n';
            }
            else {
                output << Colors::Red << "WARNING: " << Colors::Reset << "Unknown argument: " << arguments[i] << '\n';
            }
        }
    }

    return output;
}
