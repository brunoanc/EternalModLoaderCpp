#include <iostream>

#include "EternalModLoader.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/stat.h>
#endif

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

void SetOptimalBufferSize(std::string driveRootPath)
{
    BufferSize = GetClusterSize(driveRootPath);

    if (BufferSize == -1)
        BufferSize = 4096;

    Buffer = new std::byte[BufferSize];
}