#include <iostream>
#include "EternalModLoader.hpp"

/**
 * @brief Read all sound entries in the given sound container
 * 
 * @param memoryMappedFile MemoryMappedFile object containing the resource to read from
 * @param soundContainer SoundContainer object to read data from
 */
void ReadSoundEntries(MemoryMappedFile &memoryMappedFile, SoundContainer &soundContainer)
{
    // Read the info and the header sizes
    uint32_t infoSize, headerSize;
    std::copy(memoryMappedFile.Mem + 4, memoryMappedFile.Mem + 8, (std::byte*)&infoSize);
    std::copy(memoryMappedFile.Mem + 8, memoryMappedFile.Mem + 12, (std::byte*)&headerSize);

    int64_t pos = headerSize + 12;

    // Loop through all the sound info entries and add them to our list
    for (uint32_t i = 0; i < (infoSize - headerSize) / 32; i++) {
        pos += 8;

        uint32_t soundId;
        std::copy(memoryMappedFile.Mem + pos, memoryMappedFile.Mem + pos + 4, (std::byte*)&soundId);
        pos += 4;

        soundContainer.SoundEntries.push_back(SoundEntry(soundId, pos));
        pos += 20;
    }
}

/**
 * @brief Get sound entries to modify
 * 
 * @param soundContainer SoundContainer object to get entries from
 * @param soundModId id of the sound files to find
 */
std::vector<SoundEntry> GetSoundEntriesToModify(SoundContainer &soundContainer, uint32_t soundModId)
{
    std::vector<SoundEntry> soundEntries;

    // Loop through sound entries and get the ones with matching id
    for (auto &soundEntry : soundContainer.SoundEntries) {
        if (soundEntry.SoundId == soundModId) {
            soundEntries.push_back(soundEntry);
        }
    }

    return soundEntries;
}
