#include <iostream>
#include <filesystem>
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

    mz_zip_archive modZip;
    mz_zip_zero_struct(&modZip);
    mz_zip_reader_init_file(&modZip, zippedMod.c_str(), 0);

    Mod mod;

    if (!ListResources) {
        char *unzippedModJson;
        size_t unzippedModJsonSize;

        if ((unzippedModJson = (char*)mz_zip_reader_extract_file_to_heap(&modZip, "EternalMod.json", &unzippedModJsonSize, 0)) != nullptr) {
            std::string modJson(unzippedModJson, unzippedModJsonSize);
            free(unzippedModJson);

            try {
                mod = Mod(modJson);

                if (mod.RequiredVersion > Version) {
                    mtx.lock();
                    std::cout << RED << "WARNING: " << RESET << "Mod " << std::filesystem::path(zippedMod).filename().string() << " requires mod loader version "
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

    for (int32_t i = 0; i < modZip.m_total_files; i++) {
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

        if (0 == zipEntryName.compare(zipEntryName.length() - 1, 1, "/")) {
            continue;
        }

        bool isSoundMod = false;
        std::string modFileName = zipEntryName;
        std::vector<std::string> modFilePathParts = SplitString(modFileName, '/');

        if (modFilePathParts.size() < 2) {
            continue;
        }

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

        if (isSoundMod) {
            mtx.lock();

            int32_t soundContainerIndex = GetSoundContainer(resourceName);

            if (soundContainerIndex == -1) {
                SoundContainer soundContainer(resourceName, resourcePath);
                SoundContainerList.push_back(soundContainer);

                soundContainerIndex = SoundContainerList.size() - 1;
            }

            mtx.unlock();

            if (!ListResources) {
                std::string soundExtension = std::filesystem::path(modFileName).extension().string();

                if (std::find(SupportedFileFormats.begin(), SupportedFileFormats.end(), soundExtension) == SupportedFileFormats.end()) {
                    mtx.lock();
                    std::cout << RED << "WARNING: " << RESET << "Unsupported sound mod file format " << soundExtension << " for file " << modFileName << '\n';
                    mtx.unlock();
                    continue;
                }

                std::byte *unzippedEntry;
                size_t unzippedEntrySize;

                if ((unzippedEntry = (std::byte*)mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0)) == nullptr) {
                    mtx.lock();
                    std::cout << RED << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
                    mtx.unlock();
                    continue;
                }

                SoundModFile soundModFile(mod, std::filesystem::path(modFileName).filename().string());
                soundModFile.FileBytes = std::vector<std::byte>(unzippedEntry, unzippedEntry + unzippedEntrySize);
                free(unzippedEntry);

                soundModFiles[soundContainerIndex].push_back(soundModFile);
                zippedModCount++;
            }
        }
        else {
            mtx.lock();

            int32_t resourceContainerIndex = GetResourceContainer(resourceName);

            if (resourceContainerIndex == -1) {
                ResourceContainer resource(resourceName, PathToResourceContainer(resourceName + ".resources"));
                ResourceContainerList.push_back(resource);

                resourceContainerIndex = ResourceContainerList.size() - 1;
            }

            mtx.unlock();

            ResourceModFile resourceModFile(mod, modFileName, resourceName);

            if (!ListResources) {
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

            if (ToLower(modFilePathParts[1]) == "eternalmod") {
                if (modFilePathParts.size() == 4
                && ToLower(modFilePathParts[2]) == "assetsinfo"
                && std::filesystem::path(modFilePathParts[3]).extension() == ".json") {
                    try {
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
                            << std::filesystem::path(resourceModFile.Name).stem().string() << ".json" << '\n';
                        mtx.unlock();
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

            resourceModFiles[resourceContainerIndex].push_back(resourceModFile);
            zippedModCount++;
        }
    }

    mtx.lock();

    if (!IsModSafeForOnline(resourceModFiles)) {
        AreModsSafeForOnline = false;
        mod.IsSafeForOnline = false;

        if (!LoadOnlineSafeModsOnly) {
            for (auto &resourceMod : resourceModFiles) {
                ResourceContainer &resourceContainer = ResourceContainerList[resourceMod.first];
                resourceContainer.ModFileList.insert(resourceContainer.ModFileList.end(), resourceMod.second.begin(), resourceMod.second.end());
            }

            for (auto &soundMod : soundModFiles) {
                SoundContainer &soundContainer = SoundContainerList[soundMod.first];
                soundContainer.ModFileList.insert(soundContainer.ModFileList.end(), soundMod.second.begin(), soundMod.second.end());
        }
        }
    }
    else {
        for (auto &resourceMod : resourceModFiles) {
            ResourceContainer &resourceContainer = ResourceContainerList[resourceMod.first];
            resourceContainer.ModFileList.insert(resourceContainer.ModFileList.end(), resourceMod.second.begin(), resourceMod.second.end());
        }

        for (auto &soundMod : soundModFiles) {
            SoundContainer &soundContainer = SoundContainerList[soundMod.first];
            soundContainer.ModFileList.insert(soundContainer.ModFileList.end(), soundMod.second.begin(), soundMod.second.end());
        }
    }

    mtx.unlock();
        
    if (zippedModCount > 0 && !ListResources) {
        mtx.lock();

        if (!LoadOnlineSafeModsOnly || (LoadOnlineSafeModsOnly && mod.IsSafeForOnline)) {
            std::cout << "Found " << BLUE << zippedModCount << " file(s) " << RESET << "in archive " << YELLOW << zippedMod << RESET << "..." << '\n';

            if (!mod.IsSafeForOnline) {
                std::cout << YELLOW << "WARNING: Mod " << zippedMod << " is not safe for online play, multiplayer will be disabled" << RESET << '\n';
            }
        }
        else {
            std::cout << RED << "WARNING: " << RESET << "Mod " << YELLOW << zippedMod << RESET << " is not safe for online play, skipping" << '\n';
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
    std::vector<std::string> &notFoundContainers)
{
    std::replace(unzippedMod.begin(), unzippedMod.end(), Separator, '/');
    std::vector<std::string> modFilePathParts = SplitString(unzippedMod, '/');

    if (modFilePathParts.size() < 4) {
        return;
    }

    bool isSoundMod = false;
    std::string resourceName = modFilePathParts[2];
    std::string fileName;

    if (ToLower(resourceName) == "generated") {
        resourceName = "gameresources";
        fileName = unzippedMod.substr(modFilePathParts[1].size() + 3, unzippedMod.size() - modFilePathParts[1].size() - 3);
    }
    else {
        fileName = unzippedMod.substr(modFilePathParts[1].size() + resourceName.size() + 4, unzippedMod.size() - resourceName.size() - 4);
    }

    std::string resourcePath = PathToResourceContainer(resourceName + ".resources");

    if (resourcePath.empty()) {
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

    if (isSoundMod) {
        mtx.lock();

        int32_t soundContainerIndex = GetSoundContainer(resourceName);

        if (soundContainerIndex == -1) {
            SoundContainer soundContainer(resourceName, resourcePath);
            SoundContainerList.push_back(soundContainer);

            soundContainerIndex = SoundContainerList.size() - 1;
        }

        mtx.unlock();

        if (!ListResources) {
            std::string soundExtension = std::filesystem::path(fileName).extension().string();

            if (std::find(SupportedFileFormats.begin(), SupportedFileFormats.end(), soundExtension) == SupportedFileFormats.end()) {
                mtx.lock();
                std::cout << RED << "WARNING: " << RESET << "Unsupported sound mod file format " << soundExtension << " for file " << fileName << '\n';
                mtx.unlock();
                return;
            }

            int64_t unzippedModSize = std::filesystem::file_size(unzippedMod);
            
            SoundModFile soundModFile(globalLooseMod, std::filesystem::path(fileName).filename().string());
            
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

        int32_t resourceContainerIndex = GetResourceContainer(resourceName);

        if (resourceContainerIndex == -1) {
            ResourceContainer resourceContainer(resourceName, PathToResourceContainer(resourceName + ".resources"));
            ResourceContainerList.push_back(resourceContainer);

            resourceContainerIndex = ResourceContainerList.size() - 1;
        }

        mtx.unlock();

        ResourceModFile resourceModFile(globalLooseMod, fileName, resourceName);

        if (!ListResources) {
            int64_t unzippedModSize = std::filesystem::file_size(unzippedMod);

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

        if (ToLower(modFilePathParts[3]) == "eternalmod") {
            if (modFilePathParts.size() == 6
            && ToLower(modFilePathParts[4]) == "assetsinfo"
            && std::filesystem::path(modFilePathParts[5]).extension() == ".json") {
                try {
                    if (ListResources) {
                        int64_t unzippedModSize = std::filesystem::file_size(unzippedMod);

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
                        << std::filesystem::path(resourceModFile.Name).stem().string() << ".json" << '\n';
                    mtx.unlock();
                    return;
                }
            }
            else if (modFilePathParts.size() == 6
            && ToLower(modFilePathParts[4]) == "strings"
            && std::filesystem::path(modFilePathParts[5]).extension() == ".json") {
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