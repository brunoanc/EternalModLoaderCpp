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

#include <optional>
#include <vector>
#include <cmath>

#ifndef ETERNALMODLOADER_HPP
#define ETERNALMODLOADER_HPP

#include "mmap_allocator/mmappable_vector.h"
#include "AssetsInfo.hpp"
#include "Oodle.hpp"
#include "PackageMapSpec.hpp"

class Mod {
public:
    std::string Name;
    long UncompressedSize;
    std::vector<std::byte> FileBytes;
    bool IsBlangJson;
    bool IsAssetsInfoJson;
    std::optional<class AssetsInfo> AssetsInfo = std::nullopt;
    std::optional<unsigned long> StreamDbHash = std::nullopt;
    std::string ResourceType;
    std::optional<unsigned short> Version = std::nullopt;
    std::optional<std::byte> SpecialByte1 = std::nullopt;
    std::optional<std::byte> SpecialByte2 = std::nullopt;
    std::optional<std::byte> SpecialByte3 = std::nullopt;

    explicit Mod(std::string name)
    {
        Name = name;
    }
};

class SoundMod {
public:
    std::string Name;
    std::vector<std::byte> FileBytes;
};

class ResourceName {
public:
    std::string FullFileName;
    std::string NormalizedFileName;

    ResourceName() {}

    ResourceName(std::string fullFileName, std::string normalizedFileName)
    {
        FullFileName = fullFileName;
        NormalizedFileName = normalizedFileName;
    }
};

class ResourceChunk {
public:
    class ResourceName ResourceName;
    long FileOffset;
    long SizeOffset;
    long SizeZ;
    long Size;
    std::byte CompressionMode;

    ResourceChunk(class ResourceName name, long fileOffset)
    {
        ResourceName = name;
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
    std::vector<ResourceName> NamesList;
    std::vector<ResourceChunk> ChunkList;

    ResourceInfo(std::string name, std::string path)
    {
        Name = name;
        Path = path;
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

    bool ContainsResourceWithName(std::string name)
    {
        for (auto &resourceName : NamesList) {
            if (resourceName.FullFileName == name || resourceName.NormalizedFileName == name)
                return true;
        }

        return false;
    }

    long GetResourceNameId(std::string name)
    {
        for (int i = 0; i < NamesList.size(); i++) {
            if (NamesList[i].FullFileName == name || NamesList[i].NormalizedFileName == name)
                return i;
        }

        return -1;
    }
};

class SoundBankInfo {
public:
    std::string Name;
    std::string Path;
    std::vector<SoundMod> ModList;

    SoundBankInfo(std::string name, std::string path)
    {
        Name = name;
        Path = path;
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

    BlangFile() {}
    BlangFile(std::vector<std::byte> &blangBytes, std::string &resourceName);

    std::vector<std::byte> ToByteVector(std::string &resourceName);
};

class MapAsset {
public:
    int AssetTypeIndex;
    std::string Name;
    int UnknownData1 = 0;
    int UnknownData2 = 0;
    long UnknownData3 = 0;
    long UnknownData4 = 0;
};

class ResourceDataEntry {
public:
    unsigned long StreamDbHash;
    std::byte Version;
    std::byte SpecialByte1;
    std::byte SpecialByte2;
    std::byte SpecialByte3;
    std::string ResourceType;
    std::string MapResourceType;
    std::string MapResourceName;
};

class MapResourcesFile {
public:
    int Magic;
    std::vector<std::string> Layers;
    std::vector<std::string> AssetTypes;
    std::vector<MapAsset> Assets;
    std::vector<std::string> Maps;

    MapResourcesFile() {}
    MapResourcesFile(std::vector<std::byte> &rawData);

    std::vector<std::byte> ToByteVector();
};

inline bool operator==(ResourceChunk& chunk1, const ResourceChunk& chunk2)
{
    if (chunk1.ResourceName.FullFileName == chunk2.ResourceName.FullFileName) {
        return true;
    }
    else {
        return false;
    }
}

inline bool operator==(MapAsset& mapAsset1, const MapAsset& mapAsset2)
{
    if (mapAsset1.Name == mapAsset2.Name) {
        return true;
    }
    else {
        return false;
    }
}

extern const std::string ResourceDataFileName;
extern const std::string PackageMapSpecJsonFileName;
extern std::string BasePath;
extern std::vector<ResourceInfo> ResourceList;
extern std::vector<SoundBankInfo> SoundBankList;
extern bool Verbose;
extern std::map<unsigned long, ResourceDataEntry> ResourceDataMap;

extern std::string RESET;
extern std::string RED;
extern std::string GREEN;
extern std::string YELLOW;
extern std::string BLUE;

extern std::vector<std::string> SupportedFileFormats;

std::string PathToResource(std::string name);
void ReadChunkInfo(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, ResourceInfo &resourceInfo);
ResourceChunk *GetChunk(std::string name, ResourceInfo &resource);
void ReplaceChunks(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, ResourceInfo &resourceInfo);
void AddChunks(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, ResourceInfo &resourceInfo);
void ReadResource(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, ResourceInfo &resourceInfo);
int GetResourceInfo(std::string resourceName);
std::vector<std::byte> IdCrypt(std::vector<std::byte> fileData, std::string internalPath, bool decrypt);
BlangFile ParseBlang(std::vector<std::byte> &blangBytes, std::string &resourceName);
std::vector<std::byte> WriteBlangToVector(BlangFile blangFile, std::string &resourceName);
std::string RemoveWhitespace(std::string &stringWithWhitespace);
std::string ToLower(std::string &str);
std::vector<std::string> SplitString(std::string stringToSplit, char delimiter);
void LoadSoundMods(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, SoundBankInfo &soundBankInfo);
std::map<unsigned long, ResourceDataEntry> ParseResourceData(std::string &filename);
unsigned long CalculateResourceFileNameHash(std::string &input);
std::string NormalizeResourceFilename(std::string filename);
bool EndsWith(const std::string &fullString, const std::string &ending);
std::string PathToSoundBank(std::string name);
int GetSoundBankInfo(std::string soundBankName);

#endif