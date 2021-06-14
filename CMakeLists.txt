cmake_minimum_required(VERSION 3.10)
project(EternalModLoader)

set(CMAKE_CXX_STANDARD 17)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

file(GLOB SOURCES
        ./EternalModLoader.cpp
        ./AddChunks.cpp
        ./AssetsInfo.cpp
        ./BlangDecrypt.cpp
        ./BlangParser.cpp
        ./MapResourcesFile.cpp
        ./Mod.cpp
        ./Oodle.cpp
        ./PackageMapSpec.cpp
        ./PathtoRes.cpp
        ./ReadChunkInfo.cpp
        ./ReadResourceFile.cpp
        ./ReplaceChunks.cpp
        ./ResourceData.cpp
        ./SoundMods.cpp
        ./Utils.cpp
        ./miniz/miniz.c
        )

if(DEFINED ENV{ETERNALMODLOADERCPP_STATIC})
        set(OPENSSL_USE_STATIC_LIBS TRUE)
        set (CMAKE_CXX_FLAGS "-Ofast -DNDEBUG -ftree-vectorize -msse2 -static")
else()
        set (CMAKE_CXX_FLAGS "-Ofast -DNDEBUG -ftree-vectorize -msse2")
endif()

add_executable(DEternal_loadMods ${SOURCES})

find_package(OpenSSL REQUIRED)

target_link_libraries(
        DEternal_loadMods
        PUBLIC
        nlohmann_json::nlohmann_json
        OpenSSL::Crypto
        ${CMAKE_DL_LIBS}
)
