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
#include <chrono>

#include "miniz/miniz.h"
#include "EternalModLoader.hpp"

const int32_t Version = 9;
const std::string ResourceDataFileName = "rs_data";
const std::string PackageMapSpecJsonFileName = "packagemapspec.json";
const std::byte *DivinityMagic = (std::byte*)"DIVINITY";

char separator;
std::string BasePath;
bool Verbose = false;
bool SlowMode = false;
bool CompressTextures = false;

std::vector<ResourceContainer> ResourceContainerList;
std::vector<SoundContainer> SoundContainerList;
std::map<uint64_t, ResourceDataEntry> ResourceDataMap;

std::byte *Buffer = NULL;
int64_t BufferSize = -1;

std::string RESET = "";
std::string RED = "";
std::string GREEN = "";
std::string YELLOW = "";
std::string BLUE = "";

int32_t main(int32_t argc, char **argv)
{
    std::ios::sync_with_stdio(false);

    separator = std::filesystem::path::preferred_separator;

    if (std::getenv("ETERNALMODLOADER_NO_COLORS") == NULL) {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
#endif

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
        std::cout << "\t--verbose - Print more information during the mod loading process.\n";
        std::cout << "\t--slow - Slow mod loading mode that produces lighter files.\n";
        std::cout << "\t--compress-textures - Compress texture files during the mod loading process.\n" << std::endl;
        return 1;
    }

    if (!strcmp(argv[1], "--version")) {
        std::cout << Version << std::endl;
        return Version;
    }

    BasePath = std::string(argv[1]) + separator + "base" + separator;

    if (!std::filesystem::exists(BasePath)) {
        std::cerr << RED << "ERROR: " << RESET << "Game directory does not exist!" << std::endl;
        return 1;
    }

    bool listResources = false;

    if (argc > 2) {
        for (int32_t i = 2; i < argc; i++) {
            if (!strcmp(argv[i], "--list-res")) {
                listResources = true;
            }
            else if (!strcmp(argv[i], "--verbose")) {
                Verbose = true;
                std::cout << YELLOW << "INFO: Verbose logging is enabled." << RESET << std::endl;
            }
            else if (!strcmp(argv[i], "--slow")) {
                SlowMode = true;
                std::cout << YELLOW << "INFO: Slow mod loading mode is enabled." << RESET << std::endl;
            }
            else if (!strcmp(argv[i], "--compress-textures")) {
                CompressTextures = true;
                std::cout << YELLOW << "INFO: Texture compression is enabled." << RESET << std::endl;
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
    std::vector<std::string> unzippedMods;

    for (const auto &file : std::filesystem::directory_iterator(std::string(argv[1]) + separator + "Mods")) {
        if (!std::filesystem::is_regular_file(file.path()))
            continue;

        if (file.path().extension() == ".zip" && file.path() == std::string(argv[1]) + separator + "Mods" + separator + file.path().filename().string()) {
            zippedMods.push_back(file.path().string());
        }
        else {
            unzippedMods.push_back(file.path().string());
        }
    }

    std::sort(zippedMods.begin(), zippedMods.end(), [](std::string str1, std::string str2) { return std::strcoll(str1.c_str(), str2.c_str()) <= 0 ? true : false; });
    std::sort(unzippedMods.begin(), unzippedMods.end(), [](std::string str1, std::string str2) { return std::strcoll(str1.c_str(), str2.c_str()) <= 0 ? true : false; });

    for (const auto &zippedMod : zippedMods) {
        int32_t zippedModCount = 0;
        std::vector<std::string> modFileNameList;

        mz_zip_archive modZip;
        mz_zip_zero_struct(&modZip);
        mz_zip_reader_init_file(&modZip, zippedMod.c_str(), 0);

        Mod mod(std::filesystem::path(zippedMod).filename().string());

        if (!listResources) {
            char *unzippedModJson;
            size_t unzippedModJsonSize;

            if ((unzippedModJson = (char*)mz_zip_reader_extract_file_to_heap(&modZip, "EternalMod.json", &unzippedModJsonSize, 0)) != NULL) {
                std::string modJson(unzippedModJson, unzippedModJsonSize);
                free(unzippedModJson);

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

        for (int32_t i = 0; i < modZip.m_total_files; i++) {
            int32_t zipEntryNameSize = mz_zip_reader_get_filename(&modZip, i, NULL, 0);
            char *zipEntryNameBuffer = new char[zipEntryNameSize];

            if (mz_zip_reader_get_filename(&modZip, i, zipEntryNameBuffer, zipEntryNameSize) != zipEntryNameSize || zipEntryNameBuffer == NULL) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to read zip file entry from " << zippedMod << std::endl;
                delete[] zipEntryNameBuffer;
                continue;
            }

            std::string zipEntryName(zipEntryNameBuffer);
            delete[] zipEntryNameBuffer;

            if (0 == zipEntryName.compare(zipEntryName.length() - 1, 1, "/"))
                continue;

            bool isSoundMod = false;
            std::string modFileName = zipEntryName;
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
                int32_t soundContainerIndex = GetSoundContainer(resourceName);

                if (soundContainerIndex == -1) {
                    SoundContainer soundContainer(resourceName, resourcePath);
                    SoundContainerList.push_back(soundContainer);

                    soundContainerIndex = SoundContainerList.size() - 1;
                }

                if (!listResources) {
                    std::string soundExtension = std::filesystem::path(modFileName).extension().string();

                    if (std::find(SupportedFileFormats.begin(), SupportedFileFormats.end(), soundExtension) == SupportedFileFormats.end()) {
                        std::cerr << RED << "WARNING: " << RESET << "Unsupported sound mod file format " << soundExtension << " for file " << modFileName << std::endl;
                        continue;
                    }

                    std::byte *unzippedEntry;
                    size_t unzippedEntrySize;

                    if ((unzippedEntry = (std::byte*)mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0)) == NULL) {
                        std::cerr << RED << "ERROR: " << "Failed to extract zip entry from " << zippedMod << std::endl;
                        continue;
                    }

                    SoundModFile soundModFile(mod, std::filesystem::path(modFileName).filename().string());
                    soundModFile.FileBytes = std::vector<std::byte>(unzippedEntry, unzippedEntry + unzippedEntrySize);
                    free(unzippedEntry);

                    SoundContainerList[soundContainerIndex].ModFileList.push_back(soundModFile);
                    zippedModCount++;
                }
            }
            else {
                int32_t resourceContainerIndex = GetResourceContainer(resourceName);

                if (resourceContainerIndex == -1) {
                    ResourceContainer resource(resourceName, PathToResourceContainer(resourceName + ".resources"));
                    ResourceContainerList.push_back(resource);

                    resourceContainerIndex = ResourceContainerList.size() - 1;
                }

                ResourceModFile resourceModFile(mod, modFileName);

                if (!listResources) {
                    std::byte *unzippedEntry;
                    size_t unzippedEntrySize;

                    if ((unzippedEntry = (std::byte*)mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0)) == NULL) {
                        std::cerr << RED << "ERROR: " << "Failed to extract zip entry from " << zippedMod << std::endl;
                        continue;
                    }

                    resourceModFile.FileBytes = std::vector<std::byte>(unzippedEntry, unzippedEntry + unzippedEntrySize);
                    free(unzippedEntry);
                }

                if (ToLower(modFilePathParts[1]) == "eternalmod") {
                    if (modFilePathParts.size() == 4
                    && ToLower(modFilePathParts[2]) == "assetsinfo"
                    && std::filesystem::path(modFilePathParts[3]).extension() == ".json") {
                        try {
                            if (listResources) {
                                std::byte *unzippedEntry;
                                size_t unzippedEntrySize;

                                if ((unzippedEntry = (std::byte*)mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0)) == NULL) {
                                    std::cerr << RED << "ERROR: " << "Failed to extract zip entry from " << zippedMod << std::endl;
                                    continue;
                                }

                                resourceModFile.FileBytes = std::vector<std::byte>(unzippedEntry, unzippedEntry + unzippedEntrySize);
                                free(unzippedEntry);
                            }

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
            
        if (zippedModCount > 0 && !listResources)
            std::cout << "Found " << BLUE << zippedModCount << " file(s) " << RESET << "in archive " << YELLOW << zippedMod << RESET << "..." << std::endl;

        mz_zip_reader_end(&modZip);
    }

    int32_t unzippedModCount = 0;

    Mod globalLooseMod;
    globalLooseMod.LoadPriority = INT_MIN;

    for (const auto &unzippedMod : unzippedMods) {
        std::string unzippedModPath = unzippedMod;
        std::vector<std::string> modFilePathParts = SplitString(unzippedModPath, separator);

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
            int32_t soundContainerIndex = GetSoundContainer(resourceName);

            if (soundContainerIndex == -1) {
                SoundContainer soundContainer(resourceName, resourcePath);
                SoundContainerList.push_back(soundContainer);

                soundContainerIndex = SoundContainerList.size() - 1;
            }

            if (!listResources) {
                std::string soundExtension = std::filesystem::path(fileName).extension().string();

                if (std::find(SupportedFileFormats.begin(), SupportedFileFormats.end(), soundExtension) == SupportedFileFormats.end()) {
                    std::cerr << RED << "WARNING: " << RESET << "Unsupported sound mod file format " << soundExtension << " for file " << fileName << std::endl;
                    continue;
                }

                int64_t unzippedModSize = std::filesystem::file_size(unzippedModPath);

                if (unzippedModSize > ResourceContainerList.max_size())
                    std::cerr << RED << "WARNING: " << RESET << "Skipped " << fileName << " - too large." << std::endl;
                
                SoundModFile soundModFile(globalLooseMod, std::filesystem::path(fileName).filename().string());
                
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
            int32_t resourceContainerIndex = GetResourceContainer(resourceName);

            if (resourceContainerIndex == -1) {
                ResourceContainer resourceContainer(resourceName, PathToResourceContainer(resourceName));
                ResourceContainerList.push_back(resourceContainer);

                resourceContainerIndex = ResourceContainerList.size() - 1;
            }

            ResourceModFile resourceModFile(globalLooseMod, fileName);

            if (!listResources) {
                int64_t unzippedModSize = std::filesystem::file_size(unzippedModPath);

                if (unzippedModSize > ResourceContainerList.max_size())
                    std::cerr << RED << "WARNING: " << RESET << "Skipped " << fileName << " - too large." << std::endl;

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
            }

            if (ToLower(modFilePathParts[3]) == "eternalmod") {
                if (modFilePathParts.size() == 6
                && ToLower(modFilePathParts[4]) == "assetsinfo"
                && std::filesystem::path(modFilePathParts[5]).extension() == ".json") {
                    try {
                        if (listResources) {
                            int64_t unzippedModSize = std::filesystem::file_size(unzippedModPath);

                            if (unzippedModSize > ResourceContainerList.max_size())
                                std::cerr << RED << "WARNING: " << RESET << "Skipped " << fileName << " - too large." << std::endl;

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
                        }

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

    if (unzippedModCount > 0 && !(listResources))
        std::cout << "Found " << BLUE << unzippedModCount << " file(s) " << RESET << "in " << YELLOW << "'Mods' " << RESET << "folder..." << std::endl;

    if (listResources) {
        for (auto &resourceContainer : ResourceContainerList) {
            if (resourceContainer.Path.empty())
                continue;

            bool shouldListResource = false;

            for (auto &modFile : resourceContainer.ModFileList) {
                if (!modFile.IsAssetsInfoJson) {
                    shouldListResource = true;
                    break;
                }

                if (!modFile.AssetsInfo.has_value())
                    continue;

                if (modFile.AssetsInfo.value().Assets.empty()
                    && modFile.AssetsInfo.value().Layers.empty()
                    && modFile.AssetsInfo.value().Maps.empty())
                        continue;

                shouldListResource = true;
                break;
            }

            if (shouldListResource)
                std::cout << resourceContainer.Path << std::endl;
        }

        for (auto &soundContainer : SoundContainerList) {
            if (soundContainer.Path.empty())
                continue;

            std::cout << soundContainer.Path << std::endl;
        }

        return 0;
    }

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    for (auto &resourceContainer : ResourceContainerList) {
        if (resourceContainer.Path.empty()) {
            std::cerr << RED << "WARNING: " << YELLOW << resourceContainer.Name << ".resources" << RESET << " was not found! Skipping " << RED << resourceContainer.ModFileList.size() << " file(s)" << RESET << "..." << std::endl;
            continue;
        }

        int64_t fileSize = std::filesystem::file_size(resourceContainer.Path);

        if (fileSize == 0) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        if (BufferSize == -1 || Buffer == NULL) {
            try {
                SetOptimalBufferSize(std::filesystem::absolute(resourceContainer.Path).root_path().string());
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Error while determining the optimal buffer size, using 4096 as the default." << std::endl;

                if (Buffer != NULL)
                    delete[] Buffer;

                Buffer = new std::byte[4096];
            }
        }

#ifdef _WIN32
        HANDLE hFile = CreateFileA(resourceContainer.Path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (GetLastError() != ERROR_SUCCESS || hFile == INVALID_HANDLE_VALUE) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        HANDLE fileMapping = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, *((DWORD*)&fileSize + 1), *(DWORD*)&fileSize, NULL);

        if (GetLastError() != ERROR_SUCCESS || fileMapping == NULL) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        std::byte *mem = (std::byte*)MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        if (GetLastError() != ERROR_SUCCESS || mem == NULL) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        ReadResource(mem, resourceContainer);
        ReplaceChunks(mem, hFile, fileMapping, resourceContainer);
        AddChunks(mem, hFile, fileMapping, resourceContainer);

        UnmapViewOfFile(mem);
        CloseHandle(fileMapping);
        CloseHandle(hFile);
#else
        int32_t fd = open(resourceContainer.Path.c_str(), O_RDWR);

        if (fd == -1) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        std::byte *mem = (std::byte*)mmap(0, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        if (mem == NULL) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
            close(fd);
            continue;
        }

        madvise(mem, fileSize, MADV_WILLNEED);

        ReadResource(mem, resourceContainer);
        ReplaceChunks(mem, fd, resourceContainer);
        AddChunks(mem, fd, resourceContainer);

        munmap(mem, std::filesystem::file_size(resourceContainer.Path));
        close(fd);
#endif
    }

    if (Buffer != NULL)
        delete[] Buffer;

    for (auto &soundContainer : SoundContainerList) {
        if (soundContainer.Path.empty()) {
            std::cerr << RED << "WARNING: " << YELLOW << soundContainer.Name << ".resources" << RESET << " was not found! Skipping " << RED << soundContainer.ModFileList.size() << " file(s)" << RESET << "..." << std::endl;
            continue;
        }

        int64_t fileSize = std::filesystem::file_size(soundContainer.Path);

        if (fileSize == 0) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

#ifdef _WIN32
        HANDLE hFile = CreateFileA(soundContainer.Path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,  FILE_ATTRIBUTE_NORMAL, NULL);

        if (GetLastError() != ERROR_SUCCESS || hFile == INVALID_HANDLE_VALUE) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        HANDLE fileMapping = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, *((DWORD*)&fileSize + 1), *(DWORD*)&fileSize, NULL);

        if (GetLastError() != ERROR_SUCCESS || fileMapping == NULL) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        std::byte *mem = (std::byte*)MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        if (GetLastError() != ERROR_SUCCESS || mem == NULL) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        ReadSoundEntries(mem, soundContainer);
        ReplaceSounds(mem, hFile, fileMapping, soundContainer);

        UnmapViewOfFile(mem);
        CloseHandle(fileMapping);
        CloseHandle(hFile);
#else
        int32_t fd = open(soundContainer.Path.c_str(), O_RDWR);

        if (fd == -1) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
            continue;
        }

        std::byte *mem = (std::byte*)mmap(0, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        if (mem == NULL) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
            close(fd);
            continue;
        }

        madvise(mem, fileSize, MADV_WILLNEED);

        ReadSoundEntries(mem, soundContainer);
        ReplaceSounds(mem, fd, soundContainer);

        munmap(mem, std::filesystem::file_size(soundContainer.Path));
        close(fd);
#endif
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    double time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0;

    std::cout << GREEN << "Finished in " << time << " seconds." << RESET << std::endl;
    return 0;
}
