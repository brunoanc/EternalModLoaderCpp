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
#include <thread>

#include "EternalModLoader.hpp"

namespace chrono = std::chrono;

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

std::vector<std::stringstream> stringStreams;
int32_t streamIndex = 0;

std::byte *Buffer = NULL;
int64_t BufferSize = -1;

std::string RESET = "";
std::string RED = "";
std::string GREEN = "";
std::string YELLOW = "";
std::string BLUE = "";

int main(int argc, char **argv)
{
    std::ios::sync_with_stdio(false);

    char coutBuf[8192];
    std::cout.rdbuf()->pubsetbuf(coutBuf, 8192);

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
                std::cerr << RED << "ERROR: " << RESET << "Failed to parse " << ResourceDataFileName << '\n';
            }
        }
        else {
            if (Verbose) {
                std::cerr << RED << "WARNING: " << RESET << ResourceDataFileName << " was not found! There will be issues when adding existing new assets to containers..." << '\n';
            }
        }
    }

    std::vector<std::string> zippedMods;
    std::vector<std::string> unzippedMods;

    for (const auto &file : std::filesystem::recursive_directory_iterator(std::string(argv[1]) + separator + "Mods")) {
        if (!std::filesystem::is_regular_file(file.path()))
            continue;

        if (file.path().extension() == ".zip" && file.path() == std::string(argv[1]) + separator + "Mods" + separator + file.path().filename().string()) {
            zippedMods.push_back(file.path().string());
        }
        else if (file.path().extension() != ".zip") {
            unzippedMods.push_back(file.path().string());
        }
    }

    GetResourceContainerPathList();

    chrono::steady_clock::time_point zippedModsBegin = chrono::steady_clock::now();

    std::vector<std::thread> zippedModLoadingThreads;
    zippedModLoadingThreads.reserve(zippedMods.size());

    for (const auto &zippedMod : zippedMods)
        zippedModLoadingThreads.push_back(std::thread(LoadZippedMod, zippedMod, listResources));

    for (auto &thread : zippedModLoadingThreads)
        thread.join();

    chrono::steady_clock::time_point zippedModsEnd = chrono::steady_clock::now();
    double zippedModsTime = chrono::duration_cast<chrono::microseconds>(zippedModsEnd - zippedModsBegin).count() / 1000000.0;

    chrono::steady_clock::time_point unzippedModsBegin = chrono::steady_clock::now();

    std::vector<std::thread> unzippedModLoadingThreads;
    unzippedModLoadingThreads.reserve(unzippedMods.size());

    std::atomic<int32_t> unzippedModCount = 0;
    Mod globalLooseMod;
    globalLooseMod.LoadPriority = INT_MIN;

    for (const auto &unzippedMod : unzippedMods)
        unzippedModLoadingThreads.push_back(std::thread(LoadUnzippedMod, unzippedMod, listResources, std::ref(globalLooseMod), std::ref(unzippedModCount)));

    for (auto &thread : unzippedModLoadingThreads)
        thread.join();

    if (unzippedModCount > 0 && !listResources)
        std::cout << "Found " << BLUE << unzippedModCount << " file(s) " << RESET << "in " << YELLOW << "'Mods' " << RESET << "folder..." << '\n';

    chrono::steady_clock::time_point unzippedModsEnd = chrono::steady_clock::now();
    double unzippedModsTime = chrono::duration_cast<chrono::microseconds>(unzippedModsEnd - unzippedModsBegin).count() / 1000000.0;

    std::cout.flush();

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
                std::cout << resourceContainer.Path << '\n';
        }

        for (auto &soundContainer : SoundContainerList) {
            if (soundContainer.Path.empty())
                continue;

            std::cout << soundContainer.Path << '\n';
        }

        std::cout.flush();
        return 0;
    }

    chrono::steady_clock::time_point modLoadingBegin = chrono::steady_clock::now();

    stringStreams.resize(ResourceContainerList.size() + SoundContainerList.size());

    if (!SlowMode) {
        std::vector<std::thread> modLoadingThreads;
        modLoadingThreads.reserve(ResourceContainerList.size() + SoundContainerList.size());

        for (auto &resourceContainer : ResourceContainerList)
            modLoadingThreads.push_back(std::thread(LoadResourceMods, std::ref(resourceContainer)));

        for (auto &soundContainer : SoundContainerList)
            modLoadingThreads.push_back(std::thread(LoadSoundMods, std::ref(soundContainer)));

        for (int i = 0; i < modLoadingThreads.size(); i++) {
            modLoadingThreads[i].join();
            std::cout << stringStreams[i].rdbuf();
        }
    }
    else {
        for (auto &resourceContainer : ResourceContainerList)
            LoadResourceMods(resourceContainer);

        for (auto &soundContainer : SoundContainerList)
            LoadSoundMods(soundContainer);
    }

    ModifyPackageMapSpec();

    if (Buffer != NULL)
        delete[] Buffer;

    chrono::steady_clock::time_point modLoadingEnd = chrono::steady_clock::now();
    double modLoadingTime = chrono::duration_cast<chrono::microseconds>(modLoadingEnd - modLoadingBegin).count() / 1000000.0;

    if (Verbose) {
        std::cout << GREEN << "Zipped mods loaded in " << zippedModsTime << " seconds.\n";
        std::cout << "Unzipped mods loaded in " << unzippedModsTime << " seconds.\n";
        std::cout << "Injection finished in " << modLoadingTime << " seconds.\n";
    }

    std::cout << GREEN << "Total time taken: " << zippedModsTime + unzippedModsTime + modLoadingTime << " seconds." << RESET << std::endl;

    return 0;
}
