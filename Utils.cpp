#include <iostream>

#include "EternalModLoader.hpp"

int GetResourceInfo(std::string resourceName) {
    for (int i = 0; i < ResourceList.size(); i++) {
        if (ResourceList[i].Name == resourceName) {
            return i;
        }
    }
    return -1;
}

int GetChunk(std::string name, int resourceIndex) {
    for (int i = 0; i < ResourceList[resourceIndex].ChunkList.size(); i++) {
        if (ResourceList[resourceIndex].ChunkList[i].Name.find(name) != std::string::npos) {
            return i;
        }
    }
    return -1;
}
