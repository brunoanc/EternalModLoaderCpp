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

#ifndef STREAMDBCONTAINER_HPP
#define STREAMDBCONTAINER_HPP

#include <string>
#include <vector>
#include "Mod.hpp"

class StreamDBHeader
{
public:
    uint64_t Magic{7045867521639097680};
    int DataStartOffset{0};
    int Padding0{0};
    int Padding1{0};
    int Padding2{0};
    int NumEntries{0};
    int Flags{3};

    StreamDBHeader() {}
    StreamDBHeader(int dataStartOffset, int numEntries) : DataStartOffset(dataStartOffset), NumEntries(numEntries) {}
};

class StreamDBEntry
{
public:
    uint64_t FileId{0};
    unsigned int DataOffset16{0};
    unsigned int DataLength{0};
    std::string Name;
    std::vector<std::byte> FileData;

    StreamDBEntry(uint64_t fileId, unsigned int dataOffset16, unsigned int dataLength, std::string name, std::vector<std::byte> fileData)
        : FileId(fileId), DataOffset16(dataOffset16), DataLength(dataLength), Name(name), FileData(fileData) {}
};

class StreamDBModFile
{
public:
    Mod Parent;
    std::string Name;
    uint64_t FileId{0};
    std::vector<std::byte> FileData;
    int LODcount{0};
    std::vector<int> LODDataOffset;
    std::vector<int> LODDataLength;
    std::vector<std::vector<std::byte>> LODFileData;

    StreamDBModFile(Mod parent, std::string name) : Parent(parent), Name(name) {}
};

class StreamDBContainer
{
public:
    std::string Name;
    std::string Path;
    StreamDBHeader Header;
    std::vector<StreamDBModFile> ModFiles;
    std::vector<StreamDBEntry> StreamDBEntries;

    StreamDBContainer(std::string name, std::string path) : Name(name), Path(path) {}
};

#endif
