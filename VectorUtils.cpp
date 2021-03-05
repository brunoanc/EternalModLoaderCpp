#include <iostream>
#include <vector>
#include <cstring>

#include "EternalModLoader.hpp"

std::vector<std::byte> VectorIntegralAdd(std::vector<std::byte>& vect, int numberOfBytes, long numberToAdd) {
    long a;
    std::vector<std::byte> vect_buffer(8);
    std::copy(vect.begin(), vect.begin() + 8, vect_buffer.begin());
    memcpy(&a, vect_buffer.data(), numberOfBytes);
    a += numberToAdd;
    auto arrayOfBytes = (char *)&a;

    std::vector<std::byte> vectorResult;
    vectorResult.reserve(numberOfBytes);
    for (int i = 0; i < numberOfBytes; i++) {
        vectorResult.push_back(static_cast<const std::byte>(arrayOfBytes[i]));
    }
    return vectorResult;
}

long VectorToNumber(std::vector<std::byte>& vect, int numberOfBytes) {
    long a;
    memcpy(&a, vect.data(), numberOfBytes);
    return a;
}

std::vector<std::byte> LongToVector(long number, int numberOfBytes) {
    auto arrayOfBytes  = (char *)&number;
    std::vector<std::byte> vectorResult;
    vectorResult.reserve(numberOfBytes);
    for (int i = 0; i < numberOfBytes; i++) {
        vectorResult.push_back(static_cast<const std::byte>(arrayOfBytes[i]));
    }
    return vectorResult;
}
