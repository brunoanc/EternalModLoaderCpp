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

#include "jsonxx/jsonxx.h"
#include "Mod/Mod.hpp"

/**
 * @brief Construct a new Mod:: Mod object
 * 
 * @param name Mod's name
 * @param json JSON string to deserialize
 */
Mod::Mod(const std::string &json)
{
    jsonxx::Object modJson;
    modJson.parse(json);

    if (modJson.has<jsonxx::Number>("loadPriority")) {
        LoadPriority = modJson.get<jsonxx::Number>("loadPriority");
    }

    if (modJson.has<jsonxx::Number>("requiredVersion")) {
        RequiredVersion = modJson.get<jsonxx::Number>("requiredVersion");
    }
}
