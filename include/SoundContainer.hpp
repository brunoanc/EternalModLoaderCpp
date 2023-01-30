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

#ifndef SOUNDCONTAINER_HPP
#define SOUNDCONTAINER_HPP

#include <string>
#include <vector>
#include "Mod.hpp"

class SoundModFile
{
public:
    Mod Parent;
    std::string Name;
    std::vector<std::byte> FileBytes;

    SoundModFile(Mod parent, std::string name) : Parent(parent), Name(name) {}
};

class SoundEntry
{
public:
    unsigned int SoundId{0};
    size_t InfoOffset{0};

    SoundEntry(unsigned int soundId, size_t infoOffset) : SoundId(soundId), InfoOffset(infoOffset) {}
};

class SoundContainer
{
public:
    std::string Name;
    std::string Path;
    std::vector<SoundModFile> ModFileList;
    std::vector<SoundEntry> SoundEntries;

    SoundContainer(std::string name, std::string path) : Name(name), Path(path) {}
};

#endif
