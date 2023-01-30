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
#include <algorithm>
#include <cstring>
#include <climits>
#include <chrono>
#include <thread>
#include <filesystem>
#include <mutex>
#include "Colors.hpp"
#include "LoadModFiles.hpp"
#include "LoadMods.hpp"
#include "OnlineSafety.hpp"
#include "Oodle.hpp"
#include "PackageMapSpecInfo.hpp"
#include "ProgramOptions.hpp"
#include "ResourceContainer.hpp"
#include "ResourceData.hpp"
#include "SoundContainer.hpp"
#include "StreamDBContainer.hpp"
#include "Utils.hpp"
#include "PathToResource.hpp"

namespace fs = std::filesystem;
namespace chrono = std::chrono;

std::mutex mtx;

int main(int argc, char **argv)
{
    // Disable sync with stdio
    std::ios::sync_with_stdio(false);

    // Make cout fully buffered to increase program speed
    char coutBuf[8192];
    std::cout.rdbuf()->pubsetbuf(coutBuf, 8192);

    // Enable colors
    if (std::getenv("ETERNALMODLOADER_NO_COLORS") == NULL) {
        Colors::EnableColors();
    }
    else {
        Colors::DisableColors();
    }

    // Display help
    if (argc == 1) {
        std::cout << "EternalModLoaderCpp by PowerBall253, based on EternalModLoader by proteh\n\n";
        std::cout << "Loads DOOM Eternal mods from ZIPs or loose files in 'Mods' folder into the .resources files in the specified directory.\n\n";
        std::cout << "USAGE: " << argv[0] << " <game path | --version> [OPTIONS]\n";
        std::cout << "\t--version - Prints the version number of the mod loader and exits with exit code same as the version number.\n\n";
        std::cout << "OPTIONS:\n";
        std::cout << "\t--list-res - List the .resources files that will be modified and exit.\n";
        std::cout << "\t--verbose - Print more information during the mod loading process.\n";
        std::cout << "\t--slow - Slow mod loading mode that produces lighter files.\n";
        std::cout << "\t--online-safe - Only load online-safe mods.\n";
        std::cout << "\t--compress-textures - Compress texture files during the mod loading process.\n";
        std::cout << "\t--disable-multithreading - Disables multi-threaded mod loading." << std::endl;
        return 1;
    }

    // Display and return program version
    if (!strcmp(argv[1], "--version")) {
        std::cout << VERSION << std::endl;
        return VERSION;
    }

    // Get program options
    auto argsOutput = ProgramOptions::GetProgramOptions(argv, argc);

    if (ProgramOptions::BasePath.empty()) {
        return 1;
    }

    if (!ProgramOptions::ListResources) {
        std::cout << argsOutput.str();
        std::cout.flush();
    }

    // Init oodle
    if (!Oodle::Init(ProgramOptions::BasePath)) {
        std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Failed to initialize oodle." << std::endl;
        return 1;
    }

    // Parse rs_data
    std::map<uint64_t, ResourceDataEntry> resourceDataMap;

    if (!ProgramOptions::ListResources) {
        std::string resourceDataFilePath = ProgramOptions::BasePath + "rs_data";

        if (fs::exists(resourceDataFilePath)) {
            try {
                resourceDataMap = ParseResourceData(resourceDataFilePath);

                if (resourceDataMap.empty()) {
                    throw std::exception();
                }
            }
            catch (...) {
                std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Failed to parse rs_data" << '\n';
            }
        }
        else {
            if (ProgramOptions::Verbose) {
                std::cout << Colors::Red << "WARNING: " << Colors::Reset << "rs_data was not found! There will be issues when adding existing new assets to containers..." << '\n';
            }
        }
    }

    // Find mods
    std::vector<std::string> zippedMods;
    std::vector<std::string> unzippedMods;
    std::vector<std::string> notFoundContainers;

    for (const auto& file : fs::recursive_directory_iterator(std::string(argv[1]) + SEPARATOR + "Mods")) {
        if (!fs::is_regular_file(file.path())) {
            continue;
        }

        if (file.path().extension() == ".zip" && file.path() == std::string(argv[1]) + SEPARATOR + "Mods" + SEPARATOR + file.path().filename().string()) {
            zippedMods.push_back(file.path().string());
        }
        else if (file.path().extension() != ".zip") {
            unzippedMods.push_back(file.path().string());
        }
    }

    // Get the resource container paths
    GetResourceContainerPathList();

    // Store all containers used by mods
    std::vector<ResourceContainer> resourceContainerList;
    std::vector<SoundContainer> soundContainerList;
    std::vector<StreamDBContainer> streamDBContainerList;

    // Reserve enough space for all resource/sound containers
    resourceContainerList.reserve(100);
    soundContainerList.reserve(30);

    // Load zipped mods
    chrono::steady_clock::time_point zippedModsBegin = chrono::steady_clock::now();

    if (ProgramOptions::MultiThreading) {
        std::vector<std::thread> zippedModLoadingThreads;
        zippedModLoadingThreads.reserve(zippedMods.size());

        for (const auto& zippedMod : zippedMods) {
            zippedModLoadingThreads.push_back(std::thread(LoadZippedMod, zippedMod, std::ref(resourceContainerList),
                std::ref(soundContainerList), std::ref(streamDBContainerList), std::ref(notFoundContainers)));
        }

        for (auto& thread : zippedModLoadingThreads) {
            thread.join();
        }
    }
    else {
        for (const auto& zippedMod : zippedMods) {
            LoadZippedMod(zippedMod, resourceContainerList, soundContainerList, streamDBContainerList, notFoundContainers);
        }
    }

    chrono::steady_clock::time_point zippedModsEnd = chrono::steady_clock::now();
    double zippedModsTime = chrono::duration_cast<chrono::microseconds>(zippedModsEnd - zippedModsBegin).count() / 1000000.0;

    // Load unzipped mods
    chrono::steady_clock::time_point unzippedModsBegin = chrono::steady_clock::now();

    std::atomic<size_t> unzippedModCount = 0;
    std::map<size_t, std::vector<ResourceModFile>> resourceModFiles;
    std::map<size_t, std::vector<SoundModFile>> soundModFiles;
    std::map<size_t, std::vector<StreamDBModFile>> streamDBModFiles;
    Mod globalLooseMod;
    globalLooseMod.LoadPriority = INT_MIN;

    if (ProgramOptions::MultiThreading) {
        std::vector<std::thread> unzippedModLoadingThreads;
        unzippedModLoadingThreads.reserve(unzippedMods.size());

        for (const auto& unzippedMod : unzippedMods) {
            unzippedModLoadingThreads.push_back(std::thread(LoadUnzippedMod, unzippedMod, std::ref(globalLooseMod),
                std::ref(unzippedModCount), std::ref(resourceModFiles), std::ref(soundModFiles), std::ref(streamDBModFiles),
                std::ref(resourceContainerList), std::ref(soundContainerList), std::ref(streamDBContainerList),
                std::ref(notFoundContainers)));
        }

        for (auto& thread : unzippedModLoadingThreads) {
            thread.join();
        }
    }
    else {
        for (const auto& unzippedMod : unzippedMods) {
            LoadUnzippedMod(unzippedMod, globalLooseMod, unzippedModCount, resourceModFiles, soundModFiles,
                streamDBModFiles, resourceContainerList, soundContainerList, streamDBContainerList, notFoundContainers);
        }
    }

    // Check if the unzipped mods are safe for online play
    if (!IsModSafeForOnline(resourceModFiles)) {
        // Mods are not safe for online
        // Check if they should be loaded
        ProgramOptions::AreModsSafeForOnline = false;
        globalLooseMod.IsSafeForOnline = false;

        if (!ProgramOptions::LoadOnlineSafeModsOnly) {
            // Inject online disabler mods
            for (const auto& resourceMod : resourceModFiles) {
                ResourceContainer& resourceContainer = resourceContainerList[resourceMod.first];
                resourceContainer.ModFileList.insert(resourceContainer.ModFileList.end(), resourceMod.second.begin(), resourceMod.second.end());
            }

            for (const auto& soundMod : soundModFiles) {
                SoundContainer& soundContainer = soundContainerList[soundMod.first];
                soundContainer.ModFileList.insert(soundContainer.ModFileList.end(), soundMod.second.begin(), soundMod.second.end());
            }

            for (const auto& streamDBMod : streamDBModFiles) {
                auto& streamDBContainer = streamDBContainerList[streamDBMod.first];
                streamDBContainer.ModFiles.insert(streamDBContainer.ModFiles.end(), streamDBMod.second.begin(), streamDBMod.second.end());
            }
        }
    }
    else {
        // Inject mods
        for (const auto& resourceMod : resourceModFiles) {
            ResourceContainer& resourceContainer = resourceContainerList[resourceMod.first];
            resourceContainer.ModFileList.insert(resourceContainer.ModFileList.end(), resourceMod.second.begin(), resourceMod.second.end());
        }

        for (const auto& soundMod : soundModFiles) {
            SoundContainer& soundContainer = soundContainerList[soundMod.first];
            soundContainer.ModFileList.insert(soundContainer.ModFileList.end(), soundMod.second.begin(), soundMod.second.end());
        }

        for (const auto& streamDBMod : streamDBModFiles) {
            auto& streamDBContainer = streamDBContainerList[streamDBMod.first];
            streamDBContainer.ModFiles.insert(streamDBContainer.ModFiles.end(), streamDBMod.second.begin(), streamDBMod.second.end());
        }
    }

    if (unzippedModCount > 0 && !ProgramOptions::ListResources) {
        if (ProgramOptions::LoadOnlineSafeModsOnly && !globalLooseMod.IsSafeForOnline) {
            std::cout << Colors::Red << "WARNING: " << Colors::Reset << "Loose mod files are not safe for public matchmaking, skipping" << '\n';
        }
        else {
            std::cout << "Found " << Colors::Blue << unzippedModCount << " file(s) " << Colors::Reset << "in " << Colors::Yellow << "'Mods' " << Colors::Reset << "folder..." << '\n';

            if (!globalLooseMod.IsSafeForOnline) {
                std::cout << Colors::Yellow << "WARNING: Loose mod files are not safe for online play, public matchmaking will be disabled" << Colors::Reset << '\n';
            }
        }
    }

    chrono::steady_clock::time_point unzippedModsEnd = chrono::steady_clock::now();
    double unzippedModsTime = chrono::duration_cast<chrono::microseconds>(unzippedModsEnd - unzippedModsBegin).count() / 1000000.0;

    // Remove resources from the list if they have no mods to load
    for (auto i = static_cast<ssize_t>(resourceContainerList.size()) - 1; i >= 0; i--) {
        if (resourceContainerList[i].ModFileList.empty()) {
            resourceContainerList.erase(resourceContainerList.begin() + i);
        }
    }

    // Disable multiplayer if needed
    if (!ProgramOptions::AreModsSafeForOnline && !ProgramOptions::LoadOnlineSafeModsOnly) {
        std::vector<ResourceModFile> multiplayerDisablerMods = GetMultiplayerDisablerMods();

        for (auto& resourceContainer : resourceContainerList) {
            for (auto i = static_cast<ssize_t>(resourceContainer.ModFileList.size()) - 1; i >= 0; i--) {
                for (auto& modFile: multiplayerDisablerMods) {
                    if (!modFile.IsBlangJson && !modFile.IsAssetsInfoJson && modFile.Name == resourceContainer.ModFileList[i].Name) {
                        resourceContainer.ModFileList.erase(resourceContainer.ModFileList.begin() + i);
                    }
                }
            }
        }

        for (auto& modFile : multiplayerDisablerMods) {
            bool found = false;

            for (auto& resourceContainer : resourceContainerList) {
                if (modFile.ResourceName == resourceContainer.Name) {
                    found = true;
                    resourceContainer.ModFileList.push_back(modFile);
                    break;
                }
            }

            if (!found) {
                ResourceContainer resourceContainer(modFile.ResourceName, PathToResourceContainer(modFile.ResourceName + ".resources"));
                resourceContainer.ModFileList.push_back(modFile);
                resourceContainerList.push_back(resourceContainer);
            }
        }
    }

    // List resources to be modified and exit
    if (ProgramOptions::ListResources) {
        // Print the packagemapspec path if the modded streamdb was added
        bool printPackageMapSpecJsonPath = std::find_if(streamDBContainerList.begin(), streamDBContainerList.end(),
            [](const StreamDBContainer& streamDBContainer) { return streamDBContainer.Name == "EternalMod.streamdb"; }) != streamDBContainerList.end();

        for (auto& resourceContainer : resourceContainerList) {
            if (resourceContainer.Path.empty()) {
                continue;
            }

            bool shouldListResource = false;

            for (auto& modFile : resourceContainer.ModFileList) {
                if (!modFile.IsAssetsInfoJson) {
                    shouldListResource = true;

                    if (printPackageMapSpecJsonPath) {
                        break;
                    }
                    else {
                        continue;
                    }
                }

                if (!modFile.AssetsInfo.has_value()) {
                    continue;
                }

                if (!shouldListResource) {
                    if (!modFile.AssetsInfo.value().Assets.empty()
                        || !modFile.AssetsInfo.value().Layers.empty()
                        || !modFile.AssetsInfo.value().Maps.empty()) {
                            shouldListResource = true;
                    }
                }

                if (!printPackageMapSpecJsonPath) {
                    if (!modFile.AssetsInfo.value().Resources.empty()) {
                        printPackageMapSpecJsonPath = true;
                    }
                }

                if (shouldListResource && printPackageMapSpecJsonPath) {
                    break;
                }
            }

            if (shouldListResource) {
                std::cout << resourceContainer.Path << '\n';
            }
        }

        if (printPackageMapSpecJsonPath) {
            std::cout << ProgramOptions::BasePath + "packagemapspec.json" << '\n';
        }

        for (auto& soundContainer : soundContainerList) {
            if (soundContainer.Path.empty()) {
                continue;
            }

            std::cout << soundContainer.Path << '\n';
        }

        for (auto& streamDBContainer : streamDBContainerList) {
            if (streamDBContainer.Path.empty()) {
                continue;
            }

            std::cout << streamDBContainer.Path << '\n';
        }

        std::cout.flush();
        return 0;
    }

    // Display not found containers
    for (auto& container : notFoundContainers) {
        std::cout << Colors::Red << "WARNING: " << Colors::Yellow << container << Colors::Reset << " was not found! Skipping..." << std::endl;
    }

    std::cout.flush();

    // Set buffer for file i/o
    int bufferSize = 4096;

    try {
        bufferSize = GetClusterSize();

        if (bufferSize == -1) {
            throw std::exception();
        }
    }
    catch (...) {
        std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Error while determining the optimal buffer size, using 4096 as the default." << std::endl;
    }

    auto buffer = std::make_unique<std::byte[]>(bufferSize);

    // Load mods
    chrono::steady_clock::time_point modLoadingBegin = chrono::steady_clock::now();

    InitStringStreams(resourceContainerList.size() + soundContainerList.size() + streamDBContainerList.size());

    if (ProgramOptions::MultiThreading) {
        std::vector<std::thread> modLoadingThreads;
        modLoadingThreads.reserve(resourceContainerList.size() + soundContainerList.size() + streamDBContainerList.size());

        for (auto& resourceContainer : resourceContainerList) {
            modLoadingThreads.push_back(std::thread(LoadResourceMods, std::ref(resourceContainer),
                std::ref(resourceDataMap), std::ref(buffer), bufferSize));
        }

        for (auto& soundContainer : soundContainerList) {
            modLoadingThreads.push_back(std::thread(LoadSoundMods, std::ref(soundContainer)));
        }

        for (auto& streamDBContainer : streamDBContainerList) {
            modLoadingThreads.push_back(std::thread(LoadStreamDBMods, std::ref(streamDBContainer), std::ref(streamDBContainerList)));
        }

        for (size_t i = 0; i < modLoadingThreads.size(); i++) {
            modLoadingThreads[i].join();
            OutputStringStream(i);
        }
    }
    else {
        for (auto& resourceContainer : resourceContainerList) {
            LoadResourceMods(resourceContainer, resourceDataMap, buffer, bufferSize);
        }

        for (auto& soundContainer : soundContainerList) {
            LoadSoundMods(soundContainer);
        }

        for (auto& streamDBContainer : streamDBContainerList) {
            LoadStreamDBMods(streamDBContainer, streamDBContainerList);
        }
    }

    // Modify PackageMapSpec JSON file in disk
    if (!PackageMapSpecInfo::ModifyPackageMapSpec(ProgramOptions::BasePath, streamDBContainerList)) {
        std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Failed to write " << PackageMapSpecInfo::PackageMapSpecPath << std::endl;
    }
    else {
        std::cout << "Modified "<< Colors::Yellow << PackageMapSpecInfo::PackageMapSpecPath << Colors::Reset << '\n';
    }

    // Display metrics
    chrono::steady_clock::time_point modLoadingEnd = chrono::steady_clock::now();
    double modLoadingTime = chrono::duration_cast<chrono::microseconds>(modLoadingEnd - modLoadingBegin).count() / 1000000.0;

    if (ProgramOptions::Verbose) {
        std::cout << Colors::Green << "Zipped mods loaded in " << zippedModsTime << " seconds.\n";
        std::cout << "Unzipped mods loaded in " << unzippedModsTime << " seconds.\n";
        std::cout << "Injection finished in " << modLoadingTime << " seconds.\n";
    }

    std::cout << Colors::Green << "Total time taken: " << zippedModsTime + unzippedModsTime + modLoadingTime << " seconds." << Colors::Reset << std::endl;

    // Exit the program with error code 0
    return 0;
}
