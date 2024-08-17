/*
* This file is part of EternalModLoaderCpp (https://github.com/PowerBall253/EternalModLoaderCpp).
* Copyright (C) 2021 PowerBall253
*
* EternalModLoaderCpp is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* EternalModLoaderCpp is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with EternalModLoaderCpp. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef PROGRAMOPTIONS_HPP
#define PROGRAMOPTIONS_HPP

#include <string>
#include <sstream>

#define VERSION 23

#ifdef _WIN32
#define SEPARATOR static_cast<char>(fs::path::preferred_separator)
#else
#define SEPARATOR fs::path::preferred_separator
#endif

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

class ProgramOptions
{
public:
    inline static std::string BasePath;
    inline static bool ListResources{false};
    inline static bool Verbose{false};
    inline static bool SlowMode{false};
    inline static bool LoadOnlineSafeModsOnly{false};
    inline static bool CompressTextures{false};
    inline static bool MultiThreading{true};
    inline static bool AreModsSafeForOnline{true};
    inline static std::string BlangFileContainerRedirect;

    static std::stringstream GetProgramOptions(char **arguments, int count);
};

#endif
