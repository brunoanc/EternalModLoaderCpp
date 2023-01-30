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

#ifndef MEMORYMAPPEDFILE_HPP
#define MEMORYMAPPEDFILE_HPP

#include <string>

class MemoryMappedFile
{
public:
    std::string FilePath;
    std::byte *Mem;
    size_t Size{0};

    MemoryMappedFile(const std::string filePath);
    ~MemoryMappedFile();

    void UnmapFile();
    bool ResizeFile(const size_t newSize);
private:
#ifdef _WIN32
    HANDLE FileHandle;
    HANDLE FileMapping;
#else
    int FileDescriptor;
#endif
};

#endif
