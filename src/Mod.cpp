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

#include "Mod.hpp"
#include "jsonxx/jsonxx.h"

Mod::Mod(const std::string& json)
{
    // Mod JSON object
    jsonxx::Object modJson;
    modJson.parse(json);

    // Get loadPriority property
    if (modJson.has<jsonxx::Number>("loadPriority")) {
        LoadPriority = modJson.get<jsonxx::Number>("loadPriority");
    }

    // Get requiredVersion property
    if (modJson.has<jsonxx::Number>("requiredVersion")) {
        RequiredVersion = modJson.get<jsonxx::Number>("requiredVersion");
    }
}
