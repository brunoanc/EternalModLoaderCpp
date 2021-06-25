#include <iostream>
#include <filesystem>

#include "EternalModLoader.hpp"

std::mutex mtx;

void LoadResourceMods(ResourceContainer &resourceContainer)
{
    int64_t fileSize = std::filesystem::file_size(resourceContainer.Path);

    if (fileSize == 0) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

    if (BufferSize == -1 || Buffer == NULL) {
        try {
            SetOptimalBufferSize(std::filesystem::absolute(resourceContainer.Path).root_path().string());
        }
        catch (...) {
            std::cerr << RED << "ERROR: " << RESET << "Error while determining the optimal buffer size, using 4096 as the default." << std::endl;

            if (Buffer != NULL)
                delete[] Buffer;

            Buffer = new std::byte[4096];
        }
    }

#ifdef _WIN32
    HANDLE hFile = CreateFileA(resourceContainer.Path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (GetLastError() != ERROR_SUCCESS || hFile == INVALID_HANDLE_VALUE) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

    HANDLE fileMapping = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, *((DWORD*)&fileSize + 1), *(DWORD*)&fileSize, NULL);

    if (GetLastError() != ERROR_SUCCESS || fileMapping == NULL) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

    std::byte *mem = (std::byte*)MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (GetLastError() != ERROR_SUCCESS || mem == NULL) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

    ReadResource(mem, resourceContainer);

    mtx.lock();
    std::stringstream &os = stringStreams[streamIndex++];
    mtx.unlock();

    if (SlowMode)
        ((std::ostream&)os).rdbuf(std::cout.rdbuf());

    ReplaceChunks(mem, hFile, fileMapping, resourceContainer, os);
    AddChunks(mem, hFile, fileMapping, resourceContainer, os);

    UnmapViewOfFile(mem);
    CloseHandle(fileMapping);
    CloseHandle(hFile);
#else
    int32_t fd = open(resourceContainer.Path.c_str(), O_RDWR);

    if (fd == -1) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

    std::byte *mem = (std::byte*)mmap(0, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (mem == NULL) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << resourceContainer.Path << RESET << " for writing!" << std::endl;
        close(fd);
        return;
    }

    madvise(mem, fileSize, MADV_WILLNEED);

    ReadResource(mem, resourceContainer);

    mtx.lock();
    std::stringstream &os = stringStreams[streamIndex++];
    mtx.unlock();

    if (SlowMode)
        ((std::ostream&)os).rdbuf(std::cout.rdbuf());

    ReplaceChunks(mem, fd, resourceContainer, os);
    AddChunks(mem, fd, resourceContainer, os);

    munmap(mem, std::filesystem::file_size(resourceContainer.Path));
    close(fd);
#endif
}

void LoadSoundMods(SoundContainer &soundContainer)
{
    int64_t fileSize = std::filesystem::file_size(soundContainer.Path);

    if (fileSize == 0) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

#ifdef _WIN32
    HANDLE hFile = CreateFileA(soundContainer.Path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,  FILE_ATTRIBUTE_NORMAL, NULL);

    if (GetLastError() != ERROR_SUCCESS || hFile == INVALID_HANDLE_VALUE) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

    HANDLE fileMapping = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, *((DWORD*)&fileSize + 1), *(DWORD*)&fileSize, NULL);

    if (GetLastError() != ERROR_SUCCESS || fileMapping == NULL) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

    std::byte *mem = (std::byte*)MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (GetLastError() != ERROR_SUCCESS || mem == NULL) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

    ReadSoundEntries(mem, soundContainer);

    mtx.lock();
    std::stringstream &os = stringStreams[streamIndex++];
    mtx.unlock();

    if (SlowMode)
        ((std::ostream&)os).rdbuf(std::cout.rdbuf());

    ReplaceSounds(mem, hFile, fileMapping, soundContainer, os);

    UnmapViewOfFile(mem);
    CloseHandle(fileMapping);
    CloseHandle(hFile);
#else
    int32_t fd = open(soundContainer.Path.c_str(), O_RDWR);

    if (fd == -1) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
        return;
    }

    std::byte *mem = (std::byte*)mmap(0, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (mem == NULL) {
        std::cerr << RED << "ERROR: " << RESET << "Failed to open " << YELLOW << soundContainer.Path << RESET << " for writing!" << std::endl;
        close(fd);
        return;
    }

    madvise(mem, fileSize, MADV_WILLNEED);

    ReadSoundEntries(mem, soundContainer);

    mtx.lock();
    std::stringstream &os = stringStreams[streamIndex++];
    mtx.unlock();

    if (SlowMode)
        ((std::ostream&)os).rdbuf(std::cout.rdbuf());

    ReplaceSounds(mem, fd, soundContainer, os);

    munmap(mem, std::filesystem::file_size(soundContainer.Path));
    close(fd);
#endif
}