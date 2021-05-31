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

#include "zipper/unzipper.h"
#include "EternalModLoader.hpp"

const std::string ResourceDataFileName = "rs_data";
const std::string PackageMapSpecJsonFileName = "packagemapspec.json";
std::string BasePath;
bool Verbose;
std::vector<ResourceInfo> ResourceList;
std::vector<SoundBankInfo> SoundBankList;
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
        std::cout << "USAGE: " << argv[0] << " <game path> [OPTIONS]\n\n";
        std::cout << "OPTIONS:\n";
        std::cout << "\t--list-res - List the .resources files that will be modified and exit.\n" << std::endl;
        return 1;
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

    ResourceList.reserve(80);
    SoundBankList.reserve(40);

    if (!listResources) {
        std::string resourceDataFilePath = BasePath + ResourceDataFileName;

        if (std::filesystem::exists(resourceDataFilePath)) {
            try {
                ResourceDataMap = ParseResourceData(resourceDataFilePath);

                if (ResourceDataMap.empty())
                    throw;
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

            std::string resourcePath = PathToResource(resourceName + ".resources");

            if (resourcePath.empty()) {
                resourcePath = PathToSoundBank(resourceName);

                if (!resourcePath.empty())
                    isSoundMod = true;
            }

            if (isSoundMod) {
                int soundBankInfoIndex = GetSoundBankInfo(resourceName);

                if (soundBankInfoIndex == -1) {
                    SoundBankInfo soundBank(resourceName, resourcePath);
                    SoundBankList.push_back(soundBank);

                    soundBankInfoIndex = (int)SoundBankList.size() - 1;
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

                    SoundMod sndMod;
                    sndMod.Name = std::filesystem::path(modFileName).filename();

                    sndMod.FileBytes.resize(unzippedEntry.size());
                    std::copy((std::byte*)unzippedEntry.data(), (std::byte*)unzippedEntry.data() + unzippedEntry.size(), sndMod.FileBytes.begin());

                    SoundBankList[soundBankInfoIndex].ModList.push_back(sndMod);
                    zippedModCount++;
                }
            }
            else {
                int resourceIndex = GetResourceInfo(resourceName);

                if (resourceIndex == -1) {
                    ResourceInfo resource(resourceName, PathToResource(resourceName + ".resources"));
                    ResourceList.push_back(resource);

                    resourceIndex = (int)ResourceList.size() - 1;
                }

                if (!listResources) {
                    unsigned long streamSize = zipEntry.uncompressedSize;

                    if (streamSize > ResourceList.max_size()) {
                        std::cerr << RED << "WARNING: " << RESET << "Skipped " << modFileName << " - too large." << std::endl;
                        continue;
                    }

                    std::vector<unsigned char> unzippedEntry;
                    unzippedEntry.reserve(zipEntry.uncompressedSize);
                    modZip.extractEntryToMemory(zipEntry.name, unzippedEntry);

                    Mod mod(modFileName);

                    mod.FileBytes.resize(unzippedEntry.size());
                    std::copy((std::byte*)unzippedEntry.data(), (std::byte*)unzippedEntry.data() + unzippedEntry.size(), mod.FileBytes.begin());

                    if (ToLower(modFilePathParts[1]) == "eternalmod") {
                        if (modFilePathParts.size() == 4
                        && ToLower(modFilePathParts[2]) == "assetsinfo"
                        && std::filesystem::path(modFilePathParts[3]).extension() == ".json") {
                            try {
                                std::string assetsInfoJson((char*)mod.FileBytes.data(), mod.FileBytes.size());
                                mod.AssetsInfo = AssetsInfo(assetsInfoJson);
                                mod.IsAssetsInfoJson = true;
                                mod.FileBytes.resize(0);
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
                            mod.IsBlangJson = true;
                        }
                        else {
                            continue;
                        }
                    }
                    else {
                        mod.IsBlangJson = false;
                        mod.IsAssetsInfoJson = false;
                    }

                    ResourceList[resourceIndex].ModList.push_back(mod);
                    zippedModCount++;
                }
            }
        }
            
        if (zippedModCount > 0 && !(listResources))
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

        std::string resourcePath = PathToResource(resourceName + ".resources");

        if (resourcePath.empty()) {
            resourcePath = PathToSoundBank(resourceName);

            if (!resourcePath.empty())
                isSoundMod = true;
        }

        if (isSoundMod) {
            int soundBankInfoIndex = GetSoundBankInfo(resourceName);

            if (soundBankInfoIndex == -1) {
                SoundBankInfo soundBank(resourceName, resourcePath);
                SoundBankList.push_back(soundBank);

                soundBankInfoIndex = (int)SoundBankList.size() - 1;
            }

            if (!listResources) {
                std::string soundExtension = std::filesystem::path(fileName).extension();

                if (std::find(SupportedFileFormats.begin(), SupportedFileFormats.end(), soundExtension) == SupportedFileFormats.end()) {
                    std::cerr << RED << "WARNING: " << RESET << "Unsupported sound mod file format " << soundExtension << " for file " << fileName << std::endl;
                    continue;
                }

                long unzippedModSize = std::filesystem::file_size(unzippedModPath);

                if (unzippedModSize > ResourceList.max_size())
                    std::cerr << RED << "WARNING: " << RESET << "Skipped " << fileName << " - too large." << std::endl;
                
                SoundMod soundMod;
                soundMod.Name = std::filesystem::path(fileName).filename();
                
                FILE *unzippedModFile = fopen(unzippedModPath.c_str(), "rb");

                if (!unzippedModFile) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to open " << unzippedModPath << " for reading." << std::endl;
                    continue;
                }

                soundMod.FileBytes.resize(unzippedModSize);

                if (fread(soundMod.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                    std::cerr << RESET << "ERROR: " << RESET << "Failed to read from " << unzippedModPath << "." << std::endl;
                    continue;
                }

                fclose(unzippedModFile);

                SoundBankList[soundBankInfoIndex].ModList.push_back(soundMod);
                unzippedModCount++;
            }
        }
        else {
            int resourceIndex = GetResourceInfo(resourceName);

            if (resourceIndex == -1) {
                ResourceInfo resource(resourceName, PathToResource(resourceName));
                ResourceList.push_back(resource);

                resourceIndex = (int)ResourceList.size() - 1;
            }

            if (!listResources) {
                long unzippedModSize = std::filesystem::file_size(unzippedModPath);

                if (unzippedModSize > ResourceList.max_size())
                    std::cerr << RED << "WARNING: " << RESET << "Skipped " << fileName << " - too large." << std::endl;

                Mod mod(fileName);

                FILE *unzippedModFile = fopen(unzippedModPath.c_str(), "rb");

                if (!unzippedModFile) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to open " << unzippedModPath << " for reading." << std::endl;
                    continue;
                }

                mod.FileBytes.resize(unzippedModSize);

                if (fread(mod.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                    std::cerr << RESET << "ERROR: " << RESET << "Failed to read from " << unzippedModPath << "." << std::endl;
                    continue;
                }

                fclose(unzippedModFile);

                if (ToLower(modFilePathParts[3]) == "eternalmod") {
                    if (modFilePathParts.size() == 6
                    && ToLower(modFilePathParts[4]) == "assetsinfo"
                    && std::filesystem::path(modFilePathParts[5]).extension() == ".json") {
                        try {
                            std::string assetsInfoJson((char*)mod.FileBytes.data(), mod.FileBytes.size());
                            mod.AssetsInfo = AssetsInfo(assetsInfoJson);
                            mod.IsAssetsInfoJson = true;
                            mod.FileBytes.resize(0);
                        }
                        catch (...) {
                            std::cerr << RED << "ERROR: " << RESET << "Failed to parse EternalMod/assetsinfo/"
                                << std::filesystem::path(mod.Name).stem().string() << ".json" << std::endl;
                            continue;
                        }
                    }
                    else if (modFilePathParts.size() == 6
                    && ToLower(modFilePathParts[4]) == "strings"
                    && std::filesystem::path(modFilePathParts[5]).extension() == ".json") {
                        mod.IsBlangJson = true;
                    }
                    else {
                        continue;
                    }
                }
                else {
                    mod.IsBlangJson = false;
                }

                ResourceList[resourceIndex].ModList.push_back(mod);
                unzippedModCount++;
            }
        }
    }

    if (unzippedModCount > 0 && !(listResources))
        std::cout << "Found " << BLUE << unzippedModCount << " file(s) " << RESET << "in " << YELLOW << "'Mods' " << RESET << "folder..." << std::endl;

    if (listResources) {
        for (auto &resource : ResourceList) {
            if (resource.Path.empty())
                continue;

            std::cout << resource.Path << std::endl;
        }

        for (auto &soundBank : SoundBankList) {
            if (soundBank.Path.empty())
                continue;

            std::cout << soundBank.Path << std::endl;
        }

        return 0;
    }

    for (auto &resource : ResourceList) {
        if (resource.Path.empty()) {
            std::cerr << RED << "WARNING: " << YELLOW << resource.Name << ".resources" << RESET << " was not found! Skipping " << RED << resource.ModList.size() << " file(s)" << RESET << "..." << std::endl;
            continue;
        }

        long fileSize = std::filesystem::file_size(resource.Path);

        if (fileSize == 0) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resource.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        mmap_allocator_namespace::mmappable_vector<std::byte> mem;

        try {
            mem.mmap_file(resource.Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, fileSize,
                mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);
            
            if (mem.empty())
                throw;
        }
        catch (...) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resource.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        ReadResource(mem, resource);
        ReplaceChunks(mem, resource);
        AddChunks(mem, resource);

        mem.munmap_file();
    }

    for (auto &soundBank : SoundBankList) {
        if (soundBank.Path.empty()) {
            std::cerr << RED << "WARNING: " << YELLOW << soundBank.Name << ".resources" << RESET << " was not found! Skipping " << RED << soundBank.ModList.size() << " file(s)" << RESET << "..." << std::endl;
            continue;
        }

        long fileSize = std::filesystem::file_size(soundBank.Path);

        if (fileSize == 0) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundBank.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        mmap_allocator_namespace::mmappable_vector<std::byte> mem;

        try {
            mem.mmap_file(soundBank.Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, fileSize,
                mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);
            
            if (mem.empty())
                throw;
        }
        catch (...) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundBank.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        LoadSoundMods(mem, soundBank);

        mem.munmap_file();
    }
    
    std::cout << GREEN << "Finished." << RESET << std::endl;
    return 0;
}