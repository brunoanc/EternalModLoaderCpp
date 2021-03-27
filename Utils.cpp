#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>

#include "EternalModLoader.hpp"

int GetResourceInfo(std::string resourceName) {
    for (int i = 0; i < ResourceList.size(); i++) {
        if (ResourceList[i].Name == resourceName) {
            return i;
        }
    }
    return -1;
}

int GetChunk(std::string name, int resourceIndex) {
    for (int i = 0; i < ResourceList[resourceIndex].ChunkList.size(); i++) {
        if (ResourceList[resourceIndex].ChunkList[i].Name.find(name) != std::string::npos) {
            return i;
        }
    }
    return -1;
}

std::string RemoveWhitespace(std::string& stringWithWhitespace) {
    std::string stringWithoutWhitespace = stringWithWhitespace;
    stringWithoutWhitespace.erase( std::remove_if( stringWithoutWhitespace.begin(), stringWithoutWhitespace.end(), []( char ch ) { return std::isspace<char>( ch, std::locale::classic() ); } ), stringWithoutWhitespace.end() );
    return stringWithoutWhitespace;
}

std::string ToLower(std::string& str) {
    std::string lowercase = str;
    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), [](unsigned char c){ return std::tolower(c); });
    return lowercase;
}

std::vector<std::byte> VectorIntegralAdd(std::vector<std::byte>& vect, int numberOfBytes, long numberToAdd) {
    long a;
    std::copy(vect.begin(), vect.begin() + numberOfBytes, (std::byte*)&a);
    a += numberToAdd;
    auto arrayOfBytes = (char*)&a;

    std::vector<std::byte> vectorResult(numberOfBytes);
    for (int i = 0; i < numberOfBytes; i++) {
        vectorResult.push_back(static_cast<const std::byte>(arrayOfBytes[i]));
    }
    return vectorResult;
}

long VectorToNumber(std::vector<std::byte>& vect, int numberOfBytes) {
    long a;
    std::copy(vect.begin(), vect.end(), (std::byte*)&a);
    return a;
}

std::vector<std::byte> LongToVector(long number, int numberOfBytes) {
    auto arrayOfBytes  = (char*)&number;
    std::vector<std::byte> vectorResult;
    vectorResult.reserve(numberOfBytes);
    for (int i = 0; i < numberOfBytes; i++) {
        vectorResult.push_back(static_cast<const std::byte>(arrayOfBytes[i]));
    }
    return vectorResult;
}