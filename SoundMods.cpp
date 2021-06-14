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
#include <cstdio>

#include "EternalModLoader.hpp"

std::vector<std::string> SupportedFileFormats = { ".ogg", ".opus", ".wav", ".wem", ".flac", ".aiff", ".pcm" };

int GetDecodedOpusFileSize(SoundModFile &soundModFile)
{
#ifdef _WIN32
    FILE *p = _popen(std::string(BasePath + "opusdec.exe - tmp.wav > nul 2>&1").c_str(), "w");
#else
    FILE *p = popen("opusdec - tmp.wav >/dev/null 2>&1", "w");
#endif

    if (!p)
        return -1;

    if (fwrite(soundModFile.FileBytes.data(), 1, soundModFile.FileBytes.size(), p) != soundModFile.FileBytes.size()) 
        return -1;

#ifdef _WIN32
    if (_pclose(p) == -1)
#else
    if (pclose(p) == -1)
#endif
        return -1;

    long decSize = -1;

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

int EncodeSoundMod(SoundModFile &soundModFile)
{
#ifdef _WIN32
    FILE *p = _popen(std::string(BasePath + "opusenc.exe - tmp.ogg > nul 2>&1").c_str(), "w");
#else
    FILE *p = popen("opusenc - tmp.ogg >/dev/null 2>&1", "w");
#endif

    if (!p)
        return -1;

    if (fwrite(soundModFile.FileBytes.data(), 1, soundModFile.FileBytes.size(), p) != soundModFile.FileBytes.size())
        return -1;

#ifdef _WIN32
    if (_pclose(p) == -1)
#else
    if (pclose(p) == -1)
#endif
        return -1;

    try {
        soundModFile.FileBytes.resize(std::filesystem::file_size("tmp.ogg"));

        if (soundModFile.FileBytes.size() == 0)
            throw std::exception();

    }
    catch (...) {
        return -1;
    }

    FILE *encFile = fopen("tmp.ogg", "rb");

    if (!encFile)
        return -1;

    if (fread(soundModFile.FileBytes.data(), 1, soundModFile.FileBytes.size(), encFile) != soundModFile.FileBytes.size())
        return -1;

    fclose(encFile);

    remove("tmp.ogg");

    return 0;
}

void LoadSoundMods(FILE *&soundBankFile, SoundContainer &soundContainer)
{
    int fileCount = 0;

    for (auto &soundModFile : soundContainer.ModFileList) {
        std::string soundFileNameStem = std::filesystem::path(soundModFile.Name).stem().string();
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
            std::cerr << RED << "ERROR: " << RESET << "Bad filename for sound file " << soundModFile.Name
                << " - sound file names should be named after the sound id, or have the sound id at the end of the filename with format _id#{{id here}}, skipping" << std::endl;
            continue;
        }

        std::string soundExtension = std::filesystem::path(soundModFile.Name).extension().string();
        int encodedSize = soundModFile.FileBytes.size();
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
        else if (format == 2) {
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

        unsigned int soundModOffset = std::filesystem::file_size(soundContainer.Path);

        try {
            std::filesystem::resize_file(soundContainer.Path, soundModOffset + soundModFile.FileBytes.size());
        }
        catch (...) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to load " << soundContainer.Path << " into memory for writing"<< std::endl;
            return;
        }

        fseek(soundBankFile, soundModOffset, SEEK_SET);
        fwrite(soundModFile.FileBytes.data(), 1, soundModFile.FileBytes.size(), soundBankFile);

        fseek(soundBankFile, 4, SEEK_SET);

        unsigned int infoSize, headerSize;
        fread(&infoSize, 1, 4, soundBankFile);
        fread(&headerSize, 1, 4, soundBankFile);

        fseek(soundBankFile, headerSize, SEEK_CUR);

        for (unsigned int i = 0, j = (infoSize - headerSize) / 32; i < j; i++) {
            fseek(soundBankFile, 8, SEEK_CUR);

            unsigned int soundId;
            fread(&soundId, 4, 1, soundBankFile);

            if (soundId != soundModId) {
                fseek(soundBankFile, 20, SEEK_CUR);
                continue;
            }

            soundFound = true;

            fwrite(&encodedSize, 4, 1, soundBankFile);
            fwrite(&soundModOffset, 4, 1, soundBankFile);
            fwrite(&decodedSize, 4, 1, soundBankFile);

            unsigned short currentFormat;
            fread(&currentFormat, 2, 1, soundBankFile);

            fseek(soundBankFile, 6, SEEK_CUR);

            if (currentFormat != format) {
                std::cerr << RED << "WARNING: " << RESET << "Format mismatch: sound file " << soundModFile.Name << " needs to be " << (currentFormat == 3 ? "WEM" : "OPUS") << " format." << std::endl;
                std::cerr << "The sound will be replaced but it might not work in-game." << std::endl;

                format = (short)currentFormat;
            }
        }

        if (!soundFound) {
            std::cerr << RED << "WARNING: " << RESET << "Couldn't find sound with id " << soundModId << " in " << soundContainer.Name << std::endl;
            continue;
        }

        std::cout << "\tReplaced sound with id " << soundModId << " with " << soundModFile.Name << std::endl;
        fileCount++;
    }

    if (fileCount > 0) {
        std::cout << "Number of sounds replaced: " << GREEN << fileCount << " sound(s) "
            << RESET << "in " << YELLOW << soundContainer.Path << RESET << "." << std::endl;
    }
}