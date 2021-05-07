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

#ifndef ETERNALMODLOADER_HPP
#define ETERNALMODLOADER_HPP

#include <vector>

#include "mmap_allocator/mmappable_vector.h"

class Mod {
public:
    std::string Name;
    std::vector<std::byte> FileBytes;
    bool IsBlangJson;
    bool IsSound;

    explicit Mod(std::string name)
    {
        Name = name;
    }
};

class ResourceChunk {
public:
    std::string Name;
    long NameId;
    long FileOffset;
    long SizeOffset;
    long SizeZ;
    long Size;
    std::byte CompressionMode;

    ResourceChunk(std::string name, long fileOffset)
    {
        Name = name;
        NameId = 0;
        FileOffset = fileOffset;
        SizeOffset = 0;
        SizeZ = 0;
        Size = 0;
        CompressionMode = (std::byte)(0);
    }
};

class ResourceInfo {
public:
    std::string Name;
    std::string Path;
    bool IsSnd;
    int FileCount;
    int TypeCount;
    int StringsSize;
    long NamesOffset;
    long InfoOffset;
    long Dummy7Offset;
    long DataOffset;
    long IdclOffset;
    int UnknownCount;
    int FileCount2;
    long NamesOffsetEnd;
    long UnknownOffset;
    long UnknownOffset2;
    std::vector<Mod> ModList;
    std::vector<Mod> ModListNew;
    std::vector<std::string> NamesList;
    std::vector<ResourceChunk> ChunkList;

    ResourceInfo(std::string name, std::string path)
    {
        Name = name;
        Path = path;
        IsSnd = false;
        FileCount = 0;
        TypeCount = 0;
        StringsSize = 0;
        NamesOffset = 0;
        InfoOffset = 0;
        Dummy7Offset = 0;
        DataOffset = 0;
        IdclOffset = 0;
        UnknownCount = 0;
        FileCount2 = 0;
        NamesOffsetEnd = 0;
        UnknownOffset = 0;
        UnknownOffset2 = 0;
    }
};

class BlangString {
public:
    unsigned int Hash;
    std::string Identifier;
    std::string Text;
    std::vector<std::byte> Unknown;

    BlangString()
    {
        Hash = 0;
        Identifier = "";
        Text = "";
    }

    BlangString(unsigned int hash, std::string identifier, std::string text, std::vector<std::byte> unknown)
    {
        Hash = hash;
        Identifier = identifier;
        Text = text;
        Unknown = unknown;
    }
};

class BlangFile {
public:
    long UnknownData;
    std::vector<BlangString> Strings;
};

inline bool operator==(ResourceChunk& chunk1, const ResourceChunk& chunk2)
{
    if (chunk1.Name == chunk2.Name)
        return true;
    else
        return false;
}

extern std::string BasePath;
extern std::vector<ResourceInfo> ResourceList;

extern std::string RESET;
extern std::string RED;
extern std::string GREEN;
extern std::string YELLOW;
extern std::string BLUE;

std::string PathToRes(std::string name, bool &isSnd);
void ReadChunkInfo(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, int resourceIndex);
int GetChunk(std::string name, int resourceIndex);
void ReplaceChunks(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, int resourceIndex);
void AddChunks(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, int resourceIndex);
void ReadResource(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, int resourceIndex);
int GetResourceInfo(std::string resourceName);
std::vector<std::byte> IdCrypt(std::vector<std::byte> fileData, std::string internalPath, bool decrypt);
BlangFile ParseBlang(std::vector<std::byte> &blangBytes, std::string &resourceName);
std::vector<std::byte> WriteBlangToVector(BlangFile blangFile, std::string &resourceName);
std::string RemoveWhitespace(std::string &stringWithWhitespace);
std::string ToLower(std::string &str);
std::vector<std::string> SplitString(std::string stringToSplit, char delimiter);
int LoadSoundMods(std::vector<std::byte> &soundBytes, std::string sndPath, std::string soundFilename);

#endif