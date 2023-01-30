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
#include <cstring>
#include <ctime>
#include <memory>
#include <openssl/evp.h>
#include "BlangDecrypt.hpp"

#define SHA256_DIGEST_LENGTH 32

std::unique_ptr<std::byte[]> HashData(const std::byte *data1, const size_t data1Len, const std::byte *data2, const size_t data2Len,
    const std::byte *data3, const size_t data3Len, const std::byte *hmacKey, size_t hmacKeyLen)
{
    // Check if hmacKey was passed
    if (hmacKey == nullptr) {
        // No HMAC key, use regular SHA256
        unsigned int sha256_length;
        auto md = std::make_unique<std::byte[]>(SHA256_DIGEST_LENGTH);

        EVP_MD_CTX *ctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);

        EVP_DigestUpdate(ctx, data1, data1Len);
        EVP_DigestUpdate(ctx, data2, data2Len);
        EVP_DigestUpdate(ctx, data3, data3Len);

        EVP_DigestFinal_ex(ctx, reinterpret_cast<unsigned char*>(md.get()), &sha256_length);
        EVP_MD_CTX_free(ctx);

        return md;
    }
    else {
        // Use SHA256 HMAC
        EVP_MAC *mac = EVP_MAC_fetch(nullptr, "HMAC", "provider=default");
        EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);
        char digest[] = "SHA256";

        const OSSL_PARAM param[] { OSSL_PARAM_utf8_string("digest", reinterpret_cast<void*>(digest), 7), OSSL_PARAM_END };
        EVP_MAC_init(ctx, reinterpret_cast<const unsigned char*>(hmacKey), hmacKeyLen, param);

        size_t hmacSize = EVP_MAC_CTX_get_mac_size(ctx);
        auto md = std::make_unique<std::byte[]>(hmacSize);

        EVP_MAC_update(ctx, reinterpret_cast<const unsigned char*>(data1), data1Len);
        EVP_MAC_update(ctx, reinterpret_cast<const unsigned char*>(data2), data2Len);
        EVP_MAC_update(ctx, reinterpret_cast<const unsigned char*>(data3), data3Len);

        size_t written;
        EVP_MAC_final(ctx, reinterpret_cast<unsigned char*>(md.get()), &written, hmacSize);

        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return md;
    }
}

size_t EncryptData(const unsigned char *plaintext, const size_t plaintextLen, const unsigned char *key, const unsigned char *iv, unsigned char *ciphertext)
{
    int len;
    size_t ciphertextLen;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintextLen);

    ciphertextLen = len;

    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);

    ciphertextLen += len;

    EVP_CIPHER_CTX_free(ctx);

    return ciphertextLen;
}

size_t DecryptData(const unsigned char *ciphertext, const size_t ciphertextLen, const unsigned char *key, const unsigned char *iv, unsigned char *plaintext)
{
    int len;
    size_t plaintextLen;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertextLen);

    plaintextLen = len;

    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    plaintextLen += len;

    EVP_CIPHER_CTX_free(ctx);

    return plaintextLen;
}

std::vector<std::byte> CryptData(const bool decrypt, const std::byte *inputData, const size_t inputDataLen, const std::byte *key, const std::byte *iv)
{
    // Create output vector
    std::vector<std::byte> output(inputDataLen + 16 - (inputDataLen % 16));
    size_t newSize;

    if (decrypt) {
        // Decrypt the data
        newSize = DecryptData(reinterpret_cast<const unsigned char*>(inputData), inputDataLen,
            reinterpret_cast<const unsigned char*>(key), reinterpret_cast<const unsigned char*>(iv),
            reinterpret_cast<unsigned char*>(output.data()));
    }
    else {
        // Encrypt the data
        newSize = EncryptData(reinterpret_cast<const unsigned char*>(inputData), inputDataLen,
            reinterpret_cast<const unsigned char*>(key), reinterpret_cast<const unsigned char*>(iv),
            reinterpret_cast<unsigned char*>(output.data()));
    }

    output.resize(newSize);
    return output;
}

std::vector<std::byte> IdCrypt(const std::vector<std::byte>& fileData, const std::string internalPath, const bool decrypt)
{
    std::string keyDeriveStatic = "swapTeam\n";

    // Init random generator
    srand(time(nullptr));

    // Get salt from file, or create a new one
    std::byte fileSalt[0xC];

    if (decrypt) {
        std::copy(fileData.begin(), fileData.begin() + 0xC, fileSalt);
    }
    else {
        std::generate(reinterpret_cast<unsigned char*>(fileSalt), reinterpret_cast<unsigned char*>(fileSalt) + 0xC, rand);
    }

    // Generate the encryption key for AES using SHA256
    std::unique_ptr<std::byte[]> encKey;

    try {
        encKey = HashData(fileSalt, 0xC,
            reinterpret_cast<const std::byte*>(keyDeriveStatic.c_str()), 0xA,
            reinterpret_cast<const std::byte*>(internalPath.c_str()), internalPath.size(), nullptr, 0);
    }
    catch (...) {
        return std::vector<std::byte>();
    }

    // Get IV for AES from the file, or create a new one
    std::byte fileIV[0x10];

    if (decrypt) {
        std::copy(fileData.begin() + 0xC, fileData.begin() + 0xC + 0x10, fileIV);
    }
    else {
        std::generate(reinterpret_cast<unsigned char*>(fileIV), reinterpret_cast<unsigned char*>(fileIV) + 0x10, rand);
    }

    // Get plaintext for AES
    std::vector<std::byte> fileText;
    std::unique_ptr<std::byte[]> hmac;

    if (decrypt) {
        fileText.resize(fileData.size() - 0x1C - 0x20);
        std::copy(fileData.begin() + 0x1C, fileData.end() - 0x20, fileText.begin());

        std::byte fileHmac[0x20];
        std::copy(fileData.end() - 0x20, fileData.end(), fileHmac);

        // Get HMAC from file data
        try {
            hmac = HashData(fileSalt, 0xC, fileIV, 0x10, fileText.data(), fileText.size(), encKey.get(), 0x20);
        }
        catch (...) {
            return std::vector<std::byte>();
        }

        // Make sure the file HMAC and the new HMAC are the same
        if (std::memcmp(hmac.get(), fileHmac, 0x20) != 0) {
            return std::vector<std::byte>();
        }
    }
    else {
        fileText.resize(fileData.size());
        std::copy(fileData.begin(), fileData.end(), fileText.begin());
    }

    // Encrypt or decrypt data using AES
    std::vector<std::byte> cryptedText;

    try {
        std::byte realKey[0x10];
        std::copy(encKey.get(), encKey.get() + 0x10, realKey);

        cryptedText = CryptData(decrypt, fileText.data(), fileText.size(), realKey, fileIV);
    }
    catch (...) {
        return std::vector<std::byte>();
    }

    // Create and return the new file
    if (decrypt) {
        return cryptedText;
    }
    else {
        std::vector<std::byte> fullData;

        fullData.reserve(cryptedText.size() + 0xC + 0x10 + 0x20);
        fullData.insert(fullData.end(), fileSalt, fileSalt + 0xC);
        fullData.insert(fullData.end(), fileIV, fileIV + 0x10);
        fullData.insert(fullData.end(), cryptedText.begin(), cryptedText.end());

        hmac = HashData(fileSalt, 0xC, fileIV, 0x10, cryptedText.data(), cryptedText.size(), encKey.get(), 0x20);
        fullData.insert(fullData.end(), hmac.get(), hmac.get() + 0x20);

        return fullData;
    }
}
