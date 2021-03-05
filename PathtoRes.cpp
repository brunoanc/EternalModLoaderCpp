#include <iostream>
#include <algorithm>
#include <filesystem>

#include "EternalModLoader.hpp"

std::string PathToRes(std::string name) {
    std::string resourcePath;

    std::string nameToLower = name;
    std::transform(nameToLower.begin(), nameToLower.end(), nameToLower.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    if (nameToLower.find("dlc_hub", 0) == 0) {
        std::string dlcHubFileName = name.substr(4, name.size() -4);
        resourcePath = "./base/game/dlc/hub/" + dlcHubFileName + ".resources";
    }
    else if (nameToLower.find("hub", 0) == 0) {
        resourcePath = "./base/game/hub/" + name + ".resources";
    }
    else {
        resourcePath = name + ".resources";
    }

    for (auto& file : std::filesystem::recursive_directory_iterator(BasePath)) {
        std::string path = file.path();
        std::string base_filename = path.substr(path.find_last_of('/') + 1);
        if (base_filename == resourcePath || path == resourcePath) {
            return path;
        }
        else continue;
    }
    std::string emptyString;
    return emptyString;
}
