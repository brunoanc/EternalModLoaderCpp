# EternalModLoaderCpp
![Build Status](https://github.com/PowerBall253/EternalModLoaderCpp/actions/workflows/test.yml/badge.svg)

C++ port of EternalModLoader by proteh, coded specifically for Linux users, but also Windows compatible.

## Features
* Smaller file size.

* Faster mod loading times.

* Better Linux compatibility (should solve issues for people having problems with the C# port).

## Compiling
The project uses Cmake to compile, and requires the OpenSSL library to be installed. It also needs MSVC or the MinGW toolchain on MSYS to compile on Windows.

First clone the repo by running:

```
git clone https://github.com/PowerBall253/EternalModLoaderCpp.git
```

Then, generate the makefile by running:
```
cd EternalModLoaderCpp
cmake -B build .. # Append "-G 'MSYS Makefiles'" on MSYS
```

Finally, build with:
```
cmake --build build --config Release
```

The DEternal_loadMods executable will be in the "build" folder in Linux/MinGW and in the "build/Release" folder in MSVC.

## Credits
* proteh: For making the C# port this code is based on.

* SutandoTsukai181: For making the original EternalModLoader.

* leveste: For helping to test the debug versions of the code.

* Fry: For creating the original sound mod loading tool.

* SamPT: For figuring out how to load new assets with proteh.

* d3vilguard: For helping me test the latest versions of the code.
