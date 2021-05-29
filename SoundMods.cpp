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

#include <iostream>
#include <filesystem>

#include "EternalModLoader.hpp"

std::vector<std::string> SupportedFileFormats = { ".ogg", ".opus", ".wav", ".wem", ".flac", ".aiff", ".pcm" };

int GetDecodedOpusFileSize(SoundMod &soundMod)
{
    FILE *p = popen("opusdec - tmp.wav >/dev/null 2>&1", "w");

    if (!p)
        return -1;

    if (fwrite(soundMod.FileBytes.data(), 1, soundMod.FileBytes.size(), p) != soundMod.FileBytes.size()) 
        return -1;

    pclose(p);

    long decSize = -1;

    try {
        decSize = std::filesystem::file_size("tmp.wav");

        if (decSize == 0 || decSize == -1)
            throw;
    }
    catch (...) {
        return -1;
    }

    //remove("tmp.wav");

    return decSize + 20;
}

int EncodeSoundMod(SoundMod &soundMod)
{
    FILE *p = popen("opusenc - tmp.ogg >/dev/null 2>&1", "w");

    if (!p)
        return -1;

    if (fwrite(soundMod.FileBytes.data(), 1, soundMod.FileBytes.size(), p) != soundMod.FileBytes.size())
        return -1;

    pclose(p);

    try {
        soundMod.FileBytes.resize(std::filesystem::file_size("tmp.ogg"));

        if (soundMod.FileBytes.size() == 0)
            throw;

    }
    catch (...) {
        return -1;
    }

    FILE *encFile = fopen("tmp.ogg", "rb");

    if (!encFile)
        return -1;

    if (fread(soundMod.FileBytes.data(), 1, soundMod.FileBytes.size(), encFile) != soundMod.FileBytes.size())
        return -1;

    fclose(encFile);

    //remove("tmp.ogg");

    return 0;
}

void LoadSoundMods(mmap_allocator_namespace::mmappable_vector<std::byte> &mem, SoundBankInfo &soundBankInfo)
{
    int fileCount = 0;

    for (auto &soundMod : soundBankInfo.ModList) {
        std::string soundFileNameStem = std::filesystem::path(soundMod.Name).stem();
        int soundModId = -1;

        try {
            soundModId = std::stoul(soundFileNameStem, nullptr, 10);
        }
        catch (...) {
            std::vector<std::string> splitName = SplitString(soundFileNameStem, '_');
            std::string idString = splitName[splitName.size() - 1];
            std::vector<std::string> idStringData = SplitString(idString, '#');

            if (idStringData.size() == 2 && idStringData[0] == "id") {
                try {
                    soundModId = std::stoul(idStringData[1], nullptr, 10);
                }
                catch (...) {
                    soundModId = -1;
                }
            }
        }

        if (soundModId == -1) {
            std::cerr << RED << "ERROR: " << RESET << "Bad filename for sound file " << soundMod.Name
                << " - sound file names should be named after the sound id, or have the sound id at the end of the filename with format _id#{{id here}}, skipping" << std::endl;
            continue;
        }

        std::string soundExtension = std::filesystem::path(soundMod.Name).extension();
        int encodedSize = soundMod.FileBytes.size();
        int decodedSize = encodedSize;
        bool needsEncoding = false;
        short format = -1;

        if (soundExtension == ".wem") {
            format = 3;
        }
        else if (soundExtension == ".ogg" || soundExtension == ".opus") {
            format = 2;
        }
        else {
            needsEncoding = true;
        }

        if (needsEncoding) {
            try {
                if (EncodeSoundMod(soundMod) == -1)
                    throw;
                
                if (soundMod.FileBytes.size() > 0) {
                    encodedSize = soundMod.FileBytes.size();
                    format = 2;
                }
                else {
                    throw;
                }
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to encode sound mod file " << soundMod.Name << " - corrupted?" << std::endl;
                continue;
            }
        }

        if (format == -1) {
            std::cerr << RED << "ERROR: " << RESET << "Couldn't determine the sound file format for " << soundMod.Name << ", skipping" << std::endl;
            continue;
        }
        else if (format == 2) {
            try {
                decodedSize = GetDecodedOpusFileSize(soundMod);

                if (decodedSize == -1)
                    throw;
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to get decoded size for " << soundMod.Name << " - corrupted file?" << std::endl;
                continue;
            }
        }

        bool soundFound = false;

        unsigned int soundModOffset = mem.size();

        mem.munmap_file();
        std::filesystem::resize_file(soundBankInfo.Path, soundModOffset + soundMod.FileBytes.size());
        
        try {
            mem.mmap_file(soundBankInfo.Path, mmap_allocator_namespace::READ_WRITE_SHARED, 0, soundModOffset + soundMod.FileBytes.size(),
                mmap_allocator_namespace::MAP_WHOLE_FILE | mmap_allocator_namespace::ALLOW_REMAP);

            if (mem.empty())
                throw;
        }
        catch (...) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to load " << soundBankInfo.Path << " into memory for writing"<< std::endl;
            return;
        }
        
        std::copy(soundMod.FileBytes.begin(), soundMod.FileBytes.end(), mem.begin() + soundModOffset);

        unsigned int infoSize, headerSize;
        std::copy(mem.begin() + 4, mem.begin() + 8, (std::byte*)&infoSize);
        std::copy(mem.begin() + 8, mem.begin() + 12, (std::byte*)&headerSize);

        long pos = headerSize + 12;

        for (unsigned int i = 0, j = (infoSize - headerSize) / 32; i < j; i++) {
            pos += 8;

            unsigned int soundId;
            std::copy(mem.begin() + pos, mem.begin() + pos + 4, (std::byte*)&soundId);
            pos += 4;

            if (soundId != soundModId) {
                pos += 20;
                continue;
            }

            soundFound = true;

            std::copy((std::byte*)&encodedSize, (std::byte*)&encodedSize + 4, mem.begin() + pos);
            std::copy((std::byte*)&soundModOffset, (std::byte*)&soundModOffset + 4, mem.begin() + pos + 4);
            std::copy((std::byte*)&decodedSize, (std::byte*)&decodedSize + 4, mem.begin() + pos + 8);
            pos += 12;

            unsigned short currentFormat;
            std::copy(mem.begin() + pos, mem.begin() + pos + 2, (std::byte*)&currentFormat);
            pos += 8;

            if (currentFormat != format) {
                std::cerr << RED << "WARNING: " << RESET << "Format mismatch: sound file " << soundMod.Name << " needs to be " << (currentFormat == 3 ? "WEM" : "OPUS") << " format." << std::endl;
                std::cerr << "The sound will be replaced but it might not work in-game." << std::endl;

                format = (short)currentFormat;
            }
        }

        if (!soundFound) {
            std::cerr << RED << "WARNING: " << RESET << "Couldn't find sound with id " << soundModId << " in " << soundBankInfo.Name << std::endl;
            continue;
        }

        std::cout << "\tReplaced sound with id " << soundModId << " with " << soundMod.Name << std::endl;
        fileCount++;
    }

    if (fileCount > 0) {
        std::cout << "Number of sounds replaced: " << GREEN << fileCount << " sound(s) "
            << RESET << "in " << YELLOW << soundBankInfo.Path << RESET << "." << std::endl;
    }
}