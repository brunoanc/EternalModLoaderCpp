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
#include <vector>

#include "EternalModLoader.hpp"

void ReadResource(FILE *&resourceFile, ResourceContainer &resourceContainer)
{
    fseek(resourceFile, 0x20, SEEK_SET);

    int fileCount;
    fread(&fileCount, 4, 1, resourceFile);

    int unknownCount;
    fread(&unknownCount, 4, 1, resourceFile);

    int dummy2Num;
    fread(&dummy2Num, 4, 1, resourceFile);

    fseek(resourceFile, 0x38, SEEK_SET);

    int stringsSize;
    fread(&stringsSize, 4, 1, resourceFile);

    fseek(resourceFile, 0x40, SEEK_SET);

    long namesOffset;
    fread(&namesOffset, 8, 1, resourceFile);

    long namesEnd;
    fread(&namesEnd, 8, 1, resourceFile);

    long infoOffset;
    fread(&infoOffset, 8, 1, resourceFile);

    fseek(resourceFile, 0x60, SEEK_SET);

    long dummy7OffsetOrg;
    fread(&dummy7OffsetOrg, 8, 1, resourceFile);

    long dataOffset;
    fread(&dataOffset, 8, 1, resourceFile);

    fseek(resourceFile, 0x74, SEEK_SET);

    long idclOffset;
    fread(&idclOffset, 8, 1, resourceFile);

    fseek(resourceFile, namesOffset, SEEK_SET);

    long namesNum;
    fread(&namesNum, 8, 1, resourceFile);

    fseek(resourceFile, namesOffset + 8 + (namesNum * 8), SEEK_SET);

    long namesOffsetEnd = ftell(resourceFile);
    long namesSize = namesEnd - namesOffsetEnd;

    std::vector<ResourceName> namesList;
    std::vector<char> currentNameBytes;
    char currentByte;
    
    for (int i = 0; i < namesSize; i++) {
        currentByte = fgetc(resourceFile);

        if (currentByte == 0 || i == namesSize - 1) {
            if (currentNameBytes.empty())
                continue;

            std::string fullFileName(currentNameBytes.data(), currentNameBytes.size());
            std::string normalizedFileName = NormalizeResourceFilename(fullFileName);

            ResourceName resourceName(fullFileName, normalizedFileName);
            namesList.push_back(resourceName);
            
            currentNameBytes.clear();
            continue;
        }

        currentNameBytes.push_back(currentByte);
    }

    resourceContainer.FileCount = fileCount;
    resourceContainer.TypeCount = dummy2Num;
    resourceContainer.StringsSize = stringsSize;
    resourceContainer.NamesOffset = namesOffset;
    resourceContainer.InfoOffset = infoOffset;
    resourceContainer.Dummy7Offset = dummy7OffsetOrg;
    resourceContainer.DataOffset = dataOffset;
    resourceContainer.IdclOffset = idclOffset;
    resourceContainer.UnknownCount = unknownCount;
    resourceContainer.FileCount2 = fileCount * 2;
    resourceContainer.NamesOffsetEnd = namesOffsetEnd;
    resourceContainer.UnknownOffset = namesEnd;
    resourceContainer.UnknownOffset2 = namesEnd;
    resourceContainer.NamesList = namesList;

    ReadChunkInfo(resourceFile, resourceContainer);
}