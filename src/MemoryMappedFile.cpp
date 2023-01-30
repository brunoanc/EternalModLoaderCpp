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

#include <filesystem>
#include "MemoryMappedFile.hpp"

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

namespace fs = std::filesystem;

MemoryMappedFile::MemoryMappedFile(const std::string filePath)
{
    // Get filepath and size
    FilePath = filePath;
    Size = fs::file_size(FilePath);

    if (Size <= 0) {
        throw std::exception();
    }

#ifdef _WIN32
    // Get file handle
    FileHandle = CreateFileA(FilePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (GetLastError() != ERROR_SUCCESS || FileHandle == INVALID_HANDLE_VALUE) {
        throw std::exception();
    }

    // Map file into memory
    FileMapping = CreateFileMappingA(FileHandle, nullptr, PAGE_READWRITE,
        *(reinterpret_cast<DWORD*>(&Size) + 1), *reinterpret_cast<DWORD*>(&Size), nullptr);

    if (GetLastError() != ERROR_SUCCESS || FileMapping == nullptr) {
        CloseHandle(FileHandle);
        throw std::exception();
    }

    // Get memory view of file
    Mem = reinterpret_cast<std::byte*>(MapViewOfFile(FileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0));

    if (GetLastError() != ERROR_SUCCESS || Mem == nullptr) {
        CloseHandle(FileHandle);
        CloseHandle(FileMapping);
        throw std::exception();
    }
#else
    // Get file descriptor
    FileDescriptor = open(FilePath.c_str(), O_RDWR);

    if (FileDescriptor == -1) {
        throw std::exception();
    }

    // Map file into memory
    Mem = reinterpret_cast<std::byte*>(mmap(0, Size, PROT_READ | PROT_WRITE, MAP_SHARED, FileDescriptor, 0));

    if (Mem == nullptr) {
        close(FileDescriptor);
        throw std::exception();
    }

    // Prepare memory for usage
    madvise(Mem, Size, MADV_WILLNEED);
#endif
}

MemoryMappedFile::~MemoryMappedFile()
{
    // Unmap the file
    UnmapFile();
}

void MemoryMappedFile::UnmapFile()
{
#ifdef _WIN32
    // Unmap memory view and close handles
    UnmapViewOfFile(Mem);
    CloseHandle(FileMapping);
    CloseHandle(FileHandle);
#else
    // Unmap file and close handle
    munmap(Mem, fs::file_size(FilePath));
    close(FileDescriptor);
#endif

    // Reset object data
    FilePath = "";
    Size = 0;
    Mem = nullptr;
}

bool MemoryMappedFile::ResizeFile(const size_t newSize)
{
    try {
#ifdef _WIN32
        // Unmap memory view and close mapping handle
        UnmapViewOfFile(Mem);
        CloseHandle(FileMapping);

        // Create new file mapping with bigger size
        FileMapping = CreateFileMappingA(FileHandle, nullptr, PAGE_READWRITE, *(reinterpret_cast<const DWORD*>(&newSize) + 1), *reinterpret_cast<const DWORD*>(&newSize), nullptr);

        if (GetLastError() != ERROR_SUCCESS || FileMapping == nullptr) {
            return false;
        }

        // Get new memory view of file
        Mem = reinterpret_cast<std::byte*>(MapViewOfFile(FileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0));

        if (GetLastError() != ERROR_SUCCESS || Mem == nullptr) {
            return false;
        }
#else
        // Unmap file
        munmap(Mem, Size);

        // Resize file
        fallocate(FileDescriptor, 0, 0, newSize);

        // Remap file
        Mem = reinterpret_cast<std::byte*>(mmap(0, newSize, PROT_READ | PROT_WRITE, MAP_SHARED, FileDescriptor, 0));

        if (Mem == nullptr) {
            return false;
        }

        // Prepare memory for usage
        madvise(Mem, newSize, MADV_WILLNEED);
#endif
    }
    catch (...) {
        return false;
    }

    // Set size attribute to new size
    Size = newSize;

    return true;
}
