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
        if (ResourceList[i].Name == resourceName) {
            return i;
        }
    }

    return -1;
}

int GetChunk(std::string name, int resourceIndex)
{
    for (int i = 0; i < ResourceList[resourceIndex].ChunkList.size(); i++) {
        if (ResourceList[resourceIndex].ChunkList[i].Name.find(name) != std::string::npos) {
            return i;
        }
    }

    return -1;
}

std::string RemoveWhitespace(std::string& stringWithWhitespace)
{
    std::string stringWithoutWhitespace = stringWithWhitespace;
    stringWithoutWhitespace.erase(std::remove_if(stringWithoutWhitespace.begin(), stringWithoutWhitespace.end(), [](char ch) { return std::isspace<char>(ch, std::locale::classic()); }), stringWithoutWhitespace.end());
    return stringWithoutWhitespace;
}

std::string ToLower(std::string& str)
{
    std::string lowercase = str;
    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), [](unsigned char c) { return std::tolower(c); });
    return lowercase;
}

std::vector<std::byte> VectorIntegralAdd(std::vector<std::byte>& vect, int numberOfBytes, long numberToAdd)
{
    long a;
    std::copy(vect.begin(), vect.begin() + numberOfBytes, (std::byte*)&a);
    a += numberToAdd;

    std::vector<std::byte> vectorResult(numberOfBytes);
    std::copy((std::byte*)&a, (std::byte*)&a + numberOfBytes, vectorResult.begin());

    return vectorResult;
}

long VectorToNumber(std::vector<std::byte>& vect, int numberOfBytes)
{
    long a;
    std::copy(vect.begin(), vect.begin() + numberOfBytes, (std::byte*)&a);
    return a;
}

std::vector<std::byte> LongToVector(long number, int numberOfBytes)
{
    std::vector<std::byte> vectorResult(numberOfBytes);
    std::copy((std::byte*)&number, (std::byte*)&number + numberOfBytes, vectorResult.begin());
    return vectorResult;
}