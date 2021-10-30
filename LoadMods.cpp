#include <iostream>
#include <sstream>
#include "EternalModLoader.hpp"

/**
 * @brief Load mods to the given resource file
 * 
 * @param resourceContainer ResourceContainer containing the resource to load the mods into
 */
void LoadResourceMods(ResourceContainer &resourceContainer)
{
    // Get stringstream to store output
    mtx.lock();
    std::stringstream &os = stringStreams[streamIndex++];
    mtx.unlock();

    if (!MultiThreading) {
        // Redirect output to stdout directly
        ((std::ostream&)os).rdbuf(std::cout.rdbuf());
    }

    // Load resource into memory as mmap
    MemoryMappedFile *memoryMappedFile;

    try {
        memoryMappedFile = new MemoryMappedFile(resourceContainer.Path);
    }
    catch (...) {
        os << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

    // Load mods
    ReadResource(*memoryMappedFile, resourceContainer);
    ReplaceChunks(*memoryMappedFile, resourceContainer, os);
    AddChunks(*memoryMappedFile, resourceContainer, os);

    delete memoryMappedFile;
}

/**
 * @brief Load sound mods to the given sound container file
 * 
 * @param soundContainer SoundContainer containing the sound container to load the mods into
 */
void LoadSoundMods(SoundContainer &soundContainer)
{
    // Get stringstream to store output
    mtx.lock();
    std::stringstream &os = stringStreams[streamIndex++];
    mtx.unlock();

    if (!MultiThreading) {
        // Redirect output to stdout directly
        ((std::ostream&)os).rdbuf(std::cout.rdbuf());
    }

    // Load snd into memory as mmap
    MemoryMappedFile *memoryMappedFile;

    try {
        memoryMappedFile = new MemoryMappedFile(soundContainer.Path);
    }
    catch (...) {
        os << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

    // Load sound mods
    ReadSoundEntries(*memoryMappedFile, soundContainer);
    ReplaceSounds(*memoryMappedFile, soundContainer, os);

    delete memoryMappedFile;
}
