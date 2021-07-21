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
#include <algorithm>
#include <string>
#include <filesystem>
#include <cstring>
#include <ctime>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "EternalModLoader.hpp"


/**
 * @brief Hash data using HMAC SHA256 algorithm
 * 
 * @param data1 First data bytes to hash
 * @param data1Len Size of first data bytes
 * @param data1 Second data bytes to hash
 * @param data1Len Size of second data bytes
 * @param data1 Third data bytes to hash
 * @param data1Len Size of third data bytes
 * @param hmacKey HMAC key used to hash, regular SHA256 will be used instead if it's nullptr
 * @param hmacKeyLen Size of HMAC key
 * @return Data hash
 */
std::byte *HashData(const std::byte *data1, const size_t data1Len, const std::byte *data2, const size_t data2Len,
    const std::byte *data3, const size_t data3Len, const std::byte *hmacKey, size_t hmacKeyLen)
{
    if (hmacKey == nullptr) {
        SHA256_CTX sha256;
        SHA256_Init(&sha256);

        std::byte *md = new std::byte[SHA256_DIGEST_LENGTH];

        SHA256_Update(&sha256, (uint8_t*)data1, data1Len);
        SHA256_Update(&sha256, (uint8_t*)data2, data2Len);
        SHA256_Update(&sha256, (uint8_t*)data3, data3Len);
        SHA256_Final((uint8_t*)md, &sha256);

        return md;
    }
    else {
        uint32_t md_len;

        HMAC_CTX *ctx = HMAC_CTX_new();
        HMAC_Init_ex(ctx, hmacKey, hmacKeyLen, EVP_sha256(), nullptr);

        std::byte *md = new std::byte[HMAC_size(ctx)];

        HMAC_Update(ctx, (uint8_t*)data1, data1Len);
        HMAC_Update(ctx, (uint8_t*)data2, data2Len);
        HMAC_Update(ctx, (uint8_t*)data3, data3Len);
        HMAC_Final(ctx, (uint8_t*)md, &md_len);

        HMAC_CTX_free(ctx);

        return md;
    }
}

/**
 * @brief Encrypt data using AES 128 CBC algorithm
 * 
 * @param plaintext Bytes to encrypt
 * @param plaintext_len Size of plaintext
 * @param key Key to use for encryption
 * @param iv IV to use for encryption
 * @param ciphertext Buffer to store the ciphertext in
 * @return Length of the ciphertext
 */
int32_t EncryptData(const unsigned char *plaintext, const size_t plaintextLen, const unsigned char *key, const unsigned char *iv, unsigned char *ciphertext)
{
    int32_t len;
    int32_t ciphertextLen;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintextLen);

    ciphertextLen = len;

    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);

    ciphertextLen += len;

    EVP_CIPHER_CTX_free(ctx);

    return ciphertextLen;
}

/**
 * @brief Decrypt data using AES 128 CBC algorithm
 * 
 * @param ciphertext Bytes to decrypt
 * @param ciphertext_len Size of ciphertext
 * @param key Key to use for decryption
 * @param iv IV to use for decryption
 * @param plaintext Buffer to store the plaintext in
 * @return Length of the plaintext
 */
int32_t DecryptData(const unsigned char *ciphertext, const size_t ciphertextLen, const unsigned char *key, const unsigned char *iv, unsigned char *plaintext)
{
    int32_t len;
    int32_t plaintextLen;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertextLen);

    plaintextLen = len;

    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    plaintextLen += len;

    EVP_CIPHER_CTX_free(ctx);

    return plaintextLen;
}

/**
 * @brief Encrypt or decrypt data using the previously defined functions
 * 
 * @param decrypt Bool indicating wether to decrypt or encrypt
 * @param inputData Data to encrypt/decrypt
 * @param inputDataLen Size of input data
 * @param key Key to use for encryption/decryption
 * @param iv IV to use for encryption/decryption
 * @return Vector containing the encrypted/decrypted data bytes
 */
