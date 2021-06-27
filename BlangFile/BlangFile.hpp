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

#ifndef BLANGFILE_HPP
#define BLANGFILE_HPP

/**
 * @brief Blang file string class
 * 
 */
class BlangString {
public:
    uint32_t Hash = 0;
    std::string Identifier;
    std::string Text;
    std::vector<std::byte> Unknown;

    BlangString()
    {
        Identifier = "";
        Text = "";
    }

    BlangString(uint32_t hash, std::string identifier, std::string text, std::vector<std::byte> unknown)
    {
        Hash = hash;
        Identifier = identifier;
        Text = text;
        Unknown = unknown;
    }
};

/**
 * @brief Blang file class
 * 
 */
class BlangFile {
public:
    int64_t UnknownData = 0;
    std::vector<BlangString> Strings;

    BlangFile() {}
    BlangFile(std::vector<std::byte> &blangBytes);

    std::vector<std::byte> ToByteVector();
};

#endif