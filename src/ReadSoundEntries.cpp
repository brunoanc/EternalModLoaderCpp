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

#include "ReadSoundEntries.hpp"

void ReadSoundEntries(MemoryMappedFile& memoryMappedFile, SoundContainer& soundContainer)
{
    // Read the info and the header sizes
    unsigned int infoSize, headerSize;
    std::copy(memoryMappedFile.Mem + 4, memoryMappedFile.Mem + 8, reinterpret_cast<std::byte*>(&infoSize));
    std::copy(memoryMappedFile.Mem + 8, memoryMappedFile.Mem + 12, reinterpret_cast<std::byte*>(&headerSize));

    size_t pos = headerSize + 12;

    // Loop through all the sound info entries and add them to our list
    for (unsigned int i = 0; i < (infoSize - headerSize) / 32; i++) {
        pos += 8;

        unsigned int soundId;
        std::copy(memoryMappedFile.Mem + pos, memoryMappedFile.Mem + pos + 4, reinterpret_cast<std::byte*>(&soundId));
        pos += 4;

        soundContainer.SoundEntries.push_back(SoundEntry(soundId, pos));
        pos += 20;
    }
}

std::vector<SoundEntry> GetSoundEntriesToModify(SoundContainer& soundContainer, unsigned int soundModId)
{
    std::vector<SoundEntry> soundEntries;

    // Loop through sound entries and get the ones with matching id
    for (auto& soundEntry : soundContainer.SoundEntries) {
        if (soundEntry.SoundId == soundModId) {
            soundEntries.push_back(soundEntry);
        }
    }

    return soundEntries;
}
