#include <iostream>
#include <filesystem>
#include <algorithm>

#include "miniz/miniz.h"
#include "EternalModLoader.hpp"

void LoadZippedMod(std::string zippedMod, bool listResources, std::vector<std::string> &notFoundContainers)
{
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
                        << mod.RequiredVersion << " but the current mod loader version is " << Version << ", skipping" << '\n';
                    return;
                }
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to parse EternalMod.json - using defaults." << '\n';
            }
        }
    }

    for (int32_t i = 0; i < modZip.m_total_files; i++) {
        int32_t zipEntryNameSize = mz_zip_reader_get_filename(&modZip, i, NULL, 0);
        char *zipEntryNameBuffer = new char[zipEntryNameSize];

        if (mz_zip_reader_get_filename(&modZip, i, zipEntryNameBuffer, zipEntryNameSize) != zipEntryNameSize || zipEntryNameBuffer == NULL) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to read zip file entry from " << zippedMod << '\n';
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

            if (!resourcePath.empty()) {
                isSoundMod = true;
            }
            else {
                mtx.lock();

                if (std::find(notFoundContainers.begin(), notFoundContainers.end(), resourceName) == notFoundContainers.end())
                    notFoundContainers.push_back(resourceName);

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

            if (!listResources) {
                std::string soundExtension = std::filesystem::path(modFileName).extension().string();

                if (std::find(SupportedFileFormats.begin(), SupportedFileFormats.end(), soundExtension) == SupportedFileFormats.end()) {
                    std::cerr << RED << "WARNING: " << RESET << "Unsupported sound mod file format " << soundExtension << " for file " << modFileName << '\n';
                    continue;
                }

                std::byte *unzippedEntry;
                size_t unzippedEntrySize;

                if ((unzippedEntry = (std::byte*)mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0)) == NULL) {
                    std::cerr << RED << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
                    continue;
                }

                SoundModFile soundModFile(mod, std::filesystem::path(modFileName).filename().string());
                soundModFile.FileBytes = std::vector<std::byte>(unzippedEntry, unzippedEntry + unzippedEntrySize);
                free(unzippedEntry);

                mtx.lock();
                SoundContainerList[soundContainerIndex].ModFileList.push_back(soundModFile);
                mtx.unlock();

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

            ResourceModFile resourceModFile(mod, modFileName);

            if (!listResources) {
                std::byte *unzippedEntry;
                size_t unzippedEntrySize;

                if ((unzippedEntry = (std::byte*)mz_zip_reader_extract_to_heap(&modZip, i, &unzippedEntrySize, 0)) == NULL) {
                    std::cerr << RED << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
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
                                std::cerr << RED << "ERROR: " << "Failed to extract zip entry from " << zippedMod << '\n';
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
                            << std::filesystem::path(resourceModFile.Name).stem().string() << ".json" << '\n';
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

            mtx.lock();
            ResourceContainerList[resourceContainerIndex].ModFileList.push_back(resourceModFile);
            mtx.unlock();

            zippedModCount++;
        }
    }
        
    if (zippedModCount > 0 && !listResources)
        std::cout << "Found " << BLUE << zippedModCount << " file(s) " << RESET << "in archive " << YELLOW << zippedMod << RESET << "..." << '\n';

    mz_zip_reader_end(&modZip);
}

void LoadUnzippedMod(std::string unzippedMod, bool listResources, Mod &globalLooseMod, std::atomic<int32_t> &unzippedModCount, std::vector<std::string> &notFoundContainers)
{
    std::replace(unzippedMod.begin(), unzippedMod.end(), separator, '/');
    std::vector<std::string> modFilePathParts = SplitString(unzippedMod, '/');

    if (modFilePathParts.size() < 4)
        return;

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

            if (std::find(notFoundContainers.begin(), notFoundContainers.end(), resourceName) == notFoundContainers.end())
                notFoundContainers.push_back(resourceName);

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

        if (!listResources) {
            std::string soundExtension = std::filesystem::path(fileName).extension().string();

            if (std::find(SupportedFileFormats.begin(), SupportedFileFormats.end(), soundExtension) == SupportedFileFormats.end()) {
                std::cerr << RED << "WARNING: " << RESET << "Unsupported sound mod file format " << soundExtension << " for file " << fileName << '\n';
                return;
            }

            int64_t unzippedModSize = std::filesystem::file_size(unzippedMod);

            if (unzippedModSize > ResourceContainerList.max_size())
                std::cerr << RED << "WARNING: " << RESET << "Skipped " << fileName << " - too large." << '\n';
            
            SoundModFile soundModFile(globalLooseMod, std::filesystem::path(fileName).filename().string());
            
            FILE *unzippedModFile = fopen(unzippedMod.c_str(), "rb");

            if (!unzippedModFile) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to open " << unzippedMod << " for reading." << '\n';
                return;
            }

            soundModFile.FileBytes.resize(unzippedModSize);

            if (fread(soundModFile.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                std::cerr << RESET << "ERROR: " << RESET << "Failed to read from " << unzippedMod << "." << '\n';
                return;
            }

            fclose(unzippedModFile);

            mtx.lock();
            SoundContainerList[soundContainerIndex].ModFileList.push_back(soundModFile);
            mtx.unlock();

            unzippedModCount++;
        }
    }
    else {
        mtx.lock();

        int32_t resourceContainerIndex = GetResourceContainer(resourceName);

        if (resourceContainerIndex == -1) {
            ResourceContainer resourceContainer(resourceName, PathToResourceContainer(resourceName));
            ResourceContainerList.push_back(resourceContainer);

            resourceContainerIndex = ResourceContainerList.size() - 1;
        }

        mtx.unlock();

        ResourceModFile resourceModFile(globalLooseMod, fileName);

        if (!listResources) {
            int64_t unzippedModSize = std::filesystem::file_size(unzippedMod);

            if (unzippedModSize > ResourceContainerList.max_size())
                std::cerr << RED << "WARNING: " << RESET << "Skipped " << fileName << " - too large." << '\n';

            FILE *unzippedModFile = fopen(unzippedMod.c_str(), "rb");

            if (!unzippedModFile) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to open " << unzippedMod << " for reading." << '\n';
                return;
            }

            resourceModFile.FileBytes.resize(unzippedModSize);

            if (fread(resourceModFile.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                std::cerr << RESET << "ERROR: " << RESET << "Failed to read from " << unzippedMod << "." << '\n';
                return;
            }

            fclose(unzippedModFile);
        }

        if (ToLower(modFilePathParts[3]) == "eternalmod") {
            if (modFilePathParts.size() == 6
            && ToLower(modFilePathParts[4]) == "assetsinfo"
            && std::filesystem::path(modFilePathParts[5]).extension() == ".json") {
                try {
                    if (listResources) {
                        int64_t unzippedModSize = std::filesystem::file_size(unzippedMod);

                        if (unzippedModSize > ResourceContainerList.max_size())
                            std::cerr << RED << "WARNING: " << RESET << "Skipped " << fileName << " - too large." << '\n';

                        FILE *unzippedModFile = fopen(unzippedMod.c_str(), "rb");

                        if (!unzippedModFile) {
                            std::cerr << RED << "ERROR: " << RESET << "Failed to open " << unzippedMod << " for reading." << '\n';
                            return;
                        }

                        resourceModFile.FileBytes.resize(unzippedModSize);

                        if (fread(resourceModFile.FileBytes.data(), 1, unzippedModSize, unzippedModFile) != unzippedModSize) {
                            std::cerr << RESET << "ERROR: " << RESET << "Failed to read from " << unzippedMod << "." << '\n';
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
                    std::cerr << RED << "ERROR: " << RESET << "Failed to parse EternalMod/assetsinfo/"
                        << std::filesystem::path(resourceModFile.Name).stem().string() << ".json" << '\n';
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
        ResourceContainerList[resourceContainerIndex].ModFileList.push_back(resourceModFile);
        mtx.unlock();

        unzippedModCount++;
    }
}