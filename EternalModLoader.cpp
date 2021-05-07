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

std::string BasePath;
std::vector<ResourceInfo> ResourceList;

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

    ResourceList.reserve(80);

    if (argc == 1 || argc > 3) {
        std::cout << "EternalModLoaderCpp by PowerBall253, based on EternalModLoader by proteh\n\n";
        std::cout << "Loads mods from ZIPs or loose files in 'Mods' folder into the .resources files in the specified directory.\n\n";
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

    if (argc == 3) {
        if (!strcmp(argv[2], "--list-res")) {
            listResources = true;
        }
        else {
            std::cerr << RED << "ERROR: " << RESET << "Unknown argument: " << argv[2] << std::endl;
            return 1;
        }
    }

    for (const auto& zippedMod : std::filesystem::directory_iterator(std::string(argv[1]) + "/Mods")) {
        if (std::filesystem::is_regular_file(zippedMod) && zippedMod.path().extension() == ".zip") {
            int zippedModCount = 0;
            std::vector<std::string> modFileNameList;
            zipper::Unzipper modZip(zippedMod.path());

            for (auto& zipEntry : modZip.entries()) {
                if (0 == zipEntry.name.compare(zipEntry.name.length() - 1, 1, "/"))
                    continue;

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

                int resourceIndex = GetResourceInfo(resourceName);
                std::string sndPath;

                if (resourceIndex == -1) {
                    bool isSnd;

                    if (PathToRes(resourceName, isSnd).empty()) {
                        if (PathToRes(modFilePathParts[2], isSnd).empty())
                            continue;
                        
                        ResourceInfo resource(modFilePathParts[2], PathToRes(modFilePathParts[2], isSnd));
                        resource.IsSnd = isSnd;
                        ResourceList.push_back(resource);
                    }
                    else {
                        ResourceInfo resource(resourceName, PathToRes(resourceName, isSnd));
                        ResourceList.push_back(resource);
                    }

                    resourceIndex = (int)ResourceList.size() - 1;
                }

                if (!listResources) {
                    unsigned long streamSize = zipEntry.uncompressedSize;

                    if (streamSize > ResourceList.max_size()) {
                        std::cerr << "Skipped " << modFileName << " - too large." << std::endl;
                        continue;
                    }

                    std::vector<unsigned char> unzipped_entry;
                    unzipped_entry.reserve(zipEntry.uncompressedSize);
                    modZip.extractEntryToMemory(zipEntry.name, unzipped_entry);

                    std::vector<std::byte> unzipped_entry_bytes((std::byte*)unzipped_entry.data(), (std::byte*)unzipped_entry.data() + unzipped_entry.size());

                    Mod mod(modFileName);
                    mod.FileBytes = unzipped_entry_bytes;

                    if (ToLower(modFilePathParts[1]) == "eternalmod") {
                        if (modFilePathParts.size() == 4
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
                    }

                    ResourceList[resourceIndex].ModList.push_back(mod);
                    zippedModCount++;
                }
            }
            if (zippedModCount > 0 && !(listResources)) {
                std::cout << "Found " << BLUE << zippedModCount << " file(s) " << RESET << "in archive " << YELLOW << zippedMod << RESET << "..." << std::endl;
            }
            modZip.close();
        }
        else {
            continue;
        }
    }

    int unzippedModCount = 0;

    for (const auto& unzippedMod : std::filesystem::recursive_directory_iterator(std::string(argv[1]) + "/Mods")) {
        if (std::filesystem::is_regular_file(unzippedMod) && !(unzippedMod.path().extension() == ".zip")) {
            std::string unzippedModPath = unzippedMod.path();

            std::vector<std::string> modFilePathParts = SplitString(unzippedModPath, '/');

            if (modFilePathParts.size() < 4)
                continue;

            std::string resourceName = modFilePathParts[2];
            std::string fileName;

            if (ToLower(resourceName) == "generated") {
                resourceName = "gameresources";
                fileName = unzippedModPath.substr(modFilePathParts[1].size() + 3, unzippedModPath.size() - modFilePathParts[1].size() - 3);
            }
            else {
                fileName = unzippedModPath.substr(modFilePathParts[1].size() + resourceName.size() + 4, unzippedModPath.size() - resourceName.size() - 4);
            }

            int resourceIndex = GetResourceInfo(resourceName);
            std::string sndPath;

            if (resourceIndex == -1) {
                bool isSnd;

                if (PathToRes(resourceName, isSnd).empty()) {
                    if (PathToRes(modFilePathParts[4], isSnd).empty())
                        continue;
                        
                    ResourceInfo resource(modFilePathParts[4], PathToRes(modFilePathParts[4], isSnd));
                    resource.IsSnd = isSnd;
                    ResourceList.push_back(resource);
                }
                else {
                    ResourceInfo resource(resourceName, PathToRes(resourceName, isSnd));
                    ResourceList.push_back(resource);
                }

                resourceIndex = (int)ResourceList.size() - 1;
            }

            if (!listResources) {
                long unzippedModSize = std::filesystem::file_size(unzippedModPath);

                if (unzippedModSize > ResourceList.max_size()) {
                    std::cerr << "Skipped " << fileName << " - too large." << std::endl;
                }

                FILE *unzippedModFile = fopen(unzippedModPath.c_str(), "rb");

                if (!unzippedModFile)
                    std::cerr << RED << "ERROR: " << RESET << "Failed to open " << unzippedModPath << " for reading." << std::endl;

                std::vector<std::byte> unzippedModBytes(unzippedModSize);
                fread(unzippedModBytes.data(), 1, unzippedModSize, unzippedModFile);

                fclose(unzippedModFile);

                Mod mod(fileName);
                mod.FileBytes = unzippedModBytes;

                if (ToLower(modFilePathParts[3]) == "eternalmod") {
                    if (modFilePathParts.size() == 6
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
        else {
            continue;
        }
    }

    if (unzippedModCount > 0 && !(listResources)) {
        std::cout << "Found " << BLUE << unzippedModCount << " file(s) " << RESET << "in " << YELLOW << "'Mods' " << RESET << "folder..." << std::endl;
    }

    if (listResources) {
        for (auto& resource : ResourceList) {
            if (resource.Path.empty())
                continue;

            std::cout << resource.Path << std::endl;
        }

        return 0;
    }

    long fileSize;

    for (int i = 0; i < ResourceList.size(); i++) {
        if (!ResourceList[i].IsSnd) {
            if (ResourceList[i].Path.empty()) {
                std::cerr << RED << "WARNING: " << YELLOW << ResourceList[i].Name << ".resources" << RESET << " was not found! Skipping " << RED << ResourceList[i].ModList.size() << " file(s)" << RESET << "..." << std::endl;
                continue;
            }

            fileSize = (long)std::filesystem::file_size(ResourceList[i].Path);

            if (fileSize == 0) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << ResourceList[i].Path << RESET << " for writing!" << std::endl;
                continue;
            }

            mmap_allocator_namespace::mmappable_vector<std::byte> mem;

            try {
                mem.mmap_file(ResourceList[i].Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, fileSize,
                    mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << ResourceList[i].Path << RESET << " for writing!" << std::endl;
            }
            
            ReadResource(mem, i);
            ReplaceChunks(mem, i);

            if (std::getenv("ETERNALMODLOADER_SKIP_ADDCHUNKS") == NULL) {
                AddChunks(mem, i);
            }

            mem.munmap_file();
        }
        else {
            if (ResourceList[i].Path.empty()) {
                std::cerr << RED << "WARNING: " << YELLOW << ResourceList[i].Name << ".snd" << RESET << " was not found! Skipping " << RED << ResourceList[i].ModList.size() << " file(s)" << RESET << "..." << std::endl;
                continue;
            }

            int fileCount = 0;

            for (auto& mod : ResourceList[i].ModList) {
                if (LoadSoundMods(mod.FileBytes, ResourceList[i].Path, mod.Name) == -1) {
                    std::cerr << RED << "ERROR: " << RESET << "Failed to inject " << mod.Name << std::endl;
                    continue;
                }

                std::cout << "\tReplaced sound file with id " << std::filesystem::path(mod.Name).stem().string() << std::endl;
                fileCount++;
            }

            std::cout << "Number of files replaced: " << GREEN << fileCount << " sound file(s) " << RESET << "in " << YELLOW << ResourceList[i].Path << RESET << "." << std::endl;
        }
    }

    std::cout << GREEN << "Finished." << RESET << std::endl;
    return 0;
}