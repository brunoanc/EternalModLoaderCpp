#include "EternalModLoader.hpp"
#include <iostream>
#include <stdio.h>
#include <vector>
#include <filesystem>

#include "EternalModLoader.hpp"

uint32_t GetDecSize(std::vector<std::byte> &sound_bytes)
{
    FILE *p = popen("opusdec --quiet - tmp.wav", "w");

    if (!p)
        return -1;

    fwrite(sound_bytes.data(), 1, sound_bytes.size(), p);

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

int LoadSoundMods(std::vector<std::byte> &soundBytes, std::string sndPath, std::string soundFilename)
{
    std::string idStr = std::filesystem::path(soundFilename).stem();
    uint16_t format = std::filesystem::path(soundFilename).extension() == ".wem" ? 3 : 2;
    uint32_t encSize = soundBytes.size();
    uint32_t decSize = format == 2 ? GetDecSize(soundBytes) : soundBytes.size();

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
    fread(&infoSize, 4, 1, sndFile);

    uint32_t headerSize;
    fread(&headerSize, 4, 1, sndFile);

    fseek(sndFile, headerSize, SEEK_CUR);

    for (int i = 0; i < (infoSize - headerSize) / 32; i++) {
        fseek(sndFile, 8, SEEK_CUR);

        uint32_t id;
        fread(&id, 4, 1, sndFile);

        if (id != soundId) {
            fseek(sndFile, 20, SEEK_CUR);
            continue;
        }

        long currentPos = ftell(sndFile);

        fseek(sndFile, 0, SEEK_END);
        uint32_t offset = ftell(sndFile);
        fwrite(soundBytes.data(), 1, encSize, sndFile);

        fseek(sndFile, currentPos, SEEK_SET);

        fwrite(&encSize, 4, 1, sndFile);
        fwrite(&offset, 4, 1, sndFile);
        fwrite(&decSize, 4, 1, sndFile);
        fwrite(&format, 2, 1, sndFile);

        break;
    }

    return 0;
}