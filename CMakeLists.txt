cmake_minimum_required(VERSION 3.10)
project(EternalModLoader)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES
        ./EternalModLoader.cpp
        ./AddChunks.cpp
        ./AssetsInfo.cpp
        ./BlangDecrypt.cpp
        ./BlangParser.cpp
        ./LoadModFiles.cpp
        ./LoadMods.cpp
        ./MapResourcesFile.cpp
        ./Mod.cpp
        ./ModifyPackageMapSpec.cpp
        ./Oodle.cpp
        ./PackageMapSpec.cpp
        ./PathtoRes.cpp
        ./ReadChunkInfo.cpp
        ./ReadResourceFile.cpp
        ./ReadSoundEntries.cpp
        ./ReplaceChunks.cpp
        ./ResourceData.cpp
        ./ReplaceSounds.cpp
        ./SetBufferSize.cpp
        ./SetModDataForChunk.cpp
        ./Utils.cpp
        ./jsonxx/jsonxx.cc
        ./miniz/miniz.c
        )

if(MSVC)
        if(DEFINED ENV{ETERNALMODLOADERCPP_STATIC})
                set(OPENSSL_USE_STATIC_LIBS TRUE)
        endif()
        set (CMAKE_CXX_FLAGS "/O2 /Oi /Ot /GL /EHsc")
else()
        if(DEFINED ENV{ETERNALMODLOADERCPP_STATIC})
                set(OPENSSL_USE_STATIC_LIBS TRUE)
                set (CMAKE_CXX_FLAGS "-pthread -Ofast -DNDEBUG -ftree-vectorize -msse2 -static")
        else()
                set (CMAKE_CXX_FLAGS "-pthread -Ofast -DNDEBUG -ftree-vectorize -msse2")
        endif()
endif(MSVC)

add_executable(DEternal_loadMods ${SOURCES})

find_package(OpenSSL REQUIRED)

target_link_libraries(
        DEternal_loadMods
        PUBLIC
        OpenSSL::Crypto
        ${CMAKE_DL_LIBS}
)
