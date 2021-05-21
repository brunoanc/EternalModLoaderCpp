#include <iostream>
#include <stdio.h>
#include <vector>
#include <filesystem>

#include "EternalModLoader.hpp"

uint32_t GetDecSize(std::vector<std::byte> &soundBytes)
{
    FILE *p = popen("opusdec - tmp.wav >/dev/null 2>&1", "w");

    if (!p)
        return -1;

    if (fwrite(soundBytes.data(), 1, soundBytes.size(), p) != soundBytes.size())
        return -1;

    pclose(p);

    long decSize;

    try {
        decSize = std::filesystem::file_size("tmp.wav");

        if (decSize == 0)
            throw;
    }
    catch (...) {
        return -1;
    }

    remove("tmp.wav");

    return decSize + 20;
}

int EncodeWav(std::vector<std::byte> &soundBytes)
{
    FILE *p = popen("opusenc - tmp.ogg >/dev/null 2>&1", "w");

    if (!p)
        return -1;

    if (fwrite(soundBytes.data(), 1, soundBytes.size(), p) != soundBytes.size())
        return -1;

    pclose(p);

    try {
        soundBytes.resize(std::filesystem::file_size("tmp.ogg"));

        if (soundBytes.size() == 0)
            throw;

    }
    catch (...) {
        return -1;
    }

    FILE *encFile = fopen("tmp.ogg", "rb");

    if (!encFile)
        return -1;

    if (fread(soundBytes.data(), 1, soundBytes.size(), encFile) != soundBytes.size())
        return -1;

    fclose(encFile);

    remove("tmp.ogg");

    return 0;
}

int LoadSoundMods(std::vector<std::byte> &soundBytes, std::string sndPath, std::string soundFilename)
{
    std::string idStr = std::filesystem::path(soundFilename).stem();
    uint16_t format = std::filesystem::path(soundFilename).extension() == ".wem" ? 3 : 2;
    bool isWav = std::filesystem::path(soundFilename).extension() == ".wav" ? true : false;
    uint32_t encSize = soundBytes.size();
    uint32_t decSize;

    if (format == 2) {
        if (isWav) {
            decSize = soundBytes.size() + 20;

            if (EncodeWav(soundBytes) == -1)
                return -1;
        }
        else {
            decSize = GetDecSize(soundBytes);
        }
    }
    else {
        soundBytes.size();
    }

    if (decSize == -1)
        return -1;

    uint32_t soundId;

    try {
        soundId = std::stoul(idStr, nullptr, 10);
    }
    catch (...) {
        return -1;
    }

    FILE *sndFile = fopen(sndPath.c_str(), "rb+");

    if (!sndFile)
        return -1;

    fseek(sndFile, 4, SEEK_SET);

    uint32_t infoSize;

    if (fread(&infoSize, 4, 1, sndFile) != 1)
        return -1;

    uint32_t headerSize;

    if (fread(&headerSize, 4, 1, sndFile) != 1)
        return -1;

    fseek(sndFile, headerSize, SEEK_CUR);

    for (int i = 0; i < (infoSize - headerSize) / 32; i++) {
        fseek(sndFile, 8, SEEK_CUR);

        uint32_t id;

        if (fread(&id, 4, 1, sndFile) != 1)
            return -1;

        if (id != soundId) {
            fseek(sndFile, 20, SEEK_CUR);
            continue;
        }

        long currentPos = ftell(sndFile);

        fseek(sndFile, 0, SEEK_END);
        uint32_t offset = ftell(sndFile);

        if (fwrite(soundBytes.data(), 1, encSize, sndFile) != encSize)
            return -1;

        fseek(sndFile, currentPos, SEEK_SET);

        if (fwrite(&encSize, 4, 1, sndFile) != 1)
            return -1;

        if (fwrite(&offset, 4, 1, sndFile) != 1)
            return -1;

        if (fwrite(&decSize, 4, 1, sndFile) != 1)
            return -1;

        if (fwrite(&format, 2, 1, sndFile) != 1)
            return -1;

        break;
    }

    return 0;
}
