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
#include <map>
#include <optional>
#include <mutex>
#include <atomic>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

#include "AssetsInfo/AssetsInfo.hpp"
#include "BlangFile/BlangFile.hpp"
#include "MapResourcesFile/MapResourcesFile.hpp"
#include "MemoryMappedFile/MemoryMappedFile.hpp"
#include "Mod/Mod.hpp"
#include "Oodle/Oodle.hpp"
#include "PackageMapSpec/PackageMapSpec.hpp"
#include "PackageMapSpec/PackageMapSpecInfo.hpp"
#include "ResourceData/ResourceData.hpp"
#include "Utils/Utils.hpp"

// Colors for cout
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"

namespace fs = std::filesystem;

/**
 * @brief ResourceModFile class
 * 
 */
class ResourceModFile {
public:
    Mod Parent;
    std::string Name;
    std::string ResourceName;
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
    bool Announce = true;

    /**
     * @brief Construct a new ResourceModFile object
     * 
     * @param parent Mod to inherit data from
     * @param name Mod file's name
     * @param resourceName Resource's name
     * @param announce Whether
     */
    ResourceModFile(Mod parent, std::string name, std::string resourceName, bool announce = true)
    {
        Parent = parent;
        Name = name;
        ResourceName = resourceName;
        Announce = announce;
    }
};

/**
 * @brief SoundModFile class
 * 
 */
class SoundModFile {
public:
    Mod Parent;
    std::string Name;
    std::vector<std::byte> FileBytes;

    /**
     * @brief Construct a new SoundModFile object
     * 
     * @param parent Mod to inherit data from
     * @param name Sound file's name
     */
    SoundModFile(Mod parent, std::string name)
    {
        Parent = parent;
        Name = name;
    }
};

/**
 * @brief ResourceName class
 * 
 */
class ResourceName {
public:
    std::string FullFileName;
    std::string NormalizedFileName;

    /**
     * @brief Construct a new ResourceName object
     * 
     * @param fullFileName Resource's full name
     * @param normalizedFileName Resource's normalized name
     */
    ResourceName(std::string fullFileName, std::string normalizedFileName)
    {
        FullFileName = fullFileName;
        NormalizedFileName = normalizedFileName;
    }

    ResourceName() {}
};

/**
 * @brief ResourceChunk class
 * 
 */
class ResourceChunk {
public:
    class ResourceName ResourceName;
    int64_t FileOffset = 0;
    int64_t SizeOffset = 0;
    int64_t SizeZ = 0;
    int64_t Size = 0;
    std::byte CompressionMode = (std::byte)0;

    /**
     * @brief Construct a new ResourceChunk object
     * 
     * @param name Resource chunk's name
     * @param fileOffset Resource chunk's offset in the resource container
     */
    ResourceChunk(class ResourceName name, int64_t fileOffset)
    {
        ResourceName = name;
        FileOffset = fileOffset;
        SizeOffset = 0;
        SizeZ = 0;
        Size = 0;
    }

    ResourceChunk() {}
};

