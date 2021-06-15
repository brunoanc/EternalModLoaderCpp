#include <iostream>

#include "EternalModLoader.hpp"

void ReadSoundEntries(std::byte *mem, SoundContainer &soundContainer)
{
    uint32_t infoSize, headerSize;
    std::copy(mem + 4, mem + 8, (std::byte*)&infoSize);
    std::copy(mem + 8, mem + 12, (std::byte*)&headerSize);

    int64_t pos = headerSize + 12;

    for (uint32_t i = 0, j = (infoSize - headerSize) / 32; i < j; i++) {
        pos += 8;

        uint32_t soundId;
        std::copy(mem + pos, mem + pos + 4, (std::byte*)&soundId);
        pos += 4;

        soundContainer.SoundEntries.push_back(SoundEntry(soundId, pos));
        pos += 20;
    }
}

std::vector<SoundEntry> GetSoundEntriesToModify(SoundContainer &soundContainer, uint32_t soundModId)
{
    std::vector<SoundEntry> soundEntries;

    for (auto &soundEntry : soundContainer.SoundEntries) {
        if (soundEntry.SoundId == soundModId)
            soundEntries.push_back(soundEntry);
    }

    return soundEntries;
}