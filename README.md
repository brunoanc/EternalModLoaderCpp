# EternalModLoaderCpp
C++ port of EternalModLoader by proteh, coded specifically for Linux users.

## Features
* Smaller file size (only around ~300 kb).

* Faster mod loading times.

* Portability (should solve issues for people having problems with the C# port).

## Compiling
The project uses Cmake to compile, and requires the zlib library to be installed.

First clone the repo:

```
https://github.com/PowerBall253/EternalModLoaderCpp.git
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
