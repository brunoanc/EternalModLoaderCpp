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

#include <algorithm>
#include <filesystem>
#include "Utils.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <sys/statvfs.h>
#endif

namespace fs = std::filesystem;

std::string RemoveWhitespace(const std::string& stringWithWhitespace)
{
    std::string stringWithoutWhitespace = stringWithWhitespace;
    stringWithoutWhitespace.erase(std::remove_if(stringWithoutWhitespace.begin(), stringWithoutWhitespace.end(), [](char ch) { return std::isspace(ch); }), stringWithoutWhitespace.end());

    return stringWithoutWhitespace;
}

std::string ToLower(const std::string& str)
{
    std::string lowercase = str;
    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), [](unsigned char c) { return std::tolower(c); });

    return lowercase;
}

std::vector<std::string> SplitString(std::string stringToSplit, const char delimiter)
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

bool EndsWith(const std::string& fullString, const std::string& suffix)
{
    if (fullString.length() >= suffix.length()) {
        return 0 == fullString.compare(fullString.length() - suffix.length(), suffix.length(), suffix);
    }
    else {
        return false;
    }
}

bool StartsWith(const std::string& fullString, const std::string& prefix)
{
    return 0 == fullString.rfind(prefix, 0);
}

std::string NormalizeResourceFilename(std::string filename)
{
    if (filename.find_first_of('$') != std::string::npos) {
        filename = filename.substr(0, filename.find_first_of('$'));
    }

    if (filename.find_last_of('#') != std::string::npos) {
        filename = filename.substr(0, filename.find_last_of('#'));
    }

    if (filename.find_first_of('#') != std::string::npos) {
        filename = filename.substr(filename.find_first_of('#'));
    }

    return filename;
}

int GetClusterSize()
{
#ifdef _WIN32
    DWORD sectorsPerCluster;
    DWORD bytesPerSector;
    DWORD numberOfFreeClusters;
    DWORD totalNumberOfClusters;
    bool result = GetDiskFreeSpaceA(nullptr, &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters);

    return result ? sectorsPerCluster * bytesPerSector : -1;
#else
    struct statvfs diskInfo;
    return statvfs(fs::current_path().c_str(), &diskInfo) == 0 ? diskInfo.f_bsize : -1;
#endif
}
