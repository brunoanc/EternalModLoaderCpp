

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

#include <iostream>
#include <vector>

#include "PackageMapSpec/PackageMapSpec.hpp"
#include "PackageMapSpec/PackageMapSpecInfo.hpp"

/**
 * @brief Modify the PackageMapSpecInfo file in disk
 * 
 */
bool PackageMapSpecInfo::ModifyPackageMapSpec() const
{
    if (PackageMapSpec != nullptr && WasPackageMapSpecModified) {
        FILE *packageMapSpecFile = fopen(PackageMapSpecPath.c_str(), "wb");

        if (!packageMapSpecFile) {
            delete PackageMapSpec;
            return false;
        }

        try {
            std::string newPackageMapSpecJson = PackageMapSpec->Dump();

            if (fwrite(newPackageMapSpecJson.c_str(), 1, newPackageMapSpecJson.size(), packageMapSpecFile) != newPackageMapSpecJson.size()) {
                throw std::exception();
            }
        }
        catch (...) {
            return false;
        }

        fclose(packageMapSpecFile);
        delete PackageMapSpec;
    }

    return true;
}