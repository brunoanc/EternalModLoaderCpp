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

#include <map>
#include <optional>
#include <vector>
#include <cmath>
#include <cstdint>

#include "AssetsInfo.hpp"
#include "Oodle.hpp"
#include "PackageMapSpec.hpp"

#ifdef __CYGWIN__
#define _WIN32
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

class Mod {
public:
    std::string Name;
    std::string Author;
    std::string Description;
    std::string Version;
    int32_t LoadPriority = 0;
    int32_t RequiredVersion = 0;

    Mod(std::string name)
    {
        Name = name;
    }

    Mod() {}
    Mod(std::string name, std::string &json);
};

class ResourceModFile {
public:
    Mod Parent;
    std::string Name;
    std::vector<std::byte> FileBytes;
    bool IsBlangJson = false;
    bool IsAssetsInfoJson = false;
    std::optional<class AssetsInfo> AssetsInfo = std::nullopt;
    std::optional<uint64_t> StreamDbHash = std::nullopt;
    std::string ResourceType;
    std::optional<uint16_t> Version = std::nullopt;
    bool PlaceBefore = false;
    std::string PlaceByName;
    std::string PlaceByType;
    std::optional<std::byte> SpecialByte1 = std::nullopt;
    std::optional<std::byte> SpecialByte2 = std::nullopt;
    std::optional<std::byte> SpecialByte3 = std::nullopt;

    ResourceModFile(Mod parent, std::string name)
    {
        Parent = parent;
        Name = name;
    }
};

class SoundModFile {
public:
    Mod Parent;
    std::string Name;
    std::vector<std::byte> FileBytes;

    SoundModFile(Mod parent, std::string name)
    {
        Parent = parent;
        Name = name;
    }
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
    int64_t FileOffset = 0;
    int64_t SizeOffset = 0;
    int64_t SizeZ = 0;
    int64_t Size = 0;
    std::byte CompressionMode;

    ResourceChunk(class ResourceName name, int64_t fileOffset)
    {
        ResourceName = name;
        FileOffset = fileOffset;
        SizeOffset = 0;
        SizeZ = 0;
        Size = 0;
        CompressionMode = (std::byte)(0);
    }

    ResourceChunk() {}
};

class ResourceContainer {
public:
    std::string Name;
    std::string Path;
    int32_t FileCount = 0;
    int32_t TypeCount = 0;
    int32_t StringsSize = 0;
    int64_t NamesOffset = 0;
    int64_t InfoOffset = 0;
    int64_t Dummy7Offset = 0;
    int64_t DataOffset = 0;
    int64_t IdclOffset = 0;
    int32_t UnknownCount = 0;
    int32_t FileCount2 = 0;
    int64_t NamesOffsetEnd = 0;
    int64_t UnknownOffset = 0;
    int64_t UnknownOffset2 = 0;
    std::vector<ResourceName> NamesList;
    std::vector<ResourceChunk> ChunkList;
    std::vector<ResourceModFile> ModFileList;
    std::vector<ResourceModFile> NewModFileList;

    ResourceContainer(std::string name, std::string path)
    {
        Name = name;
        Path = path;
    }

    bool ContainsResourceWithName(std::string name)
    {
        for (auto &resourceName : NamesList) {
            if (resourceName.FullFileName == name || resourceName.NormalizedFileName == name)
                return true;
        }

        return false;
    }

    int64_t GetResourceNameId(std::string name)
    {
        for (int32_t i = 0; i < NamesList.size(); i++) {
            if (NamesList[i].FullFileName == name || NamesList[i].NormalizedFileName == name)
                return i;
        }

        return -1;
    }
};

class SoundEntry {
public:
    uint32_t SoundId = 0;
    int64_t InfoOffset = 0;

    SoundEntry(uint32_t soundId, int64_t infoOffset)
    {
        SoundId = soundId;
        InfoOffset = infoOffset;
    }
};

class SoundContainer {
public:
    std::string Name;
    std::string Path;
    std::vector<SoundModFile> ModFileList;
    std::vector<SoundEntry> SoundEntries;

    SoundContainer(std::string name, std::string path)
    {
        Name = name;
        Path = path;
    }
};

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