/**
 * @brief ResourceContainer class
 * 
 */
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

    /**
     * @brief Construct a new ResourceContainer object
     * 
     * @param name Resource container's name
     * @param path Resource container's path
     */
    ResourceContainer(std::string name, std::string path)
    {
        Name = name;
        Path = path;
    }

    /**
     * @brief Check if resource container contains a resource with the given name
     * 
     * @param name Name of the resource to find
     * @return True if found, false otherwise
     */
    bool ContainsResourceWithName(std::string name) const
    {
        for (auto &resourceName : NamesList) {
            if (resourceName.FullFileName == name || resourceName.NormalizedFileName == name) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Get the name ID from a resource
     * 
     * @param name Name of the resource to find
     * @return the resource's ID if found, -1 otherwise
     */
    int64_t GetResourceNameId(std::string name) const
    {
        for (int32_t i = 0; i < NamesList.size(); i++) {
            if (NamesList[i].FullFileName == name || NamesList[i].NormalizedFileName == name) {
                return i;
            }
        }

        return -1;
    }
};

/**
 * @brief SoundEntry class
 * 
 */
class SoundEntry {
public:
    uint32_t SoundId = 0;
    int64_t InfoOffset = 0;

    /**
     * @brief Construct a new SoundEntry object
     * 
     * @param soundId Sound's ID
     * @param infoOffset Sound info's offset in the sound container
     */
    SoundEntry(uint32_t soundId, int64_t infoOffset)
    {
        SoundId = soundId;
        InfoOffset = infoOffset;
    }
};

/**
 * @brief SoundContainer class
 * 
 */
class SoundContainer {
public:
    std::string Name;
    std::string Path;
    std::vector<SoundModFile> ModFileList;
    std::vector<SoundEntry> SoundEntries;

    /**
     * @brief Construct a new SoundContainer object
     * 
     * @param name Sound container's name
     * @param path Sound container's path
     */
    SoundContainer(std::string name, std::string path)
    {
        Name = name;
        Path = path;
    }
};

/**
 * @brief BlangFileEntry class
 * 
 */
class BlangFileEntry {
public:
    class BlangFile BlangFile;
    ResourceChunk Chunk;
    bool WasModified = false;
    bool Announce = false;

    /**
     * @brief Construct a new BlangFileEntry object
     * 
     * @param blangFile BlangFile object
     * @param chunk ResourceChunk containing the blang file
     */
    BlangFileEntry(class BlangFile blangFile, ResourceChunk chunk)
    {
        BlangFile = blangFile;
        Chunk = chunk;
    }

    BlangFileEntry() {}
};

/**
 * @brief Equality operator for ResourceChunk objects
 * 
 * @param chunk1 First ResourceChunk object
 * @param chunk2 Second ResourceChunk object
 * @return True if equal, false otherwise
 */
inline bool operator==(ResourceChunk& chunk1, const ResourceChunk& chunk2)
{
    return chunk1.ResourceName.FullFileName == chunk2.ResourceName.FullFileName;
}

// Global variables
inline constexpr int32_t Version = 16;

extern char Separator;
extern std::string BasePath;
extern bool ListResources;
extern bool Verbose;
extern bool SlowMode;
extern bool LoadOnlineSafeModsOnly;
extern bool CompressTextures;
extern bool MultiThreading;
extern bool AreModsSafeForOnline;

extern std::vector<ResourceContainer> ResourceContainerList;
extern std::vector<SoundContainer> SoundContainerList;
extern std::map<uint64_t, ResourceDataEntry> ResourceDataMap;
extern const std::vector<std::string> SupportedFileFormats;

extern std::vector<std::stringstream> stringStreams;
extern int32_t streamIndex;

extern std::byte *Buffer;
extern int64_t BufferSize;

extern std::mutex mtx;

extern const std::string PackageMapSpecJsonFileName;
extern class PackageMapSpecInfo PackageMapSpecInfo;
extern const std::byte *DivinityMagic;

// Resource mods
void LoadResourceMods(ResourceContainer &resourceContainer);
void ReadResource(MemoryMappedFile &memoryMappedFile, ResourceContainer &resourceContainer);
void ReadChunkInfo(MemoryMappedFile &memoryMappedFile, ResourceContainer &resourceContainer);
void ReplaceChunks(MemoryMappedFile &memoryMappedFile, ResourceContainer &resourceContainer, std::stringstream &os);
void AddChunks(MemoryMappedFile &memoryMappedFile, ResourceContainer &resourceContainer, std::stringstream &os);
bool SetModDataForChunk(
    MemoryMappedFile &memoryMappedFile,
    ResourceContainer &resourceContainer,
    ResourceChunk &chunk,
    ResourceModFile &modFile,
    const uint64_t compressedSize,
    const uint64_t uncompressedSize,
    const std::byte *compressionMode);

// Sound mods
void LoadSoundMods(SoundContainer &soundContainer);
void ReadSoundEntries(MemoryMappedFile &memoryMappedFile, SoundContainer &soundContainer);
std::vector<SoundEntry> GetSoundEntriesToModify(SoundContainer &soundContainer, uint32_t soundModId);
void ReplaceSounds(MemoryMappedFile &memoryMappedFile, SoundContainer &soundContainer, std::stringstream &os);

// Path to containers
std::string PathToResourceContainer(const std::string &name);
std::string PathToSoundContainer(const std::string &name);

// Get object
int32_t GetResourceContainer(const std::string &resourceContainerName);
int32_t GetSoundContainer(const std::string &soundContainerName);
ResourceChunk *GetChunk(const std::string name, ResourceContainer &resourceContainer);

// Load mod files
void LoadZippedMod(std::string zippedMod, std::vector<std::string> &notFoundContainers);
void LoadUnzippedMod(std::string unzippedMod, Mod &globalLooseMod, std::atomic<int32_t> &unzippedModCount,
    std::map<int32_t, std::vector<ResourceModFile>> &resourceModFiles,
    std::map<int32_t, std::vector<SoundModFile>> &soundModFiles,
    std::vector<std::string> &notFoundContainers);

// Misc
std::vector<std::byte> IdCrypt(const std::vector<std::byte> &fileData, const std::string internalPath, const bool decrypt);
void GetResourceContainerPathList();

// Online Safety
std::vector<ResourceModFile> GetMultiplayerDisablerMods();
bool IsModSafeForOnline(const std::map<int32_t, std::vector<ResourceModFile>> &resourceModFiles);

#endif
