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

#include "AssetsInfo.hpp"
#include "Oodle.hpp"
#include "PackageMapSpec.hpp"

#ifdef __CYGWIN__
#define _WIN32
#endif

class Mod {
public:
    std::string Name;
    std::string Author;
    std::string Description;
    std::string Version;
    int LoadPriority = 0;
    int RequiredVersion = 0;

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
    std::optional<unsigned long> StreamDbHash = std::nullopt;
    std::string ResourceType;
    std::optional<unsigned short> Version = std::nullopt;
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
    long FileOffset = 0;
    long SizeOffset = 0;
    long SizeZ = 0;
    long Size = 0;
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

class ResourceContainer {
public:
    std::string Name;
    std::string Path;
    int FileCount = 0;
    int TypeCount = 0;
    int StringsSize = 0;
    long NamesOffset = 0;
    long InfoOffset = 0;
    long Dummy7Offset = 0;
    long DataOffset = 0;
    long IdclOffset = 0;
    int UnknownCount = 0;
    int FileCount2 = 0;
    long NamesOffsetEnd = 0;
    long UnknownOffset = 0;
    long UnknownOffset2 = 0;
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

    long GetResourceNameId(std::string name)
    {
        for (int i = 0; i < NamesList.size(); i++) {
            if (NamesList[i].FullFileName == name || NamesList[i].NormalizedFileName == name)
                return i;
        }

        return -1;
    }
};

class SoundContainer {
public:
    std::string Name;
    std::string Path;
    std::vector<SoundModFile> ModFileList;

    SoundContainer(std::string name, std::string path)
    {
        Name = name;
        Path = path;
    }
};

class BlangString {
public:
    unsigned int Hash = 0;
    std::string Identifier;
    std::string Text;
    std::vector<std::byte> Unknown;

    BlangString()
    {
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
    long UnknownData = 0;
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
    unsigned long StreamDbHash = 0;
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
    int Magic = 0;
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

extern const int Version;
extern const std::string ResourceDataFileName;
extern const std::string PackageMapSpecJsonFileName;
extern std::string BasePath;
extern std::vector<ResourceContainer> ResourceContainerList;
extern std::vector<SoundContainer> SoundContainerList;
extern bool Verbose;
extern std::map<unsigned long, ResourceDataEntry> ResourceDataMap;

extern std::string RESET;
extern std::string RED;
extern std::string GREEN;
extern std::string YELLOW;
extern std::string BLUE;

extern char separator;

extern std::vector<std::string> SupportedFileFormats;

std::string PathToResourceContainer(std::string name);
void ReadChunkInfo(FILE *&resourceFile, ResourceContainer &resourceContainer);
ResourceChunk *GetChunk(std::string name, ResourceContainer &resourceContainer);
void ReplaceChunks(FILE *&resourceFile, ResourceContainer &resourceContainer);
void AddChunks(FILE *&resourceFile, ResourceContainer &resourceContainer);
void ReadResource(FILE *&resourceFile, ResourceContainer &resourceContainer);
int GetResourceContainer(std::string &resourceContainerName);
std::vector<std::byte> IdCrypt(std::vector<std::byte> fileData, std::string internalPath, bool decrypt);
BlangFile ParseBlang(std::vector<std::byte> &blangBytes, std::string &resourceName);
std::vector<std::byte> WriteBlangToVector(BlangFile blangFile, std::string &resourceName);
std::string RemoveWhitespace(std::string &stringWithWhitespace);
std::string ToLower(std::string &str);
std::vector<std::string> SplitString(std::string stringToSplit, char delimiter);
void LoadSoundMods(FILE *&resourceFile, SoundContainer &soundContainer);
int ParseResourceData(std::string &filename);
unsigned long CalculateResourceFileNameHash(std::string &input);
std::string NormalizeResourceFilename(std::string filename);
bool EndsWith(const std::string &fullString, const std::string &ending);
std::string PathToSoundContainer(std::string name);
int GetSoundContainer(std::string &soundContainerName);

#endif
