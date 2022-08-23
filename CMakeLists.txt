# Project metadata
cmake_minimum_required(VERSION 3.10)
project(EternalModLoader)
set(CMAKE_CXX_STANDARD 17)

# Project sources
file(GLOB SOURCES
        ./AssetsInfo/AssetsInfo.cpp
        ./AssetsInfo/AssetsInfo.hpp
        ./BlangFile/BlangFile.cpp
        ./BlangFile/BlangFile.hpp
        ./MapResourcesFile/MapResourcesFile.cpp
        ./MapResourcesFile/MapResourcesFile.hpp
        ./MemoryMappedFile/MemoryMappedFile.cpp
        ./MemoryMappedFile/MemoryMappedFile.hpp
        ./Mod/Mod.cpp
        ./Mod/Mod.hpp
        ./OnlineSafety/OnlineSafety.cpp
        ./OnlineSafety/OnlineSafetySWF.hpp
        ./Oodle/Oodle.cpp
        ./Oodle/Oodle.hpp
        ./PackageMapSpec/PackageMapSpec.cpp
        ./PackageMapSpec/PackageMapSpec.hpp
        ./PackageMapSpec/PackageMapSpecInfo.cpp
        ./PackageMapSpec/PackageMapSpecInfo.hpp
        ./ResourceData/ResourceData.cpp
        ./ResourceData/ResourceData.hpp
        ./Utils/Utils.cpp
        ./Utils/Utils.hpp
        ./EternalModLoader.cpp
        ./EternalModLoader.hpp
        ./AddChunks.cpp
        ./BlangDecrypt.cpp
        ./GetObject.cpp
        ./LoadModFiles.cpp
        ./LoadMods.cpp
        ./PathToRes.cpp
        ./ReadChunkInfo.cpp
        ./ReadResourceFile.cpp
        ./ReadSoundEntries.cpp
        ./ReplaceChunks.cpp
        ./ReplaceSounds.cpp
        ./SetModDataForChunk.cpp
        ./WriteStreamDB.cpp
        ./miniz/miniz.c
        ./miniz/miniz.h
        ./jsonxx/jsonxx.cc
        ./jsonxx/jsonxx.h
        )

# Compiler arguments
if(MSVC)
        if(DEFINED ENV{ETERNALMODLOADERCPP_STATIC})
                set(OPENSSL_USE_STATIC_LIBS TRUE)
        endif()
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
        set(CMAKE_CXX_FLAGS "/O2 /Oi /Ot /GL /EHsc /I \"${CMAKE_CURRENT_SOURCE_DIR}\"")
else()
        if(DEFINED ENV{ETERNALMODLOADERCPP_STATIC})
                set(OPENSSL_USE_STATIC_LIBS TRUE)
                set(CMAKE_CXX_FLAGS "-pthread -Ofast -DNDEBUG -ftree-vectorize -msse2 -static -I${CMAKE_CURRENT_SOURCE_DIR}")
        else()
                set(CMAKE_CXX_FLAGS "-pthread -Ofast -DNDEBUG -ftree-vectorize -msse2 -I${CMAKE_CURRENT_SOURCE_DIR}")
        endif()
endif(MSVC)

# Add executable to build
add_executable(DEternal_loadMods ${SOURCES})

# Link to OpenSSL
if(MSVC)
        set(OPENSSL_USE_STATIC_LIBS TRUE)
endif(MSVC)

find_package(OpenSSL REQUIRED)

if(MSVC)
        target_link_options(DEternal_loadMods PUBLIC "/LTCG")
endif(MSVC)

target_link_libraries(
        DEternal_loadMods
        PUBLIC
        OpenSSL::Crypto
        ${CMAKE_DL_LIBS}
)