class BlangFile {
public:
    int64_t UnknownData = 0;
    std::vector<BlangString> Strings;

    BlangFile() {}
    BlangFile(std::vector<std::byte> &blangBytes);

    std::vector<std::byte> ToByteVector();
};

class BlangFileEntry {
public:
    class BlangFile BlangFile;
    ResourceChunk Chunk;
    bool WasModified = false;

    BlangFileEntry(class BlangFile blangFile, ResourceChunk chunk)
    {
        BlangFile = blangFile;
        Chunk = chunk;
    }

    BlangFileEntry() {}
};

class MapAsset {
public:
    int32_t AssetTypeIndex;
    std::string Name;
    int32_t UnknownData1 = 0;
    int32_t UnknownData2 = 0;
    int64_t UnknownData3 = 0;
    int64_t UnknownData4 = 0;
};

class ResourceDataEntry {
public:
    uint64_t StreamDbHash = 0;
    std::string ResourceType;
    std::string MapResourceType;
    std::string MapResourceName;
    std::byte Version = (std::byte)0;
    std::byte SpecialByte1 = (std::byte)0;
    std::byte SpecialByte2 = (std::byte)0;
    std::byte SpecialByte3 = (std::byte)0;
};

class MapResourcesFile {
public:
    int32_t Magic = 0;
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

extern const int32_t Version;
extern const std::string ResourceDataFileName;
extern const std::string PackageMapSpecJsonFileName;
extern std::string BasePath;
extern std::vector<ResourceContainer> ResourceContainerList;
extern std::vector<SoundContainer> SoundContainerList;
extern bool Verbose;
extern bool SlowMode;
extern std::map<uint64_t, ResourceDataEntry> ResourceDataMap;

extern std::byte *Buffer;
extern int64_t BufferSize;

extern std::string RESET;
extern std::string RED;
extern std::string GREEN;
extern std::string YELLOW;
extern std::string BLUE;

extern char separator;

extern std::vector<std::string> SupportedFileFormats;

#ifdef _WIN32
void ReplaceChunks(std::byte *&mem, HANDLE &hFile, HANDLE &fileMapping, ResourceContainer &resourceContainer);
void AddChunks(std::byte *&mem, HANDLE &hFile, HANDLE &fileMapping, ResourceContainer &resourceContainer);
void ReplaceSounds(std::byte *&mem, HANDLE &hFile, HANDLE &fileMapping, SoundContainer &soundContainer);
#else
void ReplaceChunks(std::byte *&mem, int32_t &fd, ResourceContainer &resourceContainer);
void AddChunks(std::byte *&mem, int32_t &fd, ResourceContainer &resourceContainer);
void ReplaceSounds(std::byte *&mem, int32_t &fd, SoundContainer &soundContainer);
#endif

std::string PathToResourceContainer(std::string name);
void ReadChunkInfo(std::byte *&mem, ResourceContainer &resourceContainer);
ResourceChunk *GetChunk(std::string name, ResourceContainer &resourceContainer);
void ReadResource(std::byte *&mem, ResourceContainer &resourceContainer);
int32_t GetResourceContainer(std::string &resourceContainerName);
std::vector<std::byte> IdCrypt(std::vector<std::byte> fileData, std::string internalPath, bool decrypt);
std::string RemoveWhitespace(std::string &stringWithWhitespace);
std::string ToLower(std::string &str);
std::vector<std::string> SplitString(std::string stringToSplit, char delimiter);
std::map<uint64_t, ResourceDataEntry> ParseResourceData(std::string &filename);
uint64_t CalculateResourceFileNameHash(std::string &input);
std::string NormalizeResourceFilename(std::string filename);
bool EndsWith(const std::string &fullString, const std::string &suffix);
bool StartsWith(const std::string &fullString, const std::string &prefix);
std::string PathToSoundContainer(std::string name);
int32_t GetSoundContainer(std::string &soundContainerName);
void ReadSoundEntries(std::byte *mem, SoundContainer &soundContainer);
std::vector<SoundEntry> GetSoundEntriesToModify(SoundContainer &soundContainer, uint32_t soundModId);
void SetOptimalBufferSize(std::string driveRootPath);

#endif
