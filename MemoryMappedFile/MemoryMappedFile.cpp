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
#include <filesystem>

#include "MemoryMappedFile/MemoryMappedFile.hpp"

/**
 * @brief Construct a new MemoryMappedFile object
 * 
 * @param filePath Path to the file to map in memory
 */
MemoryMappedFile::MemoryMappedFile(std::string filePath)
{
    FilePath = filePath;
    Size = std::filesystem::file_size(FilePath);

    if (Size <= 0)
        throw std::exception();

#ifdef _WIN32
    FileHandle = CreateFileA(FilePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (GetLastError() != ERROR_SUCCESS || FileHandle == INVALID_HANDLE_VALUE)
        throw std::exception();

    FileMapping = CreateFileMappingA(FileHandle, NULL, PAGE_READWRITE, *((DWORD*)&Size + 1), *(DWORD*)&Size, NULL);

    if (GetLastError() != ERROR_SUCCESS || FileMapping == NULL) {
        CloseHandle(FileHandle);
        throw std::exception();
    }

    Mem = (std::byte*)MapViewOfFile(FileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (GetLastError() != ERROR_SUCCESS || Mem == NULL) {
        CloseHandle(FileHandle);
        CloseHandle(FileMapping);
        throw std::exception();
    }
#else
    FileDescriptor = open(FilePath.c_str(), O_RDWR);

    if (FileDescriptor == -1)
        throw std::exception();

    Mem = (std::byte*)mmap(0, Size, PROT_READ | PROT_WRITE, MAP_SHARED, FileDescriptor, 0);

    if (Mem == NULL) {
        close(FileDescriptor);
        throw std::exception();
    }

    madvise(Mem, Size, MADV_WILLNEED);
#endif
}

/**
 * @brief Destroy the MemoryMappedFile object
 * 
 */
MemoryMappedFile::~MemoryMappedFile()
{
    UnmapFile();
}

/**
 * @brief Unmap the memory mapped file
 * 
 */
void MemoryMappedFile::UnmapFile()
{
#ifdef _WIN32
    UnmapViewOfFile(Mem);
    CloseHandle(FileMapping);
    CloseHandle(FileHandle);
#else
    munmap(Mem, std::filesystem::file_size(FilePath));
    close(FileDescriptor);
#endif

    Size = 0;
    Mem = NULL;
}

/**
 * @brief Resize the memory mapped file
 * 
 * @param newSize New size for file
 * @return True on success, false otherwise
 */
bool MemoryMappedFile::ResizeFile(uint64_t newSize)
{
    try {
#ifdef _WIN32
        UnmapViewOfFile(Mem);
        CloseHandle(FileMapping);

        FileMapping = CreateFileMappingA(FileHandle, NULL, PAGE_READWRITE, *((DWORD*)&newSize + 1), *(DWORD*)&newSize, NULL);

        if (GetLastError() != ERROR_SUCCESS || FileMapping == NULL)
            return false;

        Mem = (std::byte*)MapViewOfFile(FileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        if (GetLastError() != ERROR_SUCCESS || Mem == NULL)
            return false;
#else
        munmap(Mem, Size);
        std::filesystem::resize_file(FilePath, newSize);
        Mem = (std::byte*)mmap(0, newSize, PROT_READ | PROT_WRITE, MAP_SHARED, FileDescriptor, 0);

        if (Mem == NULL)
            return false;

        madvise(Mem, newSize, MADV_WILLNEED);
#endif
    }
    catch (...) {
        return false;
    }

    Size = newSize;

    return true;
}