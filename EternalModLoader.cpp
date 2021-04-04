#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
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

int main(int argc, char *argv[]) {
    std::ios::sync_with_stdio(false);

    if (std::getenv("ETERNALMODLOADER_NO_COLORS") == NULL) {
        RESET = "\033[0m";
        RED = "\033[31m";
        GREEN = "\033[32m";
        YELLOW = "\033[33m";
        BLUE = "\033[34m";
    }

    std::vector<int> maxSize;

    ResourceList.reserve(80);

    if (argc == 1 || argc > 3) {
        std::cout << "Loads mods from ZIPs or loose files in 'Mods' folder into the .resources files in the specified directory" << std::endl;
        std::cout << "USAGE: EternalModLoader <game path> [OPTIONS]" << std::endl;
        std::cout << "OPTIONS:" << std::endl;
        std::cout << "\t--list-res - List the .resources files that will be modified and exit." << std::endl;
        return 1;
    }

    std::string argv1(argv[1]);
    BasePath = argv1 + "/base/";

    if(!(std::filesystem::exists(BasePath))) {
        std::cout << RED << "ERROR: " << RESET << "Game directory does not exist!" << std::endl;
        return 1;
    }

    bool listResources = false;
    if (argc == 3) {
        std::string argv2(argv[2]);
        if (argv2 == "--list-res") {
            listResources = true;
        }
        else {
            std::cout << RED << "ERROR: " << RESET << "Unknown argument: " << argv2 << std::endl;
            return 1;
        }
    }

    for (const auto& zippedMod : std::filesystem::directory_iterator(argv1 + "/Mods")) {
        if (std::filesystem::is_regular_file(zippedMod) && zippedMod.path().extension() == ".zip") {
            int zippedModCount = 0;
            std::vector<std::string> modFileNameList;
            zipper::Unzipper modZip(zippedMod.path());

            for (auto& zipEntry : modZip.entries()) {
                if (0 == zipEntry.name.compare(zipEntry.name.length() -1, 1, "/")) continue;
                std::string modFileName = zipEntry.name;

                std::string modFileParts = modFileName;
                std::vector<std::string> modFilePathParts;
                size_t pos;
                std::string part;
                while ((pos = modFileParts.find('/')) != std::string::npos) {
                    part = modFileParts.substr(0, pos);
                    modFilePathParts.push_back(part);
                    modFileParts.erase(0, pos + 1);
                }
                modFilePathParts.push_back(modFileParts);

                if (modFilePathParts.size() < 2) continue;

                std::string resourceName = modFilePathParts[0];

                if (ToLower(resourceName) == "generated") {
                    resourceName = "gameresources";
                }
                else {
                    modFileName = modFileName.substr(resourceName.size() + 1, modFileName.size() - resourceName.size() - 1);
                }

                int resourceIndex = GetResourceInfo(resourceName);

                if (resourceIndex == -1) {
                    ResourceInfo resource(resourceName, PathToRes(resourceName));
                    ResourceList.push_back(resource);
                    resourceIndex = (int)ResourceList.size() - 1;
                }

                if (!listResources) {
                    Mod mod(modFileName);
                    unsigned long streamSize = zipEntry.uncompressedSize;
                    if (streamSize > maxSize.max_size()) {
                        std::cout << "Skipped " << modFileName << " - too large." << std::endl;
                        continue;
                    }
                    std::vector<unsigned char> unzipped_entry;
                    unzipped_entry.reserve(zipEntry.uncompressedSize);
                    std::vector<std::byte> unzipped_entry_bytes;
                    modZip.extractEntryToMemory(zipEntry.name, unzipped_entry);
                    unzipped_entry_bytes.reserve(unzipped_entry.size());
                    unzipped_entry_bytes.insert(unzipped_entry_bytes.begin(), (std::byte*)unzipped_entry.data(), (std::byte*)unzipped_entry.data() + unzipped_entry.size());
                    mod.FileBytes = unzipped_entry_bytes;

                    std::string modFilePathPart1ToLower = ToLower(modFilePathParts[1]);
                    if (modFilePathPart1ToLower == "eternalmod") {
                        std::string modFilePathPart2ToLower = ToLower(modFilePathParts[2]);
                        if (modFilePathParts.size() == 4
                        && modFilePathPart2ToLower == "strings"
                        && std::filesystem::path(modFilePathParts[3]).extension() == ".json")
                        {
                            mod.isBlangJson = true;
                        }
                        else {
                            continue;
                        }
                    }
                    else {
                        mod.isBlangJson = false;
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
        else continue;
    }

    int unzippedModCount = 0;

    for (const auto& unzippedMod : std::filesystem::recursive_directory_iterator(argv1 + "/Mods")) {
        if (std::filesystem::is_regular_file(unzippedMod) && !(unzippedMod.path().extension() == ".zip")) {
            std::string unzippedModPath = unzippedMod.path();

            std::string unzippedModParts = unzippedModPath;
            std::vector<std::string> modFilePathParts;
            size_t pos;
            std::string part;
            while ((pos = unzippedModParts.find('/')) != std::string::npos) {
                part = unzippedModParts.substr(0, pos);
                modFilePathParts.push_back(part);
                unzippedModParts.erase(0, pos + 1);
            }
            modFilePathParts.push_back(unzippedModParts);

            if (modFilePathParts.size() < 4) continue;

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

            if (resourceIndex == -1) {
                if (PathToRes(resourceName).empty()) continue;
                ResourceInfo resource(resourceName, PathToRes(resourceName));
                ResourceList.push_back(resource);
                resourceIndex = (int)ResourceList.size() - 1;
            }

            if (!listResources) {
                Mod mod(fileName);
                std::fstream stream;
                stream.open(unzippedModPath, std::ios::in | std::ios::binary);
                stream.seekg(0, std::ios::end);
                unsigned long streamSize = stream.tellg();
                if (streamSize > maxSize.max_size()) {
                    std::cout << "Skipped " << fileName << " - too large." << std::endl;
                }
                stream.seekg(0, std::ios::beg);
                std::vector<std::byte> streamVector(streamSize);
                stream.read((char*)streamVector.data(), (long)streamSize);
                stream.close();
                mod.FileBytes = streamVector;

                std::string modFilePathPart3ToLower = ToLower(modFilePathParts[3]);
                std::string modFilePathPart4ToLower = ToLower(modFilePathParts[4]);
                if (modFilePathPart3ToLower == "eternalmod") {
                    if (modFilePathParts.size() == 6
                    && modFilePathPart4ToLower == "strings"
                    && std::filesystem::path(modFilePathParts[5]).extension() == ".json")
                    {
                        mod.isBlangJson = true;
                    }
                    else {
                        continue;
                    }
                }
                else {
                    mod.isBlangJson = false;
                }

                ResourceList[resourceIndex].ModList.push_back(mod);
                unzippedModCount++;
            }
        }
        else continue;
    }

    if (unzippedModCount > 0 && !(listResources)) {
        std::cout << "Found " << BLUE << unzippedModCount << " file(s) " << RESET << "in " << YELLOW << "'Mods' " << RESET << "folder..." << std::endl;
    }

    if (listResources) {

        for (auto& resource : ResourceList) {
            if (resource.Path.empty()) continue;
            std::cout << resource.Path << std::endl;
        }
        return 0;
    }

    long fileSize;

    for (int i = 0; i < ResourceList.size(); i++) {
        if (ResourceList[i].Path.empty()) {
            std::cout << RED << "WARNING: " << YELLOW << ResourceList[i].Name << ".resources" << RESET << " was not found! Skipping " << RED << ResourceList[i].ModList.size() << " file(s)" << RESET << "..." << std::endl;
            continue;
        }

        fileSize = (long)std::filesystem::file_size(ResourceList[i].Path);

        if (fileSize == 0) {
                std::cout << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << ResourceList[i].Path << RESET << " for writing!" << std::endl;
                continue;
        }

        mmap_allocator_namespace::mmappable_vector<std::byte> mem;
        mem.mmap_file(ResourceList[i].Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, fileSize, mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);

        ReadResource(mem, i);
        ReplaceChunks(mem, i);

        if (std::getenv("ETERNALMODLOADER_SKIP_ADDCHUNKS") == NULL) {
            AddChunks(mem, i);
        }

        mem.munmap_file();
    }

    std::cout << GREEN << "Finished." << RESET << std::endl;
    return 0;
}
