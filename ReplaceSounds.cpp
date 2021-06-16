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
#include <string>
#include <filesystem>
#include <cstdlib>

#include "EternalModLoader.hpp"

const std::vector<std::string> SupportedFileFormats = { ".ogg", ".opus", ".wav", ".wem", ".flac", ".aiff", ".pcm" };

int32_t GetDecodedOpusFileSize(SoundModFile &soundModFile)
{
    FILE *encFile = fopen("tmp.opus", "wb");

    if (!encFile)
        return -1;

    if (fwrite(soundModFile.FileBytes.data(), 1, soundModFile.FileBytes.size(), encFile) != soundModFile.FileBytes.size())
        return -1;

    fclose(encFile);

#ifdef _WIN32
    std::string command = BasePath + "opusdec.exe tmp.opus tmp.wav > NUL 2>&1";
#else
    std::string command = "opusdec tmp.opus tmp.wav >/dev/null 2>&1";
#endif

    if (system(command.c_str()) != 0)
        return -1;

    int64_t decSize = -1;

    try {
        decSize = std::filesystem::file_size("tmp.wav");

        if (decSize == 0 || decSize == -1)
            throw std::exception();
    }
    catch (...) {
        return -1;
    }

    remove("tmp.wav");

    return decSize + 20;
}

int32_t EncodeSoundMod(SoundModFile &soundModFile)
{
    FILE *decFile = fopen("tmp.wav", "wb");

    if (!decFile)
        return -1;

    if (fwrite(soundModFile.FileBytes.data(), 1, soundModFile.FileBytes.size(), decFile) != soundModFile.FileBytes.size())
        return -1;

    fclose(decFile);

#ifdef _WIN32
    std::string command = BasePath + "opusenc.exe tmp.wav tmp.opus > NUL 2>&1";
#else
    std::string command = "opusenc tmp.wav tmp.opus >/dev/null 2>&1";
#endif

    if (system(command.c_str()) != 0)
        return -1;

    try {
        soundModFile.FileBytes.resize(std::filesystem::file_size("tmp.opus"));

        if (soundModFile.FileBytes.size() == 0)
            throw std::exception();

    }
    catch (...) {
        return -1;
    }

    FILE *encFile = fopen("tmp.opus", "rb");

    if (!encFile)
        return -1;

    if (fread(soundModFile.FileBytes.data(), 1, soundModFile.FileBytes.size(), encFile) != soundModFile.FileBytes.size())
        return -1;

    fclose(encFile);
    remove("tmp.ogg");

    return 0;
}

#ifdef _WIN32
void ReplaceSounds(std::byte *&mem, HANDLE &hFile, HANDLE &fileMapping, SoundContainer &soundContainer)
#else
void ReplaceSounds(std::byte *&mem, int32_t &fd, SoundContainer &soundContainer)
#endif
{
    int32_t fileCount = 0;

    for (auto &soundModFile : soundContainer.ModFileList) {
        std::string soundFileNameStem = std::filesystem::path(soundModFile.Name).stem().string();
        int32_t soundModId = -1;

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
            std::cerr << RED << "ERROR: " << RESET << "Bad filename for sound file " << soundModFile.Name
                << " - sound file names should be named after the sound id, or have the sound id at the end of the filename with format _id#{{id here}}, skipping" << std::endl;
            continue;
        }

        std::string soundExtension = std::filesystem::path(soundModFile.Name).extension().string();
        int32_t encodedSize = soundModFile.FileBytes.size();
        int32_t decodedSize = encodedSize;
        bool needsEncoding = false;
        bool needsDecoding = true;
        int16_t format = -1;

        if (soundExtension == ".wem") {
            format = 3;
        }
        else if (soundExtension == ".ogg" || soundExtension == ".opus") {
            format = 2;
        }
        else if (soundExtension == ".wav") {
            format = 2;
            decodedSize = encodedSize + 20;
            needsDecoding = false;
            needsEncoding = true;
        }
        else {
            needsEncoding = true;
        }

        if (needsEncoding) {
            try {
                if (EncodeSoundMod(soundModFile) == -1)
                    throw std::exception();
                
                if (soundModFile.FileBytes.size() > 0) {
                    encodedSize = soundModFile.FileBytes.size();
                    format = 2;
                }
                else {
                    throw std::exception();
                }
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to encode sound mod file " << soundModFile.Name << " - corrupted?" << std::endl;
                continue;
            }
        }

        if (format == -1) {
            std::cerr << RED << "ERROR: " << RESET << "Couldn't determine the sound file format for " << soundModFile.Name << ", skipping" << std::endl;
            continue;
        }
        else if (format == 2 && needsDecoding) {
            try {
                decodedSize = GetDecodedOpusFileSize(soundModFile);

                if (decodedSize == -1)
                    throw std::exception();
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to get decoded size for " << soundModFile.Name << " - corrupted file?" << std::endl;
                continue;
            }
        }

        bool soundFound = false;
        uint32_t soundModOffset = std::filesystem::file_size(soundContainer.Path);
        int64_t newContainerSize = soundModOffset + soundModFile.FileBytes.size();

#ifdef _WIN32
        if (ResizeMmap(mem, hFile, fileMapping, newContainerSize) == -1) {
#else
        if (ResizeMmap(mem, fd, soundContainer.Path, soundModOffset, newContainerSize) == -1) {
#endif
            std::cerr << RED << "ERROR: " << RESET << "Failed to resize " << soundContainer.Path << std::endl;
            return;
        }
        
        std::copy(soundModFile.FileBytes.begin(), soundModFile.FileBytes.end(), mem + soundModOffset);

        std::vector<SoundEntry> soundEntriesToModify = GetSoundEntriesToModify(soundContainer, soundModId);

        if (soundEntriesToModify.empty()) {
            std::cerr << RED << "WARNING: " << RESET << "Couldn't find sound with id " << soundModId << " in "
                << soundContainer.Name << ", sound will not be replaced" << std::endl;
            continue;
        }

        for (auto &soundEntry : soundEntriesToModify) {
            std::copy((std::byte*)&encodedSize, (std::byte*)&encodedSize + 4, mem + soundEntry.InfoOffset);
            std::copy((std::byte*)&soundModOffset, (std::byte*)&soundModOffset + 4, mem + soundEntry.InfoOffset + 4);
            std::copy((std::byte*)&decodedSize, (std::byte*)&decodedSize + 4, mem + soundEntry.InfoOffset + 8);

            uint16_t currentFormat;
            std::copy(mem + soundEntry.InfoOffset + 12, mem + soundEntry.InfoOffset + 14, (std::byte*)&currentFormat);

            if (currentFormat != format) {
                std::cerr << RED << "WARNING: " << RESET << "Format mismatch: sound file " << soundModFile.Name << " needs to be " << (currentFormat == 3 ? "WEM" : "OPUS") << " format." << std::endl;
                std::cerr << "The sound will be replaced but it might not work in-game." << std::endl;

                format = (int16_t)currentFormat;
            }
        }

        std::cout << "\tReplaced sound with id " << soundModId << " with " << soundModFile.Name << std::endl;
        fileCount++;
    }

    if (fileCount > 0) {
        std::cout << "Number of sounds replaced: " << GREEN << fileCount << " sound(s) "
            << RESET << "in " << YELLOW << soundContainer.Path << RESET << "." << std::endl;
    }
}