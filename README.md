# EternalModLoaderCpp
![Build Status](https://github.com/PowerBall253/EternalModLoaderCpp/actions/workflows/cmake.yml/badge.svg)

C++ port of EternalModLoader by proteh, coded specifically for Linux users.

## Features
* Smaller file size (only around ~600 KB).

* Faster mod loading times.

* Better Linux compatibility (should solve issues for people having problems with the C# port).

## Compiling
The project uses Cmake to compile, and requires the zlib, OpenSSL and opus-tools libraries to be installed.

First clone the repo and all submodules by running:

```
git clone https://github.com/PowerBall253/EternalModLoaderCpp.git --recurse-submodules
```

Then, generate the makefile by running:
```
cd EternalModLoaderCpp
mkdir build
cd build
cmake ..
```

Finally, build with:
```
make
```

The DEternal_loadMods executable will be in the "build" folder.

## Credits
* proteh: For making the C# port this code is based on.

* SutandoTsukai181: For making the original EternalModLoader.

* leveste: For helping to test the debug versions of the code.

* Fry: For creating the original sound mod loading tool.