std::vector<std::byte> CryptData(const bool decrypt, const std::byte *inputData, const size_t inputDataLen, const std::byte *key, const std::byte *iv)
{
    unsigned char *output = new unsigned char[inputDataLen + 16 - (inputDataLen % 16)];
    uint64_t newSize;

    if (decrypt) {
        newSize = DecryptData((unsigned char*)inputData, inputDataLen, (unsigned char*)key, (unsigned char*)iv, output);
    }
    else {
        newSize = EncryptData((unsigned char*)inputData, inputDataLen, (unsigned char*)key, (unsigned char*)iv, output);
    }

    std::vector<std::byte> outputVector((std::byte*)output, (std::byte*)output + newSize);

    delete[] output;

    return outputVector;
}

/**
 * @brief Encrypt/decrypt a blang file
 * 
 * @param fileData Vector containing the blang's bytes
 * @param internalPath Blang's path as used by the game
 * @param decrypt Bool indicating whether to decrypt or encrypt
 * @return std::vector<std::byte> 
 */
std::vector<std::byte> IdCrypt(const std::vector<std::byte> &fileData, const std::string internalPath, const bool decrypt)
{
    std::string keyDeriveStatic = "swapTeam\n";
    srand(time(nullptr));

    std::byte fileSalt[0xC];

    if (decrypt) {
        std::copy(fileData.begin(), fileData.begin() + 0xC, fileSalt);
    }
    else {
        std::generate((char*)fileSalt, (char*)fileSalt + 0xC, rand);
    }

    std::byte *encKey;

    try {
        encKey = HashData(fileSalt, 0xC, (std::byte*)keyDeriveStatic.c_str(), 0xA, (std::byte*)internalPath.c_str(), internalPath.size(), nullptr, 0);
    }
    catch (...) {
        return std::vector<std::byte>();
    }

    std::byte fileIV[0x10];

    if (decrypt) {
        std::copy(fileData.begin() + 0xC, fileData.begin() + 0xC + 0x10, fileIV);
    }
    else {
        std::generate((char*)fileIV, (char*)fileIV + 0x10, rand);
    }

    std::vector<std::byte> fileText;
    std::byte *hmac;

    if (decrypt) {
        fileText.resize(fileData.size() - 0x1C - 0x20);
        std::copy(fileData.begin() + 0x1C, fileData.end() - 0x20, fileText.begin());

        std::byte fileHmac[0x20];
        std::copy(fileData.end() - 0x20, fileData.end(), fileHmac);

        try {
            hmac = HashData(fileSalt, 0xC, fileIV, 0x10, fileText.data(), fileText.size(), encKey, 0x20);
        }
        catch (...) {
            return std::vector<std::byte>();
        }

        if (std::memcmp(hmac, fileHmac, 0x20))
            return std::vector<std::byte>();
    }
    else {
        fileText.resize(fileData.size());
        std::copy(fileData.begin(), fileData.end(), fileText.begin());
    }

    std::vector<std::byte> cryptedText;

    try {
        std::byte realKey[0x10];
        std::copy(encKey, encKey + 0x10, realKey);

        cryptedText = CryptData(decrypt, fileText.data(), fileText.size(), realKey, fileIV);
    }
    catch (...) {
        return std::vector<std::byte>();
    }

    if (decrypt) {
        delete[] encKey;
        delete[] hmac;

        return cryptedText;
    }
    else {
        std::vector<std::byte> fullData;

        fullData.reserve(cryptedText.size() + 0xC + 0x10 + 0x20);
        fullData.insert(fullData.end(), fileSalt, fileSalt + 0xC);
        fullData.insert(fullData.end(), fileIV, fileIV + 0x10);
        fullData.insert(fullData.end(), cryptedText.begin(), cryptedText.end());

        hmac = HashData(fileSalt, 0xC, fileIV, 0x10, cryptedText.data(), cryptedText.size(), encKey, 0x20);
        fullData.insert(fullData.end(), hmac, hmac + 0x20);

        delete[] encKey;
        delete[] hmac;

        return fullData;
    }
}