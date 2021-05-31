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
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <climits>

#include "zipper/unzipper.h"
#include "EternalModLoader.hpp"

const int Version = 6;
const std::string ResourceDataFileName = "rs_data";
const std::string PackageMapSpecJsonFileName = "packagemapspec.json";
std::string BasePath;
bool Verbose;
std::vector<ResourceContainer> ResourceContainerList;
std::vector<SoundContainer> SoundContainerList;
std::map<unsigned long, ResourceDataEntry> ResourceDataMap;

std::string RESET = "";
std::string RED = "";
std::string GREEN = "";
std::string YELLOW = "";
std::string BLUE = "";

int main(int argc, char **argv)
{
    std::ios::sync_with_stdio(false);

    if (std::getenv("ETERNALMODLOADER_NO_COLORS") == NULL) {
        RESET = "\033[0m";
        RED = "\033[31m";
        GREEN = "\033[32m";
        YELLOW = "\033[33m";
        BLUE = "\033[34m";
    }

    if (argc == 1 || argc > 3) {
        std::cout << "EternalModLoaderCpp by PowerBall253, based on EternalModLoader by proteh\n\n";
        std::cout << "Loads DOOM Eternal mods from ZIPs or loose files in 'Mods' folder into the .resources files in the specified directory.\n\n";
        std::cout << "USAGE: " << argv[0] << " <game path | --version> [OPTIONS]\n";
        std::cout << "\t--version - Prints the version number of the mod loader and exits with exit code same as the version number.\n\n";
        std::cout << "OPTIONS:\n";
        std::cout << "\t--list-res - List the .resources files that will be modified and exit.\n";
        std::cout << "\t--verbose - Print more information during the mod loading process.\n" << std::endl;
        return 1;
    }

    if (!strcmp(argv[1], "--version")) {
        std::cout << Version << std::endl;
        return Version;
    }

    BasePath = std::string(argv[1]) + "/base/";

    if (!std::filesystem::exists(BasePath)) {
        std::cerr << RED << "ERROR: " << RESET << "Game directory does not exist!" << std::endl;
        return 1;
    }

    bool listResources = false;

    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            if (!strcmp(argv[i], "--list-res")) {
                listResources = true;
            }
            else if (!strcmp(argv[i], "--verbose")) {
                Verbose = true;
            }
            else {
                std::cerr << RED << "ERROR: " << RESET << "Unknown argument: " << argv[i] << std::endl;
                return 1;
            }
        }
    }

    ResourceContainerList.reserve(80);
    SoundContainerList.reserve(40);

    if (!listResources) {
        std::string resourceDataFilePath = BasePath + ResourceDataFileName;

        if (std::filesystem::exists(resourceDataFilePath)) {
            try {
                ResourceDataMap = ParseResourceData(resourceDataFilePath);

                if (ResourceDataMap.empty())
                    throw std::exception();
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to parse " << ResourceDataFileName << std::endl;
            }
        }
        else {
            if (Verbose) {
                std::cerr << RED << "WARNING: " << RESET << ResourceDataFileName << " was not found! There will be issues when adding existing new assets to containers..." << std::endl;
            }
        }
    }

    std::vector<std::string> zippedMods;

    for (const auto &zippedMod : std::filesystem::directory_iterator(std::string(argv[1]) + "/Mods")) {
        if (std::filesystem::is_regular_file(zippedMod) && zippedMod.path().extension() == ".zip")
            zippedMods.push_back(zippedMod.path());
    }

    std::sort(zippedMods.begin(), zippedMods.end(), [](std::string str1, std::string str2) { return std::strcoll(str1.c_str(), str2.c_str()) <= 0 ? true : false; });

    for (const auto &zippedMod : zippedMods) {
        int zippedModCount = 0;
        std::vector<std::string> modFileNameList;
        zipper::Unzipper modZip(zippedMod);

        Mod mod(std::filesystem::path(zippedMod).filename());

        if (!listResources) {
            std::vector<unsigned char> unzippedModJson;

            if (modZip.extractEntryToMemory("EternalMod.json", unzippedModJson)) {
                std::string modJson((char*)unzippedModJson.data(), unzippedModJson.size());

                try {
                    mod = Mod(mod.Name, modJson);

                    if (mod.RequiredVersion > Version) {
                        std::cerr << RED << "WARNING: " << RESET << "Mod " << std::filesystem::path(zippedMod).filename().string() << " requires mod loader version "
                            << mod.RequiredVersion << " but the current mod loader version is " << Version << ", skipping" << std::endl;
                        continue;
                    }
                }
                catch (...) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to parse EternalMod.json - using defaults." << std::endl;
                }
            }
        }

        for (auto &zipEntry : modZip.entries()) {
            if (0 == zipEntry.name.compare(zipEntry.name.length() - 1, 1, "/"))
                continue;

            bool isSoundMod = false;
            std::string modFileName = zipEntry.name;
            std::vector<std::string> modFilePathParts = SplitString(modFileName, '/');

            if (modFilePathParts.size() < 2)
                continue;

            std::string resourceName = modFilePathParts[0];

            if (ToLower(resourceName) == "generated") {
                resourceName = "gameresources";
            }
            else {
                modFileName = modFileName.substr(resourceName.size() + 1, modFileName.size() - resourceName.size() - 1);
            }

            std::string resourcePath = PathToResourceContainer(resourceName + ".resources");

            if (resourcePath.empty()) {
                resourcePath = PathToSoundContainer(resourceName);

                if (!resourcePath.empty())
                    isSoundMod = true;
            }

            if (isSoundMod) {
                int soundContainerIndex = GetSoundContainer(resourceName);

                if (soundContainerIndex == -1) {
                    SoundContainer soundContainer(resourceName, resourcePath);
                    SoundContainerList.push_back(soundContainer);

                    soundContainerIndex = SoundContainerList.size() - 1;
                }

                if (!listResources) {
                    std::string soundExtension = std::filesystem::path(modFileName).extension();

                    if (std::find(SupportedFileFormats.begin(), SupportedFileFormats.end(), soundExtension) == SupportedFileFormats.end()) {
                        std::cerr << RED << "WARNING: " << RESET << "Unsupported sound mod file format " << soundExtension << " for file " << modFileName << std::endl;
                        continue;
                    }

                    std::vector<unsigned char> unzippedEntry;
                    unzippedEntry.reserve(zipEntry.uncompressedSize);
                    modZip.extractEntryToMemory(zipEntry.name, unzippedEntry);

                    SoundModFile soundModFile(mod, std::filesystem::path(modFileName).filename());

                    soundModFile.FileBytes.resize(unzippedEntry.size());
                    std::copy((std::byte*)unzippedEntry.data(), (std::byte*)unzippedEntry.data() + unzippedEntry.size(), soundModFile.FileBytes.begin());

                    SoundContainerList[soundContainerIndex].ModFileList.push_back(soundModFile);
                    zippedModCount++;
                }
            }
            else {
                int resourceContainerIndex = GetResourceContainer(resourceName);

                if (resourceContainerIndex == -1) {
                    ResourceContainer resource(resourceName, PathToResourceContainer(resourceName + ".resources"));
                    ResourceContainerList.push_back(resource);

                    resourceContainerIndex = ResourceContainerList.size() - 1;
                }

                if (!listResources) {
                    unsigned long streamSize = zipEntry.uncompressedSize;

                    if (streamSize > ResourceContainerList.max_size()) {
                        std::cerr << RED << "WARNING: " << RESET << "Skipped " << modFileName << " - too large." << std::endl;
                        continue;
                    }

                    std::vector<unsigned char> unzippedEntry;
                    unzippedEntry.reserve(zipEntry.uncompressedSize);
                    modZip.extractEntryToMemory(zipEntry.name, unzippedEntry);

                    ResourceModFile resourceModFile(mod, modFileName);

                    resourceModFile.FileBytes.resize(unzippedEntry.size());
                    std::copy((std::byte*)unzippedEntry.data(), (std::byte*)unzippedEntry.data() + unzippedEntry.size(), resourceModFile.FileBytes.begin());

                    if (ToLower(modFilePathParts[1]) == "eternalmod") {
                        if (modFilePathParts.size() == 4
                        && ToLower(modFilePathParts[2]) == "assetsinfo"
                        && std::filesystem::path(modFilePathParts[3]).extension() == ".json") {
                            try {
                                std::string assetsInfoJson((char*)resourceModFile.FileBytes.data(), resourceModFile.FileBytes.size());
                                resourceModFile.AssetsInfo = AssetsInfo(assetsInfoJson);
                                resourceModFile.IsAssetsInfoJson = true;
                                resourceModFile.FileBytes.resize(0);
                            }
                            catch (...) {
                                std::cerr << RED << "ERROR: " << RESET << "Failed to parse EternalMod/assetsinfo/"
                                    << std::filesystem::path(mod.Name).stem().string() << ".json" << std::endl;
                                continue;
                            }
                        }
                        else if (modFilePathParts.size() == 4
                        && ToLower(modFilePathParts[2]) == "strings"
                        && std::filesystem::path(modFilePathParts[3]).extension() == ".json") {
                            resourceModFile.IsBlangJson = true;
                        }
                        else {
                            continue;
                        }
                    }

                    ResourceContainerList[resourceContainerIndex].ModFileList.push_back(resourceModFile);
                    zippedModCount++;
                }
            }
        }
            
        if (zippedModCount > 0 && !listResources)
            std::cout << "Found " << BLUE << zippedModCount << " file(s) " << RESET << "in archive " << YELLOW << zippedMod << RESET << "..." << std::endl;

        modZip.close();
    }

    std::vector<std::string> unzippedMods;

    for (const auto &unzippedMod : std::filesystem::recursive_directory_iterator(std::string(argv[1]) + "/Mods")) {
        if (std::filesystem::is_regular_file(unzippedMod) && !(unzippedMod.path().extension() == ".zip"))
            unzippedMods.push_back(unzippedMod.path());
    }

    std::sort(unzippedMods.begin(), unzippedMods.end(), [](std::string str1, std::string str2) { return std::strcoll(str1.c_str(), str2.c_str()) <= 0 ? true : false; });

    int unzippedModCount = 0;

    Mod globalLooseMod;
    globalLooseMod.LoadPriority = INT_MIN;

    for (const auto &unzippedMod : unzippedMods) {
        std::string unzippedModPath = unzippedMod;
        std::vector<std::string> modFilePathParts = SplitString(unzippedModPath, '/');

        if (modFilePathParts.size() < 4)
            continue;

        bool isSoundMod = false;
        std::string resourceName = modFilePathParts[2];
        std::string fileName;

        if (ToLower(resourceName) == "generated") {
            resourceName = "gameresources";
            fileName = unzippedModPath.substr(modFilePathParts[1].size() + 3, unzippedModPath.size() - modFilePathParts[1].size() - 3);
        }
        else {
            fileName = unzippedModPath.substr(modFilePathParts[1].size() + resourceName.size() + 4, unzippedModPath.size() - resourceName.size() - 4);
        }

        std::string resourcePath = PathToResourceContainer(resourceName + ".resources");

        if (resourcePath.empty()) {
            resourcePath = PathToSoundContainer(resourceName);

            if (!resourcePath.empty())
                isSoundMod = true;
        }

        if (isSoundMod) {
            int soundContainerIndex = GetSoundContainer(resourceName);

            if (soundContainerIndex == -1) {
                SoundContainer soundContainer(resourceName, resourcePath);
                SoundContainerList.push_back(soundContainer);

                soundContainerIndex = SoundContainerList.size() - 1;
            }

            if (!listResources) {
                std::string soundExtension = std::filesystem::path(fileName).extension();

                if (std::find(SupportedFileFormats.begin(), SupportedFileFormats.end(), soundExtension) == SupportedFileFormats.end()) {
                    std::cerr << RED << "WARNING: " << RESET << "Unsupported sound mod file format " << soundExtension << " for file " << fileName << std::endl;
                    continue;
                }

                long unzippedModSize = std::filesystem::file_size(unzippedModPath);

                if (unzippedModSize > ResourceContainerList.max_size())
                    std::cerr << RED << "WARNING: " << RESET << "Skipped " << fileName << " - too large." << std::endl;
                
                SoundModFile soundModFile(globalLooseMod, std::filesystem::path(fileName).filename());
                
                FILE *unzippedModFile = fopen(unzippedModPath.c_str(), "rb");

                if (!unzippedModFile) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to open " << unzippedModPath << " for reading." << std::endl;
                    continue;
                }

                soundModFile.FileBytes.resize(unzippedModSize);

                if (fread(soundModFile.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                    std::cerr << RESET << "ERROR: " << RESET << "Failed to read from " << unzippedModPath << "." << std::endl;
                    continue;
                }

                fclose(unzippedModFile);

                SoundContainerList[soundContainerIndex].ModFileList.push_back(soundModFile);
                unzippedModCount++;
            }
        }
        else {
            int resourceContainerIndex = GetResourceContainer(resourceName);

            if (resourceContainerIndex == -1) {
                ResourceContainer resourceContainer(resourceName, PathToResourceContainer(resourceName));
                ResourceContainerList.push_back(resourceContainer);

                resourceContainerIndex = ResourceContainerList.size() - 1;
            }

            if (!listResources) {
                long unzippedModSize = std::filesystem::file_size(unzippedModPath);

                if (unzippedModSize > ResourceContainerList.max_size())
                    std::cerr << RED << "WARNING: " << RESET << "Skipped " << fileName << " - too large." << std::endl;

                ResourceModFile resourceModFile(globalLooseMod, fileName);

                FILE *unzippedModFile = fopen(unzippedModPath.c_str(), "rb");

                if (!unzippedModFile) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to open " << unzippedModPath << " for reading." << std::endl;
                    continue;
                }

                resourceModFile.FileBytes.resize(unzippedModSize);

                if (fread(resourceModFile.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                    std::cerr << RESET << "ERROR: " << RESET << "Failed to read from " << unzippedModPath << "." << std::endl;
                    continue;
                }

                fclose(unzippedModFile);

                if (ToLower(modFilePathParts[3]) == "eternalmod") {
                    if (modFilePathParts.size() == 6
                    && ToLower(modFilePathParts[4]) == "assetsinfo"
                    && std::filesystem::path(modFilePathParts[5]).extension() == ".json") {
                        try {
                            std::string assetsInfoJson((char*)resourceModFile.FileBytes.data(), resourceModFile.FileBytes.size());
                            resourceModFile.AssetsInfo = AssetsInfo(assetsInfoJson);
                            resourceModFile.IsAssetsInfoJson = true;
                            resourceModFile.FileBytes.resize(0);
                        }
                        catch (...) {
                            std::cerr << RED << "ERROR: " << RESET << "Failed to parse EternalMod/assetsinfo/"
                                << std::filesystem::path(resourceModFile.Name).stem().string() << ".json" << std::endl;
                            continue;
                        }
                    }
                    else if (modFilePathParts.size() == 6
                    && ToLower(modFilePathParts[4]) == "strings"
                    && std::filesystem::path(modFilePathParts[5]).extension() == ".json") {
                        resourceModFile.IsBlangJson = true;
                    }
                    else {
                        continue;
                    }
                }

                ResourceContainerList[resourceContainerIndex].ModFileList.push_back(resourceModFile);
                unzippedModCount++;
            }
        }
    }

    if (unzippedModCount > 0 && !(listResources))
        std::cout << "Found " << BLUE << unzippedModCount << " file(s) " << RESET << "in " << YELLOW << "'Mods' " << RESET << "folder..." << std::endl;

    if (listResources) {
        for (auto &resourceContainer : ResourceContainerList) {
            if (resourceContainer.Path.empty())
                continue;

            std::cout << resourceContainer.Path << std::endl;
        }

        for (auto &soundContainer : SoundContainerList) {
            if (soundContainer.Path.empty())
                continue;

            std::cout << soundContainer.Path << std::endl;
        }

        return 0;
    }

    for (auto &resourceContainer : ResourceContainerList) {
        if (resourceContainer.Path.empty()) {
            std::cerr << RED << "WARNING: " << YELLOW << resourceContainer.Name << ".resources" << RESET << " was not found! Skipping " << RED << resourceContainer.ModFileList.size() << " file(s)" << RESET << "..." << std::endl;
            continue;
        }

        long fileSize = std::filesystem::file_size(resourceContainer.Path);

        if (fileSize == 0) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        mmap_allocator_namespace::mmappable_vector<std::byte> mem;

        try {
            mem.mmap_file(resourceContainer.Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, fileSize,
                mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);
            
            if (mem.empty())
                throw std::exception();
        }
        catch (...) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        ReadResource(mem, resourceContainer);
        ReplaceChunks(mem, resourceContainer);
        AddChunks(mem, resourceContainer);

        mem.munmap_file();
    }

    for (auto &soundContainer : SoundContainerList) {
        if (soundContainer.Path.empty()) {
            std::cerr << RED << "WARNING: " << YELLOW << soundContainer.Name << ".resources" << RESET << " was not found! Skipping " << RED << soundContainer.ModFileList.size() << " file(s)" << RESET << "..." << std::endl;
            continue;
        }

        long fileSize = std::filesystem::file_size(soundContainer.Path);

        if (fileSize == 0) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        mmap_allocator_namespace::mmappable_vector<std::byte> mem;

        try {
            mem.mmap_file(soundContainer.Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, fileSize,
                mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);
            
            if (mem.empty())
                throw std::exception();
        }
        catch (...) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        LoadSoundMods(mem, soundContainer);

        mem.munmap_file();
    }
    
    std::cout << GREEN << "Finished." << RESET << std::endl;
    return 0;
}