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
	./vendor/murmur2/MurmurHash2.c
	./vendor/murmur2/MurmurHash2.h
        )

# Compiler arguments
if(MSVC)
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
        set(CMAKE_CXX_FLAGS "/O2 /Oi /Ot /GL /EHsc")
else()
        if(DEFINED ENV{ETERNALMODLOADERCPP_STATIC})
                set(CMAKE_CXX_FLAGS "-Wall -Werror -pthread -Ofast -static -s")
        else()
                set(CMAKE_CXX_FLAGS "-Wall -Werror -pthread -Ofast -s")
        endif()
endif(MSVC)

# Add executable to build
add_executable(DEternal_loadMods ${SOURCES})

# Add include directories
target_include_directories(DEternal_loadMods PRIVATE ./include ./vendor)

# Find OpenSSL
if(DEFINED ENV{ETERNALMODLOADERCPP_STATIC})
        set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

find_package(OpenSSL "3.0" REQUIRED)

if(MSVC)
        target_link_options(DEternal_loadMods PUBLIC "/LTCG")
endif(MSVC)

# Link OpenSSL and ooz static lib
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        target_link_libraries(DEternal_loadMods OpenSSL::Crypto ${CMAKE_SOURCE_DIR}/vendor/ooz/ooz.lib)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        target_link_libraries(DEternal_loadMods OpenSSL::Crypto ${CMAKE_DL_LIBS} ${CMAKE_SOURCE_DIR}/vendor/ooz/libooz.a)
endif()
