# Project metadata
cmake_minimum_required(VERSION 3.10)
project(EternalModLoader)
set(CMAKE_CXX_STANDARD 17)

# Project sources
file(GLOB SOURCES
        ./src/*.cpp
        ./include/*.hpp
        ./vendor/jsonxx/jsonxx.cc
        ./vendor/jsonxx/jsonxx.h
        ./vendor/miniz/miniz.c
        ./vendor/miniz/miniz.h
        )

# Compiler arguments
if(MSVC)
        if(DEFINED ENV{ETERNALMODLOADERCPP_STATIC})
                set(OPENSSL_USE_STATIC_LIBS TRUE)
        endif()
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
        set(CMAKE_CXX_FLAGS "/O2 /Oi /Ot /GL /EHsc")
else()
        if(DEFINED ENV{ETERNALMODLOADERCPP_STATIC})
                set(OPENSSL_USE_STATIC_LIBS TRUE)
                set(CMAKE_CXX_FLAGS "-Wall -Werror -pthread -Ofast -static -s")
        else()
                set(CMAKE_CXX_FLAGS "-Wall -Werror -pthread -Ofast -s")
        endif()
endif(MSVC)

# Add executable to build
add_executable(DEternal_loadMods ${SOURCES})

# Add include directories
target_include_directories(DEternal_loadMods PRIVATE ./include ./vendor)

# Link to OpenSSL
find_package(OpenSSL "3.0" REQUIRED)

if(MSVC)
        target_link_options(DEternal_loadMods PUBLIC "/LTCG")
endif(MSVC)

target_link_libraries(
        DEternal_loadMods
        PUBLIC
        OpenSSL::Crypto
        ${CMAKE_DL_LIBS}
)
