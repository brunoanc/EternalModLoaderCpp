#include <iostream>

#include "EternalModLoader.hpp"

void ModifyPackageMapSpec()
{
    if (PackageMapSpecInfo.PackageMapSpec != NULL && PackageMapSpecInfo.WasPackageMapSpecModified) {
        FILE *packageMapSpecFile = fopen(PackageMapSpecInfo.PackageMapSpecPath.c_str(), "wb");

        if (!packageMapSpecFile) {
            std::cerr << RED << "ERROR: " << RESET << "Failed to write " << PackageMapSpecInfo.PackageMapSpecPath << std::endl;
        }
        else {
            try {
                std::string newPackageMapSpecJson = PackageMapSpecInfo.PackageMapSpec->Dump();

                if (fwrite(newPackageMapSpecJson.c_str(), 1, newPackageMapSpecJson.size(), packageMapSpecFile) != newPackageMapSpecJson.size())
                    throw std::exception();
            }
            catch (...) {
                std::cerr << RED << "ERROR: " << RESET << "Failed to write " << PackageMapSpecInfo.PackageMapSpecPath << std::endl;
            }

            std::cout << "Modified "<< YELLOW << PackageMapSpecInfo.PackageMapSpecPath << RESET << std::endl;
            fclose(packageMapSpecFile);
        }

        delete PackageMapSpecInfo.PackageMapSpec;
    }
}