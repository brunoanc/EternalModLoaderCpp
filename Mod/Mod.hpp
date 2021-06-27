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

#ifndef MOD_HPP
#define MOD_HPP

/**
 * @brief Mod class
 * 
 */
class Mod {
public:
    std::string Name;
    std::string Author;
    std::string Description;
    std::string Version;
    int32_t LoadPriority = 0;
    int32_t RequiredVersion = 0;

    Mod() {}
    Mod(std::string name);
    Mod(std::string name, std::string &json);
};

#endif