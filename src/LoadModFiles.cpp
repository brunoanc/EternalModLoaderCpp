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
#include <filesystem>
#include <mutex>
#include "Colors.hpp"
#include "GetObject.hpp"
#include "OnlineSafety.hpp"
#include "PathToResource.hpp"
#include "ProgramOptions.hpp"
#include "Utils.hpp"
#include "LoadModFiles.hpp"
#include "miniz/miniz.h"

namespace fs = std::filesystem;

extern std::mutex mtx;

// Supported sound file formats
const std::vector<std::string> SupportedSoundFormats{ ".ogg", ".opus", ".wav", ".wem", ".flac", ".aiff", ".pcm" };

void LoadZippedMod(std::string zippedMod,
    std::vector<ResourceContainer>& resourceContainerList, std::vector<SoundContainer>& soundContainerList,
    std::vector<StreamDBContainer>& streamDBContainerList, std::vector<std::string>& notFoundContainers)
{
    size_t zippedModCount = 0;
    std::vector<std::string> modFileNameList;
    std::map<size_t, std::vector<ResourceModFile>> resourceModFiles;
    std::map<size_t, std::vector<SoundModFile>> soundModFiles;
    std::map<size_t, std::vector<StreamDBModFile>> streamDBModFiles;

    // Load zipped mod
    mz_zip_archive modZip;
    mz_zip_zero_struct(&modZip);
    mz_zip_reader_init_file(&modZip, zippedMod.c_str(), 0);

    Mod mod;

    if (!ProgramOptions::ListResources) {
        // Read the mod info from the EternalMod JSON if it exists
        char *unzippedModJson;
        size_t unzippedModJsonSize;

        if ((unzippedModJson = static_cast<char*>(mz_zip_reader_extract_file_to_heap(&modZip, "EternalMod.json", &unzippedModJsonSize, 0))) != nullptr) {
            std::string modJson(unzippedModJson, unzippedModJsonSize);
            free(unzippedModJson);

            try {
                // Try to parse the JSON
                mod = Mod(modJson);

                // If the mod requires a higher mod loader version, print a warning and don't load the mod
                if (mod.RequiredVersion > VERSION) {
                    mtx.lock();
                    std::cout << Colors::Red << "WARNING: " << Colors::Reset << "Mod " << fs::path(zippedMod).filename().string() << " requires mod loader version "
                        << mod.RequiredVersion << " but the current mod loader version is " << VERSION << ", skipping" << '\n';
                    mtx.unlock();
                    return;
                }
            }
            catch (...) {
                mtx.lock();
                std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Failed to parse EternalMod.json - using defaults." << '\n';
                mtx.unlock();
            }
        }
    }

    // Iterate through the zip's files
    for (unsigned int i = 0; i < modZip.m_total_files; i++) {
        // Get the mod file's name
        unsigned int zipEntryNameSize = mz_zip_reader_get_filename(&modZip, i, nullptr, 0);
        auto zipEntryNameBuffer = std::make_unique<char[]>(zipEntryNameSize);

        if (mz_zip_reader_get_filename(&modZip, i, zipEntryNameBuffer.get(), zipEntryNameSize) != zipEntryNameSize || zipEntryNameBuffer == nullptr) {
            mtx.lock();
            std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Failed to read zip file entry from " << zippedMod << '\n';
            mtx.unlock();
            continue;
        }

        std::string zipEntryName(zipEntryNameBuffer.get());

        // Skip directories
        if (0 == zipEntryName.compare(zipEntryName.length() - 1, 1, "/")) {
            continue;
        }

        // Determine the game container for each mod file
        bool isSoundMod = false;
        bool isStreamDBMod = false;
        std::string modFileName = zipEntryName;
        std::vector<std::string> modFilePathParts = SplitString(modFileName, '/');

        if (modFilePathParts.size() < 2) {
            continue;
        }

        std::string resourceName = modFilePathParts[0];

        // Old mods compatibility
        if (ToLower(resourceName) == "generated") {
            resourceName = "gameresources";
        }
        else {
            // Remove the resource name from the name
            modFileName = modFileName.substr(resourceName.size() + 1, modFileName.size() - resourceName.size() - 1);
        }

        // Redirect .blang files to a different container if specified
        if (!ProgramOptions::BlangFileContainerRedirect.empty() && modFilePathParts[1] == "EternalMod" && modFilePathParts[2] == "strings") {
            resourceName = ProgramOptions::BlangFileContainerRedirect;
        }

        // Get path to resource file
        std::string resourcePath = PathToResourceContainer(resourceName + ".resources");

        // Check if this is a streamdb mod
        if (resourceName == "streamdb") {
            isStreamDBMod = true;
            resourcePath = ProgramOptions::BasePath + "EternalMod.streamdb";
        }

        // Check if this is a sound mod
        if (resourcePath.empty() && !isStreamDBMod) {
            resourcePath = PathToSoundContainer(resourceName);

            if (!resourcePath.empty()) {
                isSoundMod = true;
            }
            else {
                mtx.lock();

                if (std::find(notFoundContainers.begin(), notFoundContainers.end(), resourceName) == notFoundContainers.end()) {
                    notFoundContainers.push_back(resourceName);
                }

                mtx.unlock();
                continue;
            }
        }

        if (isStreamDBMod) {
            // Get the streamdb container info object, create it if it doesn't exist
            mtx.lock();

            auto x = std::find_if(streamDBContainerList.begin(), streamDBContainerList.end(),
                [](const StreamDBContainer& streamDbContainer) { return streamDbContainer.Name == "EternalMod.streamdb"; });

            if (x == streamDBContainerList.end()) {
                streamDBContainerList.push_back(StreamDBContainer("EternalMod.streamdb", resourcePath));
                x = streamDBContainerList.end() - 1;
            }

            auto streamDBContainerIndex = std::distance(streamDBContainerList.begin(), x);

            mtx.unlock();

            if (!ProgramOptions::ListResources) {
                // Load the streamdb mod
                std::byte *unzippedEntry;
                size_t unzippedEntrySize;

                if ((unzippedEntry = reinterpret_cast<std::byte*>(mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0))) == nullptr) {
                    mtx.lock();
                    std::cout << Colors::Red << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
                    mtx.unlock();
                    continue;
                }

                StreamDBModFile streamDBModFile(mod, fs::path(modFileName).filename().string());
                streamDBModFile.FileData = std::vector<std::byte>(unzippedEntry, unzippedEntry + unzippedEntrySize);
                free(unzippedEntry);

                streamDBModFiles[streamDBContainerIndex].push_back(streamDBModFile);
                zippedModCount++;
            }
        }
        else if (isSoundMod) {
            // Get the sound container info object, create it if it doesn't exist
            mtx.lock();

            ssize_t soundContainerIndex = GetSoundContainer(resourceName, soundContainerList);

            if (soundContainerIndex == -1) {
                SoundContainer soundContainer(resourceName, resourcePath);
                soundContainerList.push_back(soundContainer);

                soundContainerIndex = soundContainerList.size() - 1;
            }

            mtx.unlock();

            // Create the mod object and read the unzipped files
            if (!ProgramOptions::ListResources) {
                // Skip unsupported formats
                std::string soundExtension = fs::path(modFileName).extension().string();

                if (std::find(SupportedSoundFormats.begin(), SupportedSoundFormats.end(), soundExtension) == SupportedSoundFormats.end()) {
                    mtx.lock();
                    std::cout << Colors::Red << "WARNING: " << Colors::Reset << "Unsupported sound mod file format " << soundExtension << " for file " << modFileName << '\n';
                    mtx.unlock();
                    continue;
                }

                // Load the sound mod
                std::byte *unzippedEntry;
                size_t unzippedEntrySize;

                if ((unzippedEntry = reinterpret_cast<std::byte*>(mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0))) == nullptr) {
                    mtx.lock();
                    std::cout << Colors::Red << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
                    mtx.unlock();
                    continue;
                }

                SoundModFile soundModFile(mod, fs::path(modFileName).filename().string());
                soundModFile.FileBytes = std::vector<std::byte>(unzippedEntry, unzippedEntry + unzippedEntrySize);
                free(unzippedEntry);

                soundModFiles[soundContainerIndex].push_back(soundModFile);
                zippedModCount++;
            }
        }
        else {
            mtx.lock();

            // Get the resource object
            ssize_t resourceContainerIndex = GetResourceContainer(resourceName, resourceContainerList);

            if (resourceContainerIndex == -1) {
                ResourceContainer resource(resourceName, PathToResourceContainer(resourceName + ".resources"));
                resourceContainerList.push_back(resource);

                resourceContainerIndex = resourceContainerList.size() - 1;
            }

            mtx.unlock();

            // Create the mod object and read the unzipped files
            ResourceModFile resourceModFile(mod, modFileName, resourceName);

            if (!ProgramOptions::ListResources) {
                // Read the mod file to memory
                std::byte *unzippedEntry;
                size_t unzippedEntrySize;

                if ((unzippedEntry = reinterpret_cast<std::byte*>(mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0))) == nullptr) {
                    mtx.lock();
                    std::cout << Colors::Red << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
                    mtx.unlock();
                    continue;
                }

                resourceModFile.FileBytes = std::vector<std::byte>(unzippedEntry, unzippedEntry + unzippedEntrySize);
                free(unzippedEntry);
            }

            // Read the JSON files in 'assetsinfo' under 'EternalMod'
            if (ToLower(modFilePathParts[1]) == "eternalmod") {
                if (modFilePathParts.size() == 4
                && ToLower(modFilePathParts[2]) == "assetsinfo"
                && fs::path(modFilePathParts[3]).extension().string() == ".json") {
                    try {
                        // Read this JSON only if we are listing resources
                        if (ProgramOptions::ListResources) {
                            std::byte *unzippedEntry;
                            size_t unzippedEntrySize;

                            if ((unzippedEntry = reinterpret_cast<std::byte*>(mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0))) == nullptr) {
                                mtx.lock();
                                std::cout << Colors::Red << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
                                mtx.unlock();
                                continue;
                            }

                            resourceModFile.FileBytes = std::vector<std::byte>(unzippedEntry, unzippedEntry + unzippedEntrySize);
                            free(unzippedEntry);
                        }

                        std::string assetsInfoJson(reinterpret_cast<char*>(resourceModFile.FileBytes.data()), resourceModFile.FileBytes.size());
                        resourceModFile.AssetsInfo = AssetsInfo(assetsInfoJson);
                        resourceModFile.IsAssetsInfoJson = true;
                        resourceModFile.FileBytes.resize(0);
                    }
                    catch (...) {
                        mtx.lock();
                        std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Failed to parse EternalMod/assetsinfo/"
                            << fs::path(resourceModFile.Name).stem().string() << ".json" << '\n';
                        mtx.unlock();
                        continue;
                    }
                }
                else if (modFilePathParts.size() == 4
                && ToLower(modFilePathParts[2]) == "strings"
                && fs::path(modFilePathParts[3]).extension().string() == ".json") {
                    // Detect custom language files
                    resourceModFile.IsBlangJson = true;
                }
                else {
                    continue;
                }
            }

            resourceModFiles[resourceContainerIndex].push_back(resourceModFile);
            zippedModCount++;
        }
    }

    mtx.lock();

    // Check if the mod is safe for online play
    if (!IsModSafeForOnline(resourceModFiles)) {
        ProgramOptions::AreModsSafeForOnline = false;
        mod.IsSafeForOnline = false;

        // Unload the mod files if necessary
        if (!ProgramOptions::LoadOnlineSafeModsOnly) {
            for (const auto& resourceMod : resourceModFiles) {
                auto& resourceContainer = resourceContainerList[resourceMod.first];
                resourceContainer.ModFileList.insert(resourceContainer.ModFileList.end(), resourceMod.second.begin(), resourceMod.second.end());
            }

            for (const auto& soundMod : soundModFiles) {
                auto& soundContainer = soundContainerList[soundMod.first];
                soundContainer.ModFileList.insert(soundContainer.ModFileList.end(), soundMod.second.begin(), soundMod.second.end());
            }

            for (const auto& streamDBMod : streamDBModFiles) {
                auto& streamDBContainer = streamDBContainerList[streamDBMod.first];
                streamDBContainer.ModFiles.insert(streamDBContainer.ModFiles.end(), streamDBMod.second.begin(), streamDBMod.second.end());
            }
        }
    }
    else {
        for (const auto& resourceMod : resourceModFiles) {
            auto& resourceContainer = resourceContainerList[resourceMod.first];
            resourceContainer.ModFileList.insert(resourceContainer.ModFileList.end(), resourceMod.second.begin(), resourceMod.second.end());
        }

        for (const auto& soundMod : soundModFiles) {
            auto& soundContainer = soundContainerList[soundMod.first];
            soundContainer.ModFileList.insert(soundContainer.ModFileList.end(), soundMod.second.begin(), soundMod.second.end());
        }

        for (const auto& streamDBMod : streamDBModFiles) {
            auto& streamDBContainer = streamDBContainerList[streamDBMod.first];
            streamDBContainer.ModFiles.insert(streamDBContainer.ModFiles.end(), streamDBMod.second.begin(), streamDBMod.second.end());
        }
    }

    mtx.unlock();

    if (zippedModCount > 0 && !ProgramOptions::ListResources) {
        mtx.lock();

        if (!ProgramOptions::LoadOnlineSafeModsOnly || (ProgramOptions::LoadOnlineSafeModsOnly && mod.IsSafeForOnline)) {
            std::cout << "Found " << Colors::Blue << zippedModCount << " file(s) " << Colors::Reset << "in archive " << Colors::Yellow << zippedMod << Colors::Reset << "..." << '\n';

            if (!mod.IsSafeForOnline) {
                std::cout << Colors::Yellow << "WARNING: Mod " << zippedMod
                    << " is not safe for online play, public matchmaking will be disabled" << Colors::Reset << '\n';
            }
        }
        else {
            std::cout << Colors::Red << "WARNING: " << Colors::Reset << "Mod " << Colors::Yellow << zippedMod << Colors::Reset << " is not safe for public matchmaking, skipping" << '\n';
        }

        mtx.unlock();
    }

    mz_zip_reader_end(&modZip);
}

void LoadUnzippedMod(std::string unzippedMod, Mod& globalLooseMod, std::atomic<size_t>& unzippedModCount,
    std::map<size_t, std::vector<ResourceModFile>>& resourceModFiles,
    std::map<size_t, std::vector<SoundModFile>>& soundModFiles,
    std::map<size_t, std::vector<StreamDBModFile>>& streamDBModFiles,
    std::vector<ResourceContainer>& resourceContainerList, std::vector<SoundContainer>& soundContainerList,
    std::vector<StreamDBContainer>& streamDBContainerList, std::vector<std::string>& notFoundContainers)
{
    std::replace(unzippedMod.begin(), unzippedMod.end(), SEPARATOR, '/');
    std::vector<std::string> modFilePathParts = SplitString(unzippedMod, '/');

    if (modFilePathParts.size() < 4) {
        return;
    }

    // Determine the game container for each mod file
    bool isSoundMod = false;
    bool isStreamDBMod = false;
    std::string resourceName = modFilePathParts[2];
    std::string fileName;

    // Old mods compatibility
    if (ToLower(resourceName) == "generated") {
        resourceName = "gameresources";
        fileName = unzippedMod.substr(modFilePathParts[1].size() + 3, unzippedMod.size() - modFilePathParts[1].size() - 3);
    }
    else {
        // Remove the resource name from the path
        fileName = unzippedMod.substr(modFilePathParts[1].size() + resourceName.size() + 4, unzippedMod.size() - resourceName.size() - 4);
    }

    // Redirect .blang files to a different container if specified
    if (!ProgramOptions::BlangFileContainerRedirect.empty() && modFilePathParts[3] == "EternalMod" && modFilePathParts[4] == "strings") {
        resourceName = ProgramOptions::BlangFileContainerRedirect;
    }

    // Get path to resource file
    std::string resourcePath = PathToResourceContainer(resourceName + ".resources");

    // Check if this is a streamdb mod
    if (resourceName == "streamdb") {
        isStreamDBMod = true;
        resourcePath = ProgramOptions::BasePath + "EternalMod.streamdb";
    }

    // Check if this is a sound mod
    if (resourcePath.empty() && !isStreamDBMod) {
        resourcePath = PathToSoundContainer(resourceName);

        if (!resourcePath.empty()) {
            isSoundMod = true;
        }
        else {
            mtx.lock();

            if (std::find(notFoundContainers.begin(), notFoundContainers.end(), resourceName) == notFoundContainers.end()) {
                notFoundContainers.push_back(resourceName);
            }

            mtx.unlock();
            return;
        }
    }

    if (isStreamDBMod) {
        // Get the streamdb container info object, create it if it doesn't exist
        mtx.lock();

        auto x = std::find_if(streamDBContainerList.begin(), streamDBContainerList.end(),
            [](const StreamDBContainer& streamDbContainer) { return streamDbContainer.Name == "EternalMod.streamdb"; });

        if (x == streamDBContainerList.end()) {
            streamDBContainerList.push_back(StreamDBContainer("EternalMod.streamdb", resourcePath));
            x = streamDBContainerList.end() - 1;
        }

        auto streamDBContainerIndex = std::distance(streamDBContainerList.begin(), x);

        mtx.unlock();

        if (!ProgramOptions::ListResources) {
            // Load the streamdb mod
            size_t unzippedModSize = fs::file_size(unzippedMod);

            StreamDBModFile streamDBModFile(globalLooseMod, fs::path(fileName).filename().string());

            FILE *unzippedModFile = fopen(unzippedMod.c_str(), "rb");

            if (!unzippedModFile) {
                mtx.lock();
                std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Failed to open " << unzippedMod << " for reading." << '\n';
                mtx.unlock();
                return;
            }

            streamDBModFile.FileData.resize(unzippedModSize);

            if (fread(streamDBModFile.FileData.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                mtx.lock();
                std::cout << Colors::Reset << "ERROR: " << Colors::Reset << "Failed to read from " << unzippedMod << "." << '\n';
                mtx.unlock();
                return;
            }

            fclose(unzippedModFile);

            mtx.lock();
            streamDBModFiles[streamDBContainerIndex].push_back(streamDBModFile);
            mtx.unlock();

            unzippedModCount++;
        }
    }
    else if (isSoundMod) {
        mtx.lock();

        // Get the sound container info object, create it if it doesn't exist
        ssize_t soundContainerIndex = GetSoundContainer(resourceName, soundContainerList);

        if (soundContainerIndex == -1) {
            SoundContainer soundContainer(resourceName, resourcePath);
            soundContainerList.push_back(soundContainer);

            soundContainerIndex = soundContainerList.size() - 1;
        }

        mtx.unlock();

        // Create the mod object and read the unzipped files
        if (!ProgramOptions::ListResources) {
            // Skip unsupported formats
            std::string soundExtension = fs::path(fileName).extension().string();

            if (std::find(SupportedSoundFormats.begin(), SupportedSoundFormats.end(), soundExtension) == SupportedSoundFormats.end()) {
                mtx.lock();
                std::cout << Colors::Red << "WARNING: " << Colors::Reset << "Unsupported sound mod file format " << soundExtension << " for file " << fileName << '\n';
                mtx.unlock();
                return;
            }

            // Load the sound mod
            size_t unzippedModSize = fs::file_size(unzippedMod);

            SoundModFile soundModFile(globalLooseMod, fs::path(fileName).filename().string());

            FILE *unzippedModFile = fopen(unzippedMod.c_str(), "rb");

            if (!unzippedModFile) {
                mtx.lock();
                std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Failed to open " << unzippedMod << " for reading." << '\n';
                mtx.unlock();
                return;
            }

            soundModFile.FileBytes.resize(unzippedModSize);

            if (fread(soundModFile.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                mtx.lock();
                std::cout << Colors::Reset << "ERROR: " << Colors::Reset << "Failed to read from " << unzippedMod << "." << '\n';
                mtx.unlock();
                return;
            }

            fclose(unzippedModFile);

            mtx.lock();
            soundModFiles[soundContainerIndex].push_back(soundModFile);
            mtx.unlock();

            unzippedModCount++;
        }
    }
    else {
        mtx.lock();

        // Get the resource object
        ssize_t resourceContainerIndex = GetResourceContainer(resourceName, resourceContainerList);

        if (resourceContainerIndex == -1) {
            ResourceContainer resourceContainer(resourceName, PathToResourceContainer(resourceName + ".resources"));
            resourceContainerList.push_back(resourceContainer);

            resourceContainerIndex = resourceContainerList.size() - 1;
        }

        mtx.unlock();

        // Create the mod object and read the files
        ResourceModFile resourceModFile(globalLooseMod, fileName, resourceName);

        if (!ProgramOptions::ListResources) {
            size_t unzippedModSize = fs::file_size(unzippedMod);

            FILE *unzippedModFile = fopen(unzippedMod.c_str(), "rb");

            if (!unzippedModFile) {
                mtx.lock();
                std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Failed to open " << unzippedMod << " for reading." << '\n';
                mtx.unlock();
                return;
            }

            resourceModFile.FileBytes.resize(unzippedModSize);

            if (fread(resourceModFile.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                mtx.lock();
                std::cout << Colors::Reset << "ERROR: " << Colors::Reset << "Failed to read from " << unzippedMod << "." << '\n';
                mtx.unlock();
                return;
            }

            fclose(unzippedModFile);
        }

        // Read the JSON files in 'assetsinfo' under 'EternalMod'
        if (ToLower(modFilePathParts[3]) == "eternalmod") {
            if (modFilePathParts.size() == 6
            && ToLower(modFilePathParts[4]) == "assetsinfo"
            && fs::path(modFilePathParts[5]).extension().string() == ".json") {
                try {
                    // Read this JSON only if we are listing resources
                    if (ProgramOptions::ListResources) {
                        size_t unzippedModSize = fs::file_size(unzippedMod);

                        FILE *unzippedModFile = fopen(unzippedMod.c_str(), "rb");

                        if (!unzippedModFile) {
                            mtx.lock();
                            std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Failed to open " << unzippedMod << " for reading." << '\n';
                            mtx.unlock();
                            return;
                        }

                        resourceModFile.FileBytes.resize(unzippedModSize);

                        if (fread(resourceModFile.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                            mtx.lock();
                            std::cout << Colors::Reset << "ERROR: " << Colors::Reset << "Failed to read from " << unzippedMod << "." << '\n';
                            mtx.unlock();
                            return;
                        }

                        fclose(unzippedModFile);
                    }

                    std::string assetsInfoJson(reinterpret_cast<char*>(resourceModFile.FileBytes.data()), resourceModFile.FileBytes.size());
                    resourceModFile.AssetsInfo = AssetsInfo(assetsInfoJson);
                    resourceModFile.IsAssetsInfoJson = true;
                    resourceModFile.FileBytes.resize(0);
                }
                catch (...) {
                    mtx.lock();
                    std::cout << Colors::Red << "ERROR: " << Colors::Reset << "Failed to parse EternalMod/assetsinfo/"
                        << fs::path(resourceModFile.Name).stem().string() << ".json" << '\n';
                    mtx.unlock();
                    return;
                }
            }
            else if (modFilePathParts.size() == 6
            && ToLower(modFilePathParts[4]) == "strings"
            && fs::path(modFilePathParts[5]).extension().string() == ".json") {
                // Detect custom language files
                resourceModFile.IsBlangJson = true;
            }
            else {
                return;
            }
        }

        mtx.lock();
        resourceModFiles[resourceContainerIndex].push_back(resourceModFile);
        mtx.unlock();

        unzippedModCount++;
    }
}
