#include <iostream>
#include <algorithm>
#include "miniz/miniz.h"
#include "EternalModLoader.hpp"

/**
 * @brief Load mod files from zip
 * 
 * @param zippedMod Zipped mod path
 * @param notFoundContainers Vector to push not found resources to
 */
void LoadZippedMod(std::string zippedMod, std::vector<std::string> &notFoundContainers)
{
    int32_t zippedModCount = 0;
    std::vector<std::string> modFileNameList;
    std::map<int32_t, std::vector<ResourceModFile>> resourceModFiles;
    std::map<int32_t, std::vector<SoundModFile>> soundModFiles;
    std::map<int32_t, std::vector<StreamDBModFile>> streamDBModFiles;

    // Load zipped mod
    mz_zip_archive modZip;
    mz_zip_zero_struct(&modZip);
    mz_zip_reader_init_file(&modZip, zippedMod.c_str(), 0);

    Mod mod;

    if (!ListResources) {
        // Read the mod info from the EternalMod JSON if it exists
        char *unzippedModJson;
        size_t unzippedModJsonSize;

        if ((unzippedModJson = (char*)mz_zip_reader_extract_file_to_heap(&modZip, "EternalMod.json", &unzippedModJsonSize, 0)) != nullptr) {
            std::string modJson(unzippedModJson, unzippedModJsonSize);
            free(unzippedModJson);

            try {
                // Try to parse the JSON
                mod = Mod(modJson);

                // If the mod requires a higher mod loader version, print a warning and don't load the mod
                if (mod.RequiredVersion > Version) {
                    mtx.lock();
                    std::cout << RED << "WARNING: " << RESET << "Mod " << fs::path(zippedMod).filename().string() << " requires mod loader version "
                        << mod.RequiredVersion << " but the current mod loader version is " << Version << ", skipping" << '\n';
                    mtx.unlock();
                    return;
                }
            }
            catch (...) {
                mtx.lock();
                std::cout << RED << "ERROR: " << RESET << "Failed to parse EternalMod.json - using defaults." << '\n';
                mtx.unlock();
            }
        }
    }

    // Iterate through the zip's files
    for (int32_t i = 0; i < modZip.m_total_files; i++) {
        // Get the mod file's name
        int32_t zipEntryNameSize = mz_zip_reader_get_filename(&modZip, i, nullptr, 0);
        char *zipEntryNameBuffer = new char[zipEntryNameSize];

        if (mz_zip_reader_get_filename(&modZip, i, zipEntryNameBuffer, zipEntryNameSize) != zipEntryNameSize || zipEntryNameBuffer == nullptr) {
            mtx.lock();
            std::cout << RED << "ERROR: " << RESET << "Failed to read zip file entry from " << zippedMod << '\n';
            mtx.unlock();
            delete[] zipEntryNameBuffer;
            continue;
        }

        std::string zipEntryName(zipEntryNameBuffer);
        delete[] zipEntryNameBuffer;

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

        // Get path to resource file
        std::string resourcePath = PathToResourceContainer(resourceName + ".resources");

        // Check if this is a streamdb mod
        if (resourceName == "streamdb") {
            isStreamDBMod = true;
            resourcePath = BasePath + "EternalMod.streamdb";
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

            auto x = std::find_if(StreamDBContainerList.begin(), StreamDBContainerList.end(),
                [](const StreamDBContainer &streamDbContainer) { return streamDbContainer.Name == "EternalMod.streamdb"; });

            if (x == StreamDBContainerList.end()) {
                StreamDBContainerList.push_back(StreamDBContainer("EternalMod.streamdb", resourcePath));
                x = StreamDBContainerList.end() - 1;
            }

            auto streamDBContainerIndex = std::distance(StreamDBContainerList.begin(), x);

            mtx.unlock();

            if (!ListResources) {
                // Load the streamdb mod
                std::byte *unzippedEntry;
                size_t unzippedEntrySize;

                if ((unzippedEntry = (std::byte*)mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0)) == nullptr) {
                    mtx.lock();
                    std::cout << RED << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
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

            int32_t soundContainerIndex = GetSoundContainer(resourceName);

            if (soundContainerIndex == -1) {
                SoundContainer soundContainer(resourceName, resourcePath);
                SoundContainerList.push_back(soundContainer);

                soundContainerIndex = SoundContainerList.size() - 1;
            }

            mtx.unlock();

            // Create the mod object and read the unzipped files
            if (!ListResources) {
                // Skip unsupported formats
                std::string soundExtension = fs::path(modFileName).extension().string();

                if (std::find(SupportedFileFormats.begin(), SupportedFileFormats.end(), soundExtension) == SupportedFileFormats.end()) {
                    mtx.lock();
                    std::cout << RED << "WARNING: " << RESET << "Unsupported sound mod file format " << soundExtension << " for file " << modFileName << '\n';
                    mtx.unlock();
                    continue;
                }

                // Load the sound mod
                std::byte *unzippedEntry;
                size_t unzippedEntrySize;

                if ((unzippedEntry = (std::byte*)mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0)) == nullptr) {
                    mtx.lock();
                    std::cout << RED << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
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
            int32_t resourceContainerIndex = GetResourceContainer(resourceName);

            if (resourceContainerIndex == -1) {
                ResourceContainer resource(resourceName, PathToResourceContainer(resourceName + ".resources"));
                ResourceContainerList.push_back(resource);

                resourceContainerIndex = ResourceContainerList.size() - 1;
            }

            mtx.unlock();

            // Create the mod object and read the unzipped files
            ResourceModFile resourceModFile(mod, modFileName, resourceName);

            if (!ListResources) {
                // Read the mod file to memory
                std::byte *unzippedEntry;
                size_t unzippedEntrySize;

                if ((unzippedEntry = (std::byte*)mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0)) == nullptr) {
                    mtx.lock();
                    std::cout << RED << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
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
                        if (ListResources) {
                            std::byte *unzippedEntry;
                            size_t unzippedEntrySize;

                            if ((unzippedEntry = (std::byte*)mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0)) == nullptr) {
                                mtx.lock();
                                std::cout << RED << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
                                mtx.unlock();
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
                        mtx.lock();
                        std::cout << RED << "ERROR: " << RESET << "Failed to parse EternalMod/assetsinfo/"
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
        AreModsSafeForOnline = false;
        mod.IsSafeForOnline = false;

        // Unload the mod files if necessary
        if (!LoadOnlineSafeModsOnly) {
            for (const auto &resourceMod : resourceModFiles) {
                auto &resourceContainer = ResourceContainerList[resourceMod.first];
                resourceContainer.ModFileList.insert(resourceContainer.ModFileList.end(), resourceMod.second.begin(), resourceMod.second.end());
            }

            for (const auto &soundMod : soundModFiles) {
                auto &soundContainer = SoundContainerList[soundMod.first];
                soundContainer.ModFileList.insert(soundContainer.ModFileList.end(), soundMod.second.begin(), soundMod.second.end());
            }

            for (const auto &streamDBMod : streamDBModFiles) {
                auto &streamDBContainer = StreamDBContainerList[streamDBMod.first];
                streamDBContainer.ModFiles.insert(streamDBContainer.ModFiles.end(), streamDBMod.second.begin(), streamDBMod.second.end());
            }
        }
    }
    else {
        for (const auto &resourceMod : resourceModFiles) {
            auto &resourceContainer = ResourceContainerList[resourceMod.first];
            resourceContainer.ModFileList.insert(resourceContainer.ModFileList.end(), resourceMod.second.begin(), resourceMod.second.end());
        }

        for (const auto &soundMod : soundModFiles) {
            auto &soundContainer = SoundContainerList[soundMod.first];
            soundContainer.ModFileList.insert(soundContainer.ModFileList.end(), soundMod.second.begin(), soundMod.second.end());
        }

        for (const auto &streamDBMod : streamDBModFiles) {
            auto &streamDBContainer = StreamDBContainerList[streamDBMod.first];
            streamDBContainer.ModFiles.insert(streamDBContainer.ModFiles.end(), streamDBMod.second.begin(), streamDBMod.second.end());
        }
    }

    mtx.unlock();

    if (zippedModCount > 0 && !ListResources) {
        mtx.lock();

        if (!LoadOnlineSafeModsOnly || (LoadOnlineSafeModsOnly && mod.IsSafeForOnline)) {
            std::cout << "Found " << BLUE << zippedModCount << " file(s) " << RESET << "in archive " << YELLOW << zippedMod << RESET << "..." << '\n';

            if (!mod.IsSafeForOnline) {
                std::cout << YELLOW << "WARNING: Mod " << zippedMod
                    << " is not safe for online play, public matchmaking will be disabled" << RESET << '\n';
            }
        }
        else {
            std::cout << RED << "WARNING: " << RESET << "Mod " << YELLOW << zippedMod << RESET << " is not safe for public matchmaking, skipping" << '\n';
        }

        mtx.unlock();
    }

    mz_zip_reader_end(&modZip);
}

/**
 * @brief Load loose mod files from Mods directory
 * 
 * @param unzippedMod Loose mod path
 * @param globalLooseMod Mod object to use for all loose mods
 * @param unzippedModCount Atomic int to increase with every unzipped mod loaded
 * @param resourceModFiles Map to add resource mod files to
 * @param soundModFiles Map to add sound mod files to
 * @param notFoundContainers Vector to push not found resources to
 */
void LoadUnzippedMod(std::string unzippedMod, Mod &globalLooseMod, std::atomic<int32_t> &unzippedModCount,
    std::map<int32_t, std::vector<ResourceModFile>> &resourceModFiles,
    std::map<int32_t, std::vector<SoundModFile>> &soundModFiles,
    std::map<int32_t, std::vector<StreamDBModFile>> &streamDBModFiles,
    std::vector<std::string> &notFoundContainers)
{
    std::replace(unzippedMod.begin(), unzippedMod.end(), Separator, '/');
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

    // Get path to resource file
    std::string resourcePath = PathToResourceContainer(resourceName + ".resources");

    // Check if this is a streamdb mod
    if (resourceName == "streamdb") {
        isStreamDBMod = true;
        resourcePath = BasePath + "EternalMod.streamdb";
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

        auto x = std::find_if(StreamDBContainerList.begin(), StreamDBContainerList.end(),
            [](const StreamDBContainer &streamDbContainer) { return streamDbContainer.Name == "EternalMod.streamdb"; });

        if (x == StreamDBContainerList.end()) {
            StreamDBContainerList.push_back(StreamDBContainer("EternalMod.streamdb", resourcePath));
            x = StreamDBContainerList.end() - 1;
        }

        auto streamDBContainerIndex = std::distance(StreamDBContainerList.begin(), x);

        mtx.unlock();

        if (!ListResources) {
            // Load the streamdb mod
            int64_t unzippedModSize = fs::file_size(unzippedMod);

            StreamDBModFile streamDBModFile(globalLooseMod, fs::path(fileName).filename().string());

            FILE *unzippedModFile = fopen(unzippedMod.c_str(), "rb");

            if (!unzippedModFile) {
                mtx.lock();
                std::cout << RED << "ERROR: " << RESET << "Failed to open " << unzippedMod << " for reading." << '\n';
                mtx.unlock();
                return;
            }

            streamDBModFile.FileData.resize(unzippedModSize);

            if (fread(streamDBModFile.FileData.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                mtx.lock();
                std::cout << RESET << "ERROR: " << RESET << "Failed to read from " << unzippedMod << "." << '\n';
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
        int32_t soundContainerIndex = GetSoundContainer(resourceName);

        if (soundContainerIndex == -1) {
            SoundContainer soundContainer(resourceName, resourcePath);
            SoundContainerList.push_back(soundContainer);

            soundContainerIndex = SoundContainerList.size() - 1;
        }

        mtx.unlock();

        // Create the mod object and read the unzipped files
        if (!ListResources) {
            // Skip unsupported formats
            std::string soundExtension = fs::path(fileName).extension().string();

            if (std::find(SupportedFileFormats.begin(), SupportedFileFormats.end(), soundExtension) == SupportedFileFormats.end()) {
                mtx.lock();
                std::cout << RED << "WARNING: " << RESET << "Unsupported sound mod file format " << soundExtension << " for file " << fileName << '\n';
                mtx.unlock();
                return;
            }

            // Load the sound mod
            int64_t unzippedModSize = fs::file_size(unzippedMod);

            SoundModFile soundModFile(globalLooseMod, fs::path(fileName).filename().string());

            FILE *unzippedModFile = fopen(unzippedMod.c_str(), "rb");

            if (!unzippedModFile) {
                mtx.lock();
                std::cout << RED << "ERROR: " << RESET << "Failed to open " << unzippedMod << " for reading." << '\n';
                mtx.unlock();
                return;
            }

            soundModFile.FileBytes.resize(unzippedModSize);

            if (fread(soundModFile.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                mtx.lock();
                std::cout << RESET << "ERROR: " << RESET << "Failed to read from " << unzippedMod << "." << '\n';
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
        int32_t resourceContainerIndex = GetResourceContainer(resourceName);

        if (resourceContainerIndex == -1) {
            ResourceContainer resourceContainer(resourceName, PathToResourceContainer(resourceName + ".resources"));
            ResourceContainerList.push_back(resourceContainer);

            resourceContainerIndex = ResourceContainerList.size() - 1;
        }

        mtx.unlock();

        // Create the mod object and read the files
        ResourceModFile resourceModFile(globalLooseMod, fileName, resourceName);

        if (!ListResources) {
            int64_t unzippedModSize = fs::file_size(unzippedMod);

            FILE *unzippedModFile = fopen(unzippedMod.c_str(), "rb");

            if (!unzippedModFile) {
                mtx.lock();
                std::cout << RED << "ERROR: " << RESET << "Failed to open " << unzippedMod << " for reading." << '\n';
                mtx.unlock();
                return;
            }

            resourceModFile.FileBytes.resize(unzippedModSize);

            if (fread(resourceModFile.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                mtx.lock();
                std::cout << RESET << "ERROR: " << RESET << "Failed to read from " << unzippedMod << "." << '\n';
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
                    if (ListResources) {
                        int64_t unzippedModSize = fs::file_size(unzippedMod);

                        FILE *unzippedModFile = fopen(unzippedMod.c_str(), "rb");

                        if (!unzippedModFile) {
                            mtx.lock();
                            std::cout << RED << "ERROR: " << RESET << "Failed to open " << unzippedMod << " for reading." << '\n';
                            mtx.unlock();
                            return;
                        }

                        resourceModFile.FileBytes.resize(unzippedModSize);

                        if (fread(resourceModFile.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                            mtx.lock();
                            std::cout << RESET << "ERROR: " << RESET << "Failed to read from " << unzippedMod << "." << '\n';
                            mtx.unlock();
                            return;
                        }

                        fclose(unzippedModFile);
                    }

                    std::string assetsInfoJson((char*)resourceModFile.FileBytes.data(), resourceModFile.FileBytes.size());
                    resourceModFile.AssetsInfo = AssetsInfo(assetsInfoJson);
                    resourceModFile.IsAssetsInfoJson = true;
                    resourceModFile.FileBytes.resize(0);
                }
                catch (...) {
                    mtx.lock();
                    std::cout << RED << "ERROR: " << RESET << "Failed to parse EternalMod/assetsinfo/"
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
