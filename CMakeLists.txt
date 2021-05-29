cmake_minimum_required(VERSION 3.12)
project(EternalModLoader)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(zipper)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

file(GLOB SOURCES
        ./EternalModLoader.hpp
        ./EternalModLoader.cpp
        ./AddChunks.cpp
        ./AssetsInfo.cpp
        ./BlangDecrypt.cpp
        ./BlangParser.cpp
        ./MapResourcesFile.cpp
        ./Oodle.cpp
        ./PackageMapSpec.cpp
        ./PathtoRes.cpp
        ./ReadChunkInfo.cpp
        ./ReadResourceFile.cpp
        ./ReplaceChunks.cpp
        ./ResourceData.cpp
        ./SoundMods.cpp
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
        ${CMAKE_DL_LIBS}
)
