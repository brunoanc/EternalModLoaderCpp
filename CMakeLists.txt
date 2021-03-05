cmake_minimum_required(VERSION 3.19)
project(EternalModLoader)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(zipper)

file(GLOB SOURCES
        ./EternalModLoader.h
        ./EternalModLoader.cpp
        ./AddChunks.cpp
        ./PathtoRes.cpp
        ./ReadChunkInfo.cpp
        ./ReadResourceFile.cpp
        ./ReplaceChunks.cpp
        ./Utils.cpp
        ./VectorUtils.cpp
        ./mmap_allocator/*
        )

set (CMAKE_CXX_FLAGS "-Ofast -DNDEBUG -ftree-vectorize -msse2")
add_executable(DEternal_loadMods ${SOURCES})

target_link_libraries(
        DEternal_loadMods
        PUBLIC
        staticZipper
)