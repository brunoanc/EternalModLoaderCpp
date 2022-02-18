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

#include <sstream>
#include <algorithm>
#include <cstring>
#include "EternalModLoader.hpp"

const std::byte *StreamDBMagic = (std::byte*)"STREAMDB";

/**
 * @brief Constructs StreamDBHeader and StreamDBEntries table for the specified streamdb container object
 *
 * @param streamDBContainer StreamDB container info object
 * @param os StringStream to output to
 */
void BuildStreamDBIndex(StreamDBContainer &streamDBContainer, std::stringstream &os)
{
    // Get the streamdb mod file IDs
    for (auto &streamDBMod : streamDBContainer.ModFiles) {
        // Parse the identifier of the streamdb file we want to replace
        auto streamDBFileStem = fs::path(streamDBMod.Name).stem().string();
        uint64_t streamDBModId = 0;

        // Try to find the id at the end of the filename
        // Format: _#id{id here]
        auto splitName = SplitString(streamDBFileStem, '_');
        auto idString = splitName[splitName.size() - 1];
        auto idStringData = SplitString(idString, '#');

        if (idStringData.size() == 2 && idStringData[0] == "id") {
            try {
                streamDBModId = std::stoull(idStringData[1]);
            }
            catch (...) {
                streamDBModId = 0;
            }
        }

        if (streamDBModId == 0) {
            os << RED << "WARNING: " << RESET << "Bad filename for streamdb mod " << streamDBMod.Name << " - streamdb mod files should have the streamdb file id at the end of the filename with format \"_id#{{id here}}\", skipping\n";
            continue;
        }

        streamDBMod.FileId = streamDBModId;
    }

    // Remove mods with bad FileId
    streamDBContainer.ModFiles.erase(std::remove_if(streamDBContainer.ModFiles.begin(), streamDBContainer.ModFiles.end(),
        [](const StreamDBModFile &streamDbModFile) { return streamDbModFile.FileId == 0; }), streamDBContainer.ModFiles.end());

    if (streamDBContainer.ModFiles.empty()) {
        os.flush();
        return;
    }

    // Sort mod file list by priority
    std::stable_sort(streamDBContainer.ModFiles.begin(), streamDBContainer.ModFiles.end(),
        [](const StreamDBModFile &streamDbModFile1, const StreamDBModFile &streamDbModFile2) { return streamDbModFile1.Parent.LoadPriority > streamDbModFile2.Parent.LoadPriority; });

    // Remove mods with duplicate FileId.
    std::vector<StreamDBModFile> buffer;
    buffer.reserve(streamDBContainer.ModFiles.size());

    for (const auto &streamDBModFile : streamDBContainer.ModFiles) {
        bool found = false;

        // Check if buffer already has this file id
        for (int32_t i = 0; i < buffer.size(); i++) {
            if (buffer[i].FileId == streamDBModFile.FileId) {
                found = true;

                // Keep the one with bigger priority
                if (buffer[i].Parent.LoadPriority >= streamDBModFile.Parent.LoadPriority) {
                    buffer[i] = streamDBModFile;
                }
            }
        }

        if (!found) {
            // Add the streamdb mod file
            buffer.push_back(streamDBModFile);
        }
    }

    // Set mod file list to filtered buffer
    streamDBContainer.ModFiles = buffer;

    // Read the streamdb mod header
    for (auto &streamDBMod : streamDBContainer.ModFiles) {
        // Check for STREAMDB magic
        if (std::memcmp(streamDBMod.FileData.data(), StreamDBMagic, 8) != 0) {
            os << RED << "WARNING: " << RESET << "streamdb mod \"" << streamDBMod.Name << "\" is missing a required header. Skipping...\n";
            continue;
        }

        // Read LOD info
        int32_t offset = 8;
        streamDBMod.LODcount = *(uint32_t*)(streamDBMod.FileData.data() + offset);
        offset += 4;

        for (int32_t i = 0; i < streamDBMod.LODcount; i++) {
            streamDBMod.LODDataOffset.push_back(*(uint32_t*)(streamDBMod.FileData.data() + offset));
            streamDBMod.LODDataLength.push_back(*(uint32_t*)(streamDBMod.FileData.data() + offset + 4));
            offset += 8;
        }
    }

    // Remove mods with missing LODCount - this means we couldn't read STREAMDB header
    streamDBContainer.ModFiles.erase(std::remove_if(streamDBContainer.ModFiles.begin(), streamDBContainer.ModFiles.end(),
        [](const StreamDBModFile &streamDbModFile) { return streamDbModFile.LODcount == 0; }), streamDBContainer.ModFiles.end());

    if (streamDBContainer.ModFiles.empty()) {
        os.flush();
        return;
    }

    // Copy the filedata we need for each LOD
    for (auto &streamDBMod : streamDBContainer.ModFiles) {
        for (int32_t i = 0; i < streamDBMod.LODcount; i++) {
            int32_t lodDataOffset = streamDBMod.LODDataOffset[i];
            int32_t lodDataLength = streamDBMod.LODDataLength[i];

            std::vector<std::byte> lodData(streamDBMod.FileData.data() + lodDataOffset, streamDBMod.FileData.data() + lodDataOffset + lodDataLength);
            streamDBMod.LODFileData.push_back(lodData);
        }
    }

    // Sort mod file list by FileId
    std::stable_sort(streamDBContainer.ModFiles.begin(), streamDBContainer.ModFiles.end(),
        [](const StreamDBModFile &streamDbModFile1, const StreamDBModFile &streamDbModFile2) { return streamDbModFile1.FileId < streamDbModFile2.FileId; });

    // Build the streamdb index in numerical order by FileId
    for (auto &streamDBMod : streamDBContainer.ModFiles) {
        for (int32_t i = 0; i < streamDBMod.LODcount; i++) {
            uint64_t fileId = streamDBMod.FileId + i;

            StreamDBEntry streamDBEntry(fileId, 0, streamDBMod.LODDataLength[i], streamDBMod.Name, streamDBMod.LODFileData[i]);
            streamDBContainer.StreamDBEntries.push_back(streamDBEntry);
        }
    }

    // Build the streamdb header
    int32_t dataStartOffset = 32 + (streamDBContainer.StreamDBEntries.size() * 16) + 16;
    streamDBContainer.Header = StreamDBHeader(dataStartOffset, streamDBContainer.StreamDBEntries.size());

    // First StreamDBEntry dataOffset16 is known
    streamDBContainer.StreamDBEntries[0].DataOffset16 = dataStartOffset / 16;

    // Calculate additional StreamDBEntry data offsets after the first
    for (int32_t i = 1; i < streamDBContainer.StreamDBEntries.size(); i++) {
        uint32_t previousOffset = streamDBContainer.StreamDBEntries[i - 1].DataOffset16 * 16;
        uint32_t thisOffset = previousOffset + streamDBContainer.StreamDBEntries[i - 1].DataLength;

        if (thisOffset % 16 != 0) {
            // Integer math, gets next offset evenly divisible by 16
            thisOffset = thisOffset + 16 - (thisOffset % 16);
        }

        streamDBContainer.StreamDBEntries[i].DataOffset16 = thisOffset / 16;
    }
}

/**
 * @brief Writes a new streamdb file containing all mods present in the specified streamdb container
 *
 * @param streamDBFile FILE* to write to
 * @param streamDBContainer StreamDB container info object
 * @param os StringStream to output to
 */
void WriteStreamDBFile(FILE *&streamDBFile, const StreamDBContainer &streamDBContainer, std::stringstream &os)
{
    int32_t fileCount = 0;

    // Write the StreamDBHeader
    fwrite(&streamDBContainer.Header.Magic, 1, 8, streamDBFile);
    fwrite(&streamDBContainer.Header.DataStartOffset, 1, 4, streamDBFile);
    fwrite(&streamDBContainer.Header.Padding0, 1, 4, streamDBFile);
    fwrite(&streamDBContainer.Header.Padding1, 1, 4, streamDBFile);
    fwrite(&streamDBContainer.Header.Padding2, 1, 4, streamDBFile);
    fwrite(&streamDBContainer.Header.NumEntries, 1, 4, streamDBFile);
    fwrite(&streamDBContainer.Header.Flags, 1, 4, streamDBFile);

    // Write the StreamDBEntry table
    for (const auto& streamDBEntry : streamDBContainer.StreamDBEntries) {
        fwrite(&streamDBEntry.FileId, 1, 8, streamDBFile);
        fwrite(&streamDBEntry.DataOffset16, 1, 4, streamDBFile);
        fwrite(&streamDBEntry.DataLength, 1, 4, streamDBFile);
    }

    // Write the StreamDBPrefetchBlock
    int32_t numPrefetchBlocks = 0;
    int32_t totalPrefetchLength = 8;

    fwrite(&numPrefetchBlocks, 1, 4, streamDBFile);
    fwrite(&totalPrefetchLength, 1, 4, streamDBFile);

    // Write the actual mod files
    for (const auto &streamDBEntry : streamDBContainer.StreamDBEntries) {
        // Write padding (max 15 bytes)
        int64_t desiredOffset = streamDBEntry.DataOffset16 * 16;
        int64_t offsetDiff = desiredOffset - ftell(streamDBFile);

        // If the offsetDiff is outside this range, we have a corrupt StreamDBEntries table and need to abort
        if (offsetDiff < 0 || offsetDiff > 15) {
            fclose(streamDBFile);
            std::error_code ec;
            fs::remove(streamDBContainer.Path, ec);
            StreamDBContainerList.erase(std::find_if(StreamDBContainerList.begin(), StreamDBContainerList.end(),
                [&](const StreamDBContainer &streamDBContainer2) { return streamDBContainer2.Name == streamDBContainer.Name; }));

            os << RED << "ERROR: " << RESET << "Failed to build \"" << streamDBContainer.Name << "\" file.\n";
            os.flush();
            break;
        }

        // Write null padding
        std::vector<std::byte> padding(offsetDiff, (std::byte)0);
        fwrite(padding.data(), 1, offsetDiff, streamDBFile);

        // Write mod file data
        fwrite(streamDBEntry.FileData.data(), 1, streamDBEntry.FileData.size(), streamDBFile);

        os << "\tAdded streamdb mod file with id " << streamDBEntry.FileId << " [" << streamDBEntry.Name << "]\n";
        fileCount++;
    }

    if (fileCount > 0) {
        os << "Number of streamdb entries replaced: " << GREEN << fileCount
            << " streamdb entries" << RESET << " in " << YELLOW << streamDBContainer.Path << RESET << '\n';
    }

    os.flush();
}
