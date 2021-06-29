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
#include <algorithm>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <sstream>

#include "EternalModLoader.hpp"

const std::vector<std::string> SupportedFileFormats = { ".ogg", ".opus", ".wav", ".wem", ".flac", ".aiff", ".pcm" };

/**
 * @brief Get the opus file's decoded size
 * 
 * @param soundModFile SoundModFile containing the opus file to decode
 * @return Decoded opus size, or -1 on error
 */
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

/**
 * @brief Encode the given sound file
 * 
 * @param soundModFile SoundModFile object containing the sound file to encode
 * @return True on success, false otherwise
 */
bool EncodeSoundMod(SoundModFile &soundModFile)
{
    FILE *decFile = fopen("tmp.wav", "wb");

    if (!decFile)
        return false;

    if (fwrite(soundModFile.FileBytes.data(), 1, soundModFile.FileBytes.size(), decFile) != soundModFile.FileBytes.size())
        return false;

    fclose(decFile);

#ifdef _WIN32
    std::string command = BasePath + "opusenc.exe tmp.wav tmp.opus > NUL 2>&1";
#else
    std::string command = "opusenc tmp.wav tmp.opus >/dev/null 2>&1";
#endif

    if (system(command.c_str()) != 0)
        return false;

    try {
        soundModFile.FileBytes.resize(std::filesystem::file_size("tmp.opus"));

        if (soundModFile.FileBytes.size() == 0)
            throw std::exception();

    }
    catch (...) {
        return false;
    }

    FILE *encFile = fopen("tmp.opus", "rb");

    if (!encFile)
        return false;

    if (fread(soundModFile.FileBytes.data(), 1, soundModFile.FileBytes.size(), encFile) != soundModFile.FileBytes.size())
        return false;

    fclose(encFile);
    remove("tmp.ogg");

    return true;
}

/**
 * @brief Replace sounds in the given sound container file
 * 
 * @param memoryMappedFile MemoryMappedFile object containing the resource to modify
 * @param soundContainer SoundContainer object containing the sound container's data
 * @param os StringStream to output to
 */
void ReplaceSounds(MemoryMappedFile &memoryMappedFile, SoundContainer &soundContainer, std::stringstream &os)
{
    std::stable_sort(soundContainer.ModFileList.begin(), soundContainer.ModFileList.end(),
                     [](const SoundModFile &sound1, const SoundModFile &sound2) { return sound1.Parent.LoadPriority > sound2.Parent.LoadPriority; });

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
            os << RED << "ERROR: " << RESET << "Bad filename for sound file " << soundModFile.Name
                << " - sound file names should be named after the sound id, or have the sound id at the end of the filename with format _id#{{id here}}, skipping" << '\n';
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
                mtx.lock();

                if (!EncodeSoundMod(soundModFile))
                    throw std::exception();

                mtx.unlock();
                
                if (soundModFile.FileBytes.size() > 0) {
                    encodedSize = soundModFile.FileBytes.size();
                    format = 2;
                }
                else {
                    throw std::exception();
                }
            }
            catch (...) {
                os << RED << "ERROR: " << RESET << "Failed to encode sound mod file " << soundModFile.Name << " - corrupted?" << '\n';
                continue;
            }
        }

        if (format == -1) {
            os << RED << "ERROR: " << RESET << "Couldn't determine the sound file format for " << soundModFile.Name << ", skipping" << '\n';
            continue;
        }
        else if (format == 2 && needsDecoding) {
            try {
                mtx.lock();
                decodedSize = GetDecodedOpusFileSize(soundModFile);
                mtx.unlock();

                if (decodedSize == -1)
                    throw std::exception();
            }
            catch (...) {
                os << RED << "ERROR: " << RESET << "Failed to get decoded size for " << soundModFile.Name << " - corrupted file?" << '\n';
                continue;
            }
        }

        bool soundFound = false;
        uint32_t soundModOffset = memoryMappedFile.Size;
        int64_t newContainerSize = soundModOffset + soundModFile.FileBytes.size();

        if (!memoryMappedFile.ResizeFile(newContainerSize)) {
            os << RED << "ERROR: " << RESET << "Failed to resize " << soundContainer.Path << '\n';
            return;
        }
        
        std::copy(soundModFile.FileBytes.begin(), soundModFile.FileBytes.end(), memoryMappedFile.Mem + soundModOffset);

        std::vector<SoundEntry> soundEntriesToModify = GetSoundEntriesToModify(soundContainer, soundModId);

        if (soundEntriesToModify.empty()) {
            os << RED << "WARNING: " << RESET << "Couldn't find sound with id " << soundModId << " in "
                << soundContainer.Name << ", sound will not be replaced" << '\n';
            continue;
        }

        for (auto &soundEntry : soundEntriesToModify) {
            std::copy((std::byte*)&encodedSize, (std::byte*)&encodedSize + 4, memoryMappedFile.Mem + soundEntry.InfoOffset);
            std::copy((std::byte*)&soundModOffset, (std::byte*)&soundModOffset + 4, memoryMappedFile.Mem + soundEntry.InfoOffset + 4);
            std::copy((std::byte*)&decodedSize, (std::byte*)&decodedSize + 4, memoryMappedFile.Mem + soundEntry.InfoOffset + 8);

            uint16_t currentFormat;
            std::copy(memoryMappedFile.Mem + soundEntry.InfoOffset + 12, memoryMappedFile.Mem + soundEntry.InfoOffset + 14, (std::byte*)&currentFormat);

            if (currentFormat != format) {
                os << RED << "WARNING: " << RESET << "Format mismatch: sound file " << soundModFile.Name << " needs to be " << (currentFormat == 3 ? "WEM" : "OPUS") << " format." << '\n';
                os << "The sound will be replaced but it might not work in-game." << '\n';

                format = (int16_t)currentFormat;
            }
        }

        os << "\tReplaced sound with id " << soundModId << " with " << soundModFile.Name << '\n';
        fileCount++;
    }

    if (fileCount > 0) {
        os << "Number of sounds replaced: " << GREEN << fileCount << " sound(s) "
            << RESET << "in " << YELLOW << soundContainer.Path << RESET << "." << '\n';
    }

    if (SlowMode)
        os.flush();
}