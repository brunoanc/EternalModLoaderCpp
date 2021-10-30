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
#include <cstring>
#include "Utils/Utils.hpp"
#include "BlangFile/BlangFile.hpp"

/**
 * @brief Construct a new BlangFile object
 * 
 * @param blangBytes Vector containing the blang's bytes
 */
BlangFile::BlangFile(const std::vector<std::byte> &blangBytes)
{
    int32_t pos = 0;

    // Check where the blang file entries start
    std::string str((char*)blangBytes.data() + 12, 5);

    if (ToLower(str) != "#str_") {
        // Read unknown data (big endian)
        std::copy(blangBytes.begin(), blangBytes.begin() + 8, (std::byte*)&UnknownData);
        std::reverse((std::byte*)&UnknownData, (std::byte*)&UnknownData + 8);
        pos += 8;
    }

    // Read the string amount (big endian)
    uint32_t stringAmount;
    std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, (std::byte*)&stringAmount);
    std::reverse((std::byte*)&stringAmount, (std::byte*)&stringAmount + 4);
    pos += 4;

    // Parse each string
    std::vector<std::byte> identifierBytes;
    std::vector<std::byte> textBytes;
    std::vector<std::byte> unknown;

    Strings.reserve(stringAmount);
    
    for (uint32_t i = 0; i < stringAmount; i++) {
        // Read string hash
        uint32_t hash;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, (std::byte*)&hash);
        pos += 4;

        // Read string identifier
        int32_t identifierLength;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, (std::byte*)&identifierLength);
        pos += 4;

        std::string identifier((char*)blangBytes.data() + pos, (char*)blangBytes.data() + pos + identifierLength);
        pos += identifierLength;

        // Read string
        int32_t textLength;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, (std::byte*)&textLength);
        pos += 4;

        std::string text((char*)blangBytes.data() + pos, (char*)blangBytes.data() + pos + textLength);
        pos += textLength;

        // Read unknown data
        int32_t unknownLength;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, (std::byte*)&unknownLength);
        pos += 4;

        unknown.resize(unknownLength);
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + unknownLength, unknown.begin());
        pos += unknownLength;

        BlangString blangString(hash, identifier, text, unknown);
        Strings.push_back(blangString);
    }
}

/**
 * @brief Convert BlangFile object back to bytes
 * 
 * @return Vector containing the blang's bytes
 */
std::vector<std::byte> BlangFile::ToByteVector()
{
    std::vector<std::byte> blangBytes;

    // Delete invalid strings first
    // Strings must have a valid identifier
    for (int32_t i = Strings.size() - 1; i >= 0; i--) {
        if (RemoveWhitespace(Strings[i].Identifier).empty()) {
            Strings.erase(Strings.begin() + i);
        }
    }

    // Write unknown data (big endian)
    blangBytes.insert(blangBytes.end(), (std::byte*)&UnknownData, (std::byte*)&UnknownData + 8);
    std::reverse(blangBytes.end() - 8, blangBytes.end());

    // Write string amount (big endian)
    int32_t stringsAmount = Strings.size();
    blangBytes.insert(blangBytes.end(), (std::byte*)&stringsAmount, (std::byte*)&stringsAmount + 4);
    std::reverse(blangBytes.end() - 4, blangBytes.end());

    std::vector<std::byte> identifierBytes;
    std::vector<std::byte> hashBytes(4);
    std::vector<std::byte> identifierBytesNew;
    std::vector<std::byte> textBytes;

    // Write each string
    for (auto &blangString : Strings) {
        // Calculate teh hash of the identifier string (FNV1A32)
        std::string identifierToLower = ToLower(blangString.Identifier);
        identifierBytes.resize(identifierToLower.size());
        std::copy((std::byte*)identifierToLower.c_str(), (std::byte*)identifierToLower.c_str() + identifierToLower.size(), identifierBytes.begin());

        uint32_t fnvPrime = 0x01000193;
        blangString.Hash = 0x811C9DC5;

        for (int32_t i = 0; i < identifierBytes.size(); i++) {
            blangString.Hash ^= (int32_t)identifierBytes[i];
            blangString.Hash *= fnvPrime;
        }

        // Write the hash (little endian)
        std::copy((std::byte*)&blangString.Hash, (std::byte*)&blangString.Hash + 4, hashBytes.begin());
        std::reverse(hashBytes.begin(), hashBytes.end());
        std::copy(hashBytes.begin(), hashBytes.end(), (std::byte*)&blangString.Hash);
        blangBytes.insert(blangBytes.end(), hashBytes.begin(), hashBytes.end());

        // Write identifier
        int32_t identifierBytesLength = blangString.Identifier.size();
        blangBytes.insert(blangBytes.end(), (std::byte*)&identifierBytesLength, (std::byte*)&identifierBytesLength + 4);
        blangBytes.insert(blangBytes.end(), (std::byte*)blangString.Identifier.c_str(), (std::byte*)blangString.Identifier.c_str() + blangString.Identifier.size());

        // Remove carriage returns
        blangString.Text.erase(std::remove(blangString.Text.begin(), blangString.Text.end(), '\r'), blangString.Text.end());

        // Write text
        int32_t textBytesLength = blangString.Text.size();
        blangBytes.insert(blangBytes.end(), (std::byte*)&textBytesLength, (std::byte*)&textBytesLength + 4);
        blangBytes.insert(blangBytes.end(), (std::byte*)blangString.Text.c_str(), (std::byte*)blangString.Text.c_str() + blangString.Text.size());

        // Write unknown data
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
