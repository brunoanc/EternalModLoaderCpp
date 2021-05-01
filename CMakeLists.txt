cmake_minimum_required(VERSION 3.18)
project(EternalModLoader)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(zipper)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

file(GLOB SOURCES
        ./EternalModLoader.h
        ./EternalModLoader.cpp
        ./AddChunks.cpp
        ./BlangDecrypt.cpp
        ./BlangParser.cpp
        ./PathtoRes.cpp
        ./ReadChunkInfo.cpp
        ./ReadResourceFile.cpp
        ./ReplaceChunks.cpp
        ./Utils.cpp
        ./mmap_allocator/mmap*
        )

set (CMAKE_CXX_FLAGS "-Ofast -DNDEBUG -ftree-vectorize -msse2")
add_executable(DEternal_loadMods ${SOURCES})

find_package(OpenSSL REQUIRED)

target_link_libraries(
        DEternal_loadMods
        PUBLIC
        staticZipper
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
)
