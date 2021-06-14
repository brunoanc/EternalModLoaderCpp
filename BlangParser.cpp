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
#include <algorithm>
#include <fstream>

#include "EternalModLoader.hpp"

BlangFile ParseBlang(std::vector<std::byte> &blangBytes, std::string& resourceName)
{
    BlangFile blangFile;
    std::vector<BlangString> blangStrings;
    int32_t pos = 0;

    if (resourceName == "gameresources_patch1") {
        std::vector<std::byte> unknownDataBytes(blangBytes.begin(), blangBytes.begin() + 8);
        std::reverse(unknownDataBytes.begin(), unknownDataBytes.end());
        std::copy(unknownDataBytes.begin(), unknownDataBytes.end(), (std::byte*)&blangFile.UnknownData);

        pos += 8;
    }

    std::vector<std::byte> stringAmountBytes(blangBytes.begin() + 8, blangBytes.begin() + 12);
    std::reverse(stringAmountBytes.begin(), stringAmountBytes.end());

    pos += 4;

    int32_t stringAmount;
    std::copy(stringAmountBytes.begin(), stringAmountBytes.end(), (std::byte*)&stringAmount);

    std::vector<std::byte> identifierBytes;
    std::vector<std::byte> textBytes;
    std::vector<std::byte> unknown;
    
    for (int32_t i = 0; i < stringAmount; i++) {
        uint32_t hash;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, (std::byte*)&hash);
        pos += 4;

        int32_t identifierLength;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, (std::byte*)&identifierLength);
        pos += 4;

        std::string identifier((char*)blangBytes.data() + pos, (char*)blangBytes.data() + pos + identifierLength);
        pos += identifierLength;

        int32_t textLength;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, (std::byte*)&textLength);
        pos += 4;

        std::string text((char*)blangBytes.data() + pos, (char*)blangBytes.data() + pos + textLength);
        pos += textLength;

        int32_t unknownLength;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, (std::byte*)&unknownLength);
        pos += 4;

        unknown.resize(unknownLength);
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + unknownLength, unknown.begin());
        pos += unknownLength;

        BlangString blangString(hash, identifier, text, unknown);
        blangStrings.push_back(blangString);
    }

    blangFile.Strings = blangStrings;

    return blangFile;
}

std::vector<std::byte> WriteBlangToVector(BlangFile blangFile, std::string& resourceName)
{
    std::vector<std::byte> blangBytes;

    for (int32_t i = 0; i < blangFile.Strings.size(); i++) {
        if (RemoveWhitespace(blangFile.Strings[i].Identifier).empty()) {
            blangFile.Strings.erase(blangFile.Strings.begin() + i);
            i--;
        }
    }

    if (resourceName == "gameresources_patch1") {
        std::vector<std::byte> unknownDataBytes((std::byte*)&blangFile.UnknownData, (std::byte*)&blangFile.UnknownData + 8);
        std::reverse(unknownDataBytes.begin(), unknownDataBytes.end());
        blangBytes.insert(blangBytes.end(), unknownDataBytes.begin(), unknownDataBytes.end());
    }

    int32_t stringsAmount = blangFile.Strings.size();
    std::vector<std::byte> stringsAmountBytes((std::byte*)&stringsAmount, (std::byte*)&stringsAmount + 4);
    std::reverse(stringsAmountBytes.begin(), stringsAmountBytes.end());
    blangBytes.insert(blangBytes.end(), stringsAmountBytes.begin(), stringsAmountBytes.end());

    std::vector<std::byte> identifierBytes;
    std::vector<std::byte> hashBytes(4);
    std::vector<std::byte> identifierBytesNew;
    std::vector<std::byte> textBytes;

    for (auto &blangString : blangFile.Strings) {
        std::string identifierToLower = ToLower(blangString.Identifier);
        identifierBytes.resize(identifierToLower.size());
        std::copy((std::byte*)identifierToLower.c_str(), (std::byte*)identifierToLower.c_str() + identifierToLower.size(), identifierBytes.begin());

        uint32_t fnvPrime = 0x01000193;
        blangString.Hash = 0x811C9DC5;

        for (int32_t i = 0; i < identifierBytes.size(); i++) {
            blangString.Hash ^= (int32_t)identifierBytes[i];
            blangString.Hash *= fnvPrime;
        }

        std::copy((std::byte*)&blangString.Hash, (std::byte*)&blangString.Hash + 4, hashBytes.begin());
        std::reverse(hashBytes.begin(), hashBytes.end());
        std::copy(hashBytes.begin(), hashBytes.end(), (std::byte*)&blangString.Hash);
        blangBytes.insert(blangBytes.end(), hashBytes.begin(), hashBytes.end());

        int32_t identifierBytesLength = blangString.Identifier.size();
        blangBytes.insert(blangBytes.end(), (std::byte*)&identifierBytesLength, (std::byte*)&identifierBytesLength + 4);
        blangBytes.insert(blangBytes.end(), (std::byte*)blangString.Identifier.c_str(), (std::byte*)blangString.Identifier.c_str() + blangString.Identifier.size());

        std::replace(blangString.Text.begin(), blangString.Text.end(), '\r', '\n');

        int32_t textBytesLength = blangString.Text.size();
        blangBytes.insert(blangBytes.end(), (std::byte*)&textBytesLength, (std::byte*)&textBytesLength + 4);
        blangBytes.insert(blangBytes.end(), (std::byte*)blangString.Text.c_str(), (std::byte*)blangString.Text.c_str() + blangString.Text.size());

        if (blangString.Unknown.empty()) {
            std::byte emptyArray[4] = {};
            blangBytes.insert(blangBytes.end(), emptyArray, emptyArray + 4);
        }
        else {
            int32_t unknownLength = blangString.Unknown.size();
            blangBytes.insert(blangBytes.end(), (std::byte*)&unknownLength, (std::byte*)&unknownLength + 4);
            blangBytes.insert(blangBytes.end(), blangString.Unknown.begin(), blangString.Unknown.end());
        }
    }

    return blangBytes;
}