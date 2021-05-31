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
#include <vector>
#include <algorithm>
#include <cstring>

#include "EternalModLoader.hpp"

int GetResourceInfo(std::string resourceName)
{
    for (int i = 0; i < ResourceList.size(); i++) {
        if (ResourceList[i].Name == resourceName)
            return i;
    }

    return -1;
}

int GetSoundBankInfo(std::string soundBankName)
{
    for (int i = 0; i < SoundBankList.size(); i++) {
        if (SoundBankList[i].Name == soundBankName)
            return i;
    }

    return -1;
}

ResourceChunk *GetChunk(std::string name, ResourceInfo &resource)
{
    for (auto &chunk : resource.ChunkList) {
        if (chunk.ResourceName.FullFileName == name
            || chunk.ResourceName.NormalizedFileName == name) {
                return &chunk;
        }
    }

    return NULL;
}

std::string RemoveWhitespace(std::string &stringWithWhitespace)
{
    std::string stringWithoutWhitespace = stringWithWhitespace;
    stringWithoutWhitespace.erase(std::remove_if(stringWithoutWhitespace.begin(), stringWithoutWhitespace.end(), [](char ch) { return std::isspace<char>(ch, std::locale::classic()); }), stringWithoutWhitespace.end());

    return stringWithoutWhitespace;
}

std::string ToLower(std::string &str)
{
    std::string lowercase = str;
    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), [](unsigned char c) { return std::tolower(c); });
    
    return lowercase;
}

std::vector<std::string> SplitString(std::string stringToSplit, char delimiter)
{
    std::vector<std::string> resultVector;
    size_t pos;
    std::string part;

    while ((pos = stringToSplit.find(delimiter)) != std::string::npos) {
        part = stringToSplit.substr(0, pos);
        resultVector.push_back(part);
        stringToSplit.erase(0, pos + 1);
    }

    resultVector.push_back(stringToSplit);

    return resultVector;
}

bool EndsWith(const std::string &fullString, const std::string &ending)
{
    if (fullString.length() >= ending.length()) {
        return 0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending);
    }
    else {
        return false;
    }
}

std::string NormalizeResourceFilename(std::string filename)
{
    if (filename.find_first_of('$') != std::string::npos)
        filename = filename.substr(0, filename.find_first_of('$'));

    if (filename.find_last_of('#') != std::string::npos)
        filename = filename.substr(0, filename.find_last_of('#'));

    if (filename.find_first_of('#') != std::string::npos)
        filename = filename.substr(0, filename.find_first_of('#'));

    return filename;
}