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
#include <sstream>
#include <mutex>
#include "AddChunks.hpp"
#include "Colors.hpp"
#include "MemoryMappedFile.hpp"
#include "ProgramOptions.hpp"
#include "ReadResourceFile.hpp"
#include "ReadSoundEntries.hpp"
#include "ReplaceChunks.hpp"
#include "ReplaceSounds.hpp"
#include "WriteStreamDB.hpp"
#include "LoadMods.hpp"

extern std::mutex mtx;

// String streams for output
std::vector<std::stringstream> StringStreams;
size_t StreamIndex{0};

void InitStringStreams(size_t count)
{
    StringStreams.resize(count);
}

void OutputStringStream(int index)
{
    std::cout << StringStreams[index].rdbuf();
}

void LoadResourceMods(ResourceContainer& resourceContainer,
    std::map<uint64_t, ResourceDataEntry>& resourceDataMap,
    std::unique_ptr<std::byte[]>& buffer, int bufferSize)
{
    // Get stringstream to store output
    mtx.lock();
    std::stringstream& os = StringStreams[StreamIndex++];
    mtx.unlock();

    if (!ProgramOptions::MultiThreading) {
        // Redirect output to stdout directly
        reinterpret_cast<std::ostream&>(os).rdbuf(std::cout.rdbuf());
    }

    // Load resource into memory as mmap
    std::unique_ptr<MemoryMappedFile> memoryMappedFile;

    try {
        memoryMappedFile = std::make_unique<MemoryMappedFile>(resourceContainer.Path);
    }
    catch (...) {
        os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to open " << Colors::Yellow << resourceContainer.Path << Colors::Reset << " for writing!" << std::endl;
        return;
    }

    // Load mods
    ReadResource(*memoryMappedFile, resourceContainer);
    ReplaceChunks(*memoryMappedFile, resourceContainer, resourceDataMap, os, buffer, bufferSize);
    AddChunks(*memoryMappedFile, resourceContainer, resourceDataMap, os);
}

void LoadSoundMods(SoundContainer& soundContainer)
{
    // Get stringstream to store output
    mtx.lock();
    std::stringstream& os = StringStreams[StreamIndex++];
    mtx.unlock();

    if (!ProgramOptions::MultiThreading) {
        // Redirect output to stdout directly
        reinterpret_cast<std::ostream&>(os).rdbuf(std::cout.rdbuf());
    }

    // Load snd into memory as mmap
    std::unique_ptr<MemoryMappedFile> memoryMappedFile;

    try {
        memoryMappedFile = std::make_unique<MemoryMappedFile>(soundContainer.Path);
    }
    catch (...) {
        os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to open " << Colors::Yellow << soundContainer.Path << Colors::Reset << " for writing!" << std::endl;
        return;
    }

    // Load sound mods
    ReadSoundEntries(*memoryMappedFile, soundContainer);
    ReplaceSounds(*memoryMappedFile, soundContainer, os);
}

void LoadStreamDBMods(StreamDBContainer& streamDBContainer, std::vector<StreamDBContainer>& streamDBContainerList)
{
    // Get stringstream to store output
    mtx.lock();
    std::stringstream& os = StringStreams[StreamIndex++];
    mtx.unlock();

    if (!ProgramOptions::MultiThreading) {
        // Redirect output to stdout directly
        reinterpret_cast<std::ostream&>(os).rdbuf(std::cout.rdbuf());
    }

    // Construct StreamDBHeader and StreamDBEntries list in memory
    BuildStreamDBIndex(streamDBContainer, os);

    // Open streamdb file
    FILE *streamDBFile = fopen(streamDBContainer.Path.c_str(), "wb+");

    if (streamDBFile == nullptr) {
        os << Colors::Red << "ERROR: " << Colors::Reset << "Failed to open " << Colors::Yellow << streamDBContainer.Path << Colors::Reset << " for writing!" << std::endl;
        return;
    }

    // Write the custom streamdb file
    WriteStreamDBFile(streamDBFile, streamDBContainer, streamDBContainerList, os);

    // Close file
    fclose(streamDBFile);
}
