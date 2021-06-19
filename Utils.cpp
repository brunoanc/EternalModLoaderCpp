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
#include <filesystem>
#include <cstring>

#include "EternalModLoader.hpp"

int32_t GetResourceContainer(std::string &resourceContainerName)
{
    for (int32_t i = 0; i < ResourceContainerList.size(); i++) {
        if (ResourceContainerList[i].Name == resourceContainerName)
            return i;
    }

    return -1;
}

int32_t GetSoundContainer(std::string &soundContainerName)
{
    for (int32_t i = 0; i < SoundContainerList.size(); i++) {
        if (SoundContainerList[i].Name == soundContainerName)
            return i;
    }

    return -1;
}

ResourceChunk *GetChunk(std::string name, ResourceContainer &resourceContainer)
{
    for (auto &chunk : resourceContainer.ChunkList) {
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
    stringWithoutWhitespace.erase(std::remove_if(stringWithoutWhitespace.begin(), stringWithoutWhitespace.end(), [](char ch) { return std::isspace(ch); }), stringWithoutWhitespace.end());

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

bool EndsWith(const std::string &fullString, const std::string &suffix)
{
    if (fullString.length() >= suffix.length()) {
        return 0 == fullString.compare(fullString.length() - suffix.length(), suffix.length(), suffix);
    }
    else {
        return false;
    }
}

bool StartsWith(const std::string &fullString, const std::string &prefix)
{
    return 0 == fullString.rfind(prefix, 0);
}

std::string NormalizeResourceFilename(std::string filename)
{
    if (filename.find_first_of('$') != std::string::npos)
        filename = filename.substr(0, filename.find_first_of('$'));

    if (filename.find_last_of('#') != std::string::npos)
        filename = filename.substr(0, filename.find_last_of('#'));

    if (filename.find_first_of('#') != std::string::npos)
        filename = filename.substr(filename.find_first_of('#'));

    return filename;
}

#ifdef _WIN32
int32_t ResizeMmap(std::byte *&mem, HANDLE &hFile, HANDLE &fileMapping, int64_t newSize)
#else
int32_t ResizeMmap(std::byte *&mem, int32_t &fd, std::string filePath, int64_t oldSize, int64_t newSize)
#endif
{
    try {
#ifdef _WIN32
        UnmapViewOfFile(mem);
        CloseHandle(fileMapping);

        fileMapping = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, *((DWORD*)&newSize + 1), *(DWORD*)&newSize, NULL);

        if (GetLastError() != ERROR_SUCCESS || fileMapping == NULL)
            throw std::exception();

        mem = (std::byte*)MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        if (GetLastError() != ERROR_SUCCESS || mem == NULL)
            throw std::exception();
#else
        munmap(mem, oldSize);
        std::filesystem::resize_file(filePath, newSize);
        mem = (std::byte*)mmap(0, newSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        if (mem == NULL)
            throw std::exception();

        madvise(mem, newSize, MADV_WILLNEED);
#endif
    }
    catch (...) {
        return -1;
    }

    return 0;
}
