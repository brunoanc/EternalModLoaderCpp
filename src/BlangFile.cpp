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

#include <algorithm>
#include "ProgramOptions.hpp"
#include "Utils.hpp"
#include "BlangFile.hpp"

BlangFile::BlangFile(const std::vector<std::byte>& blangBytes)
{
    size_t pos = 0;

    // Check where the blang file entries start
    std::string str(reinterpret_cast<const char*>(blangBytes.data()) + 12, 5);

    if (ToLower(str) != "#str_") {
        // Read unknown data (big endian)
        std::copy(blangBytes.begin(), blangBytes.begin() + 8, reinterpret_cast<std::byte*>(&UnknownData));
        std::reverse(reinterpret_cast<std::byte*>(&UnknownData), reinterpret_cast<std::byte*>(&UnknownData) + 8);
        pos += 8;
    }

    // Read the string amount (big endian)
    unsigned int stringAmount;
    std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, reinterpret_cast<std::byte*>(&stringAmount));
    std::reverse(reinterpret_cast<std::byte*>(&stringAmount), reinterpret_cast<std::byte*>(&stringAmount) + 4);
    pos += 4;

    // Parse each string
    std::vector<std::byte> identifierBytes;
    std::vector<std::byte> textBytes;
    std::vector<std::byte> unknown;

    Strings.reserve(stringAmount);

    for (unsigned int i = 0; i < stringAmount; i++) {
        // Read string hash
        unsigned int hash;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, reinterpret_cast<std::byte*>(&hash));
        pos += 4;

        // Read string identifier
        unsigned int identifierLength;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, reinterpret_cast<std::byte*>(&identifierLength));
        pos += 4;

        std::string identifier(reinterpret_cast<const char*>(blangBytes.data()) + pos,
            reinterpret_cast<const char*>(blangBytes.data()) + pos + identifierLength);
        pos += identifierLength;

        // Read string
        unsigned int textLength;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, reinterpret_cast<std::byte*>(&textLength));
        pos += 4;

        std::string text(reinterpret_cast<const char*>(blangBytes.data()) + pos, reinterpret_cast<const char*>(blangBytes.data()) + pos + textLength);
        pos += textLength;

        // Read unknown data
        unsigned int unknownLength;
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + 4, reinterpret_cast<std::byte*>(&unknownLength));
        pos += 4;

        unknown.resize(unknownLength);
        std::copy(blangBytes.begin() + pos, blangBytes.begin() + pos + unknownLength, unknown.begin());
        pos += unknownLength;

        BlangString blangString(hash, identifier, text, unknown);
        Strings.push_back(blangString);
    }
}

std::vector<std::byte> BlangFile::ToByteVector()
{
    std::vector<std::byte> blangBytes;

    // Delete invalid strings first
    // Strings must have a valid identifier
    for (auto i = static_cast<ssize_t>(Strings.size()) - 1; i >= 0; i--) {
        if (RemoveWhitespace(Strings[i].Identifier).empty()) {
            Strings.erase(Strings.begin() + i);
        }
    }

    // Write unknown data (big endian)
    blangBytes.insert(blangBytes.end(), reinterpret_cast<std::byte*>(&UnknownData), reinterpret_cast<std::byte*>(&UnknownData) + 8);
    std::reverse(blangBytes.end() - 8, blangBytes.end());

    // Write string amount (big endian)
    auto stringsAmount = static_cast<int>(Strings.size());
    blangBytes.insert(blangBytes.end(), reinterpret_cast<std::byte*>(&stringsAmount), reinterpret_cast<std::byte*>(&stringsAmount) + 4);
    std::reverse(blangBytes.end() - 4, blangBytes.end());

    std::vector<std::byte> identifierBytes;
    std::vector<std::byte> hashBytes(4);
    std::vector<std::byte> identifierBytesNew;
    std::vector<std::byte> textBytes;

    // Write each string
    for (auto& blangString : Strings) {
        // Calculate teh hash of the identifier string (FNV1A32)
        std::string identifierToLower = ToLower(blangString.Identifier);
        identifierBytes.resize(identifierToLower.size());
        std::copy(reinterpret_cast<const std::byte*>(identifierToLower.c_str()),
            reinterpret_cast<const std::byte*>(identifierToLower.c_str()) + identifierToLower.size(), identifierBytes.begin());

        unsigned int fnvPrime = 0x01000193;
        blangString.Hash = 0x811C9DC5;

        for (size_t i = 0; i < identifierBytes.size(); i++) {
            blangString.Hash ^= static_cast<int>(identifierBytes[i]);
            blangString.Hash *= fnvPrime;
        }

        // Write the hash (little endian)
        std::copy(reinterpret_cast<std::byte*>(&blangString.Hash), reinterpret_cast<std::byte*>(&blangString.Hash) + 4, hashBytes.begin());
        std::reverse(hashBytes.begin(), hashBytes.end());
        std::copy(hashBytes.begin(), hashBytes.end(), reinterpret_cast<std::byte*>(&blangString.Hash));
        blangBytes.insert(blangBytes.end(), hashBytes.begin(), hashBytes.end());

        // Write identifier
        auto identifierBytesLength = static_cast<unsigned int>(blangString.Identifier.size());
        blangBytes.insert(blangBytes.end(), reinterpret_cast<std::byte*>(&identifierBytesLength), reinterpret_cast<std::byte*>(&identifierBytesLength) + 4);
        blangBytes.insert(blangBytes.end(), reinterpret_cast<const std::byte*>(blangString.Identifier.c_str()),
            reinterpret_cast<const std::byte*>(blangString.Identifier.c_str()) + blangString.Identifier.size());

        // Remove carriage returns
        blangString.Text.erase(std::remove(blangString.Text.begin(), blangString.Text.end(), '\r'), blangString.Text.end());

        // Write text
        auto textBytesLength = static_cast<unsigned int>(blangString.Text.size());
        blangBytes.insert(blangBytes.end(), reinterpret_cast<std::byte*>(&textBytesLength), reinterpret_cast<std::byte*>(&textBytesLength) + 4);
        blangBytes.insert(blangBytes.end(), reinterpret_cast<const std::byte*>(blangString.Text.c_str()),
            reinterpret_cast<const std::byte*>(blangString.Text.c_str()) + blangString.Text.size());

        // Write unknown data
        if (blangString.Unknown.empty()) {
            std::byte emptyArray[4] = {};
            blangBytes.insert(blangBytes.end(), emptyArray, emptyArray + 4);
        }
        else {
            auto unknownLength = static_cast<unsigned int>(blangString.Unknown.size());
            blangBytes.insert(blangBytes.end(), reinterpret_cast<std::byte*>(&unknownLength), reinterpret_cast<std::byte*>(&unknownLength) + 4);
            blangBytes.insert(blangBytes.end(), blangString.Unknown.begin(), blangString.Unknown.end());
        }
    }

    return blangBytes;
}
