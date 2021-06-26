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

#ifndef _WIN32
#include <sys/stat.h>
#endif

#include "EternalModLoader.hpp"

/**
 * @brief Get the disk's cluster size
 * 
 * @param driveRootPath Path to the drive root
 * @return Disk's cluster size, or -1 on error
 */
int32_t GetClusterSize(std::string driveRootPath)
{
#ifdef _WIN32
    DWORD sectorsPerCluster;
    DWORD bytesPerSector;
    DWORD numberOfFreeClusters;
    DWORD totalNumberOfClusters;
    bool result = GetDiskFreeSpaceA(driveRootPath.c_str(), &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters);

    return result ? sectorsPerCluster * bytesPerSector : -1;
#else
    struct stat diskInfo;
    return stat(driveRootPath.c_str(), &diskInfo) == 0 ? diskInfo.st_blksize : -1;
#endif
}

/**
 * @brief Set the optimal size for the file buffer
 * 
 * @param driveRootPath Path to the drive root
 */
void SetOptimalBufferSize(std::string driveRootPath)
{
    BufferSize = GetClusterSize(driveRootPath);

    if (BufferSize == -1)
        BufferSize = 4096;

    Buffer = new std::byte[BufferSize];
}