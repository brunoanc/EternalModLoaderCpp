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

BlangFile ParseBlang(std::vector<std::byte>& blangBytes, std::string& resourceName) {
    BlangFile blangFile;
    std::vector<BlangString> blangStrings;
    int pos;

    if (resourceName == "gameresources_patch1") {
        std::vector<std::byte> unknownDataBytes(blangBytes.begin(), blangBytes.begin() + 8);
        pos += 8;
        std::reverse(unknownDataBytes.begin(), unknownDataBytes.end());
        blangFile.UnknownData = VectorToNumber(unknownDataBytes, 8);
    }

    std::vector<std::byte> stringAmountBytes(blangBytes.begin() + 8, blangBytes.begin() + 12);
    pos += 4;
    std::reverse(stringAmountBytes.begin(), stringAmountBytes.end());
    int stringAmount = VectorToNumber(stringAmountBytes, 4);

    std::vector<std::byte> identifierBytes;
    std::vector<std::byte> textBytes;
    std::vector<std::byte> unknown;
    for (int i = 0; i < stringAmount; i++) {
        std::vector<std::byte> hashBytes(blangBytes.begin() + pos, blangBytes.begin() + pos + 4);
        pos += 4;
        unsigned int hash = VectorToNumber(hashBytes, 4);

        int identifierLength;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, (std::byte*)&identifierLength);
        pos += 4;

        identifierBytes.resize(identifierLength);
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + identifierLength, identifierBytes.begin());
        pos += identifierLength;
        std::string identifier((const char*)identifierBytes.data(), identifierBytes.size());

        int textLength;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, (std::byte*)&textLength);
        pos += 4;

        textBytes.resize(textLength);
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + textLength, textBytes.begin());
        pos += textLength;
        std::string text((const char*)textBytes.data(), textLength);

        int unknownLength;
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

std::vector<std::byte> WriteBlangToVector(BlangFile blangFile, std::string& resourceName) {
    std::vector<std::byte> blangBytes;

    for (int i = 0; i < blangFile.Strings.size(); i++) {
        if (RemoveWhitespace(blangFile.Strings[i].Identifier).empty()) {
            blangFile.Strings.erase(blangFile.Strings.begin() + i);
        }
    }
    if (resourceName == "gameresources_patch1") {
        std::vector<std::byte> unknownDataBytes = LongToVector(blangFile.UnknownData, 8);
        std::reverse(unknownDataBytes.begin(), unknownDataBytes.end());
        blangBytes.insert(blangBytes.end(), unknownDataBytes.begin(), unknownDataBytes.end());
    }

    std::vector<std::byte> stringsAmountBytes = LongToVector(blangFile.Strings.size(), 4);
    std::reverse(stringsAmountBytes.begin(), stringsAmountBytes.end());
    blangBytes.insert(blangBytes.end(), stringsAmountBytes.begin(), stringsAmountBytes.end());

    std::vector<std::byte> identifierBytes;
    std::vector<std::byte> hashBytes;
    std::vector<std::byte> identifierBytesNew;
    std::vector<std::byte> textBytes;


    for (auto& blangString : blangFile.Strings) {
        std::string identifierToLower = ToLower(blangString.Identifier);
        identifierBytes.resize(identifierToLower.size());
        std::copy((std::byte*)identifierToLower.c_str(), (std::byte*)identifierToLower.c_str() + identifierToLower.size(),identifierBytes.begin());
        unsigned int fnvPrime = 0x01000193;
        blangString.Hash = 0x811C9DC5;

        for (int i = 0; i < identifierBytes.size(); i++) {
            blangString.Hash ^= (int)identifierBytes[i];
            blangString.Hash *= fnvPrime;
        }

        hashBytes = LongToVector(blangString.Hash, 4);
        std::reverse(hashBytes.begin(), hashBytes.end());
        blangString.Hash = VectorToNumber(hashBytes, 4);
        blangBytes.insert(blangBytes.end(), hashBytes.begin(), hashBytes.end());

        identifierBytesNew.resize(blangString.Identifier.size());
        std::copy((std::byte*)blangString.Identifier.c_str(), (std::byte*)blangString.Identifier.c_str() + blangString.Identifier.size(), identifierBytesNew.begin());
        int identifierBytesLength = identifierBytes.size();
        blangBytes.insert(blangBytes.end(), (std::byte*)&identifierBytesLength, (std::byte*)&identifierBytesLength + 4);
        blangBytes.insert(blangBytes.end(), identifierBytesNew.begin(), identifierBytesNew.end());
        

        std::replace(blangString.Text.begin(), blangString.Text.end(), '\r', '\n');

        textBytes.resize(blangString.Text.size());
        std::copy((std::byte*)blangString.Text.c_str(), (std::byte*)blangString.Text.c_str() + blangString.Text.size(), textBytes.begin());
        int textBytesLength = textBytes.size();
        blangBytes.insert(blangBytes.end(), (std::byte*)&textBytesLength, (std::byte*)&textBytesLength + 4);
        blangBytes.insert(blangBytes.end(), textBytes.begin(), textBytes.end());

        if (blangString.Unknown.empty()) {
            std::byte emptyArray[4] = {(std::byte)0, (std::byte)0, (std::byte)0, (std::byte)0};
            blangBytes.insert(blangBytes.end(), emptyArray, emptyArray + 4);
        }
        else {
            int unknownLength = blangString.Unknown.size();
            blangBytes.insert(blangBytes.end(), (std::byte*)&unknownLength, (std::byte*)&unknownLength + 4);
            blangBytes.insert(blangBytes.end(), blangString.Unknown.begin(), blangString.Unknown.end());
        }
    }

    return blangBytes;
}