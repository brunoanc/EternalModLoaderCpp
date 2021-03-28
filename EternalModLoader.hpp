#ifndef ETERNALMODLOADER_CLASSES_H
#define ETERNALMODLOADER_CLASSES_H

#include <vector>

#include "mmap_allocator/mmappable_vector.h"

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"

class Mod {
public:
    std::string Name;
    std::vector<std::byte> FileBytes;
    bool isBlangJson;
    explicit Mod(std::string name) {
        Name = name;
    }
};

class ResourceChunk {
public:
    std::string Name;
    long NameId;
    long FileOffset;
    long SizeOffset;
    long SizeZ;
    long Size;
    std::byte CompressionMode;
    ResourceChunk(std::string name, long fileOffset) {
        Name = name;
        NameId = 0;
        FileOffset = fileOffset;
        SizeOffset = 0;
        SizeZ = 0;
        Size = 0;
        CompressionMode = (std::byte)(0);
    }
};

class ResourceInfo {
public:
    std::string Name;
    std::string Path;
    int FileCount;
    int TypeCount;
    int StringsSize;
    long NamesOffset;
    long InfoOffset;
    long Dummy7Offset;
    long DataOffset;
    long IdclOffset;
    int UnknownCount;
    int FileCount2;
    long NamesOffsetEnd;
    long UnknownOffset;
    long UnknownOffset2;
    std::vector<Mod> ModList;
    std::vector<Mod> ModListNew;
    std::vector<std::string> NamesList;
    std::vector<ResourceChunk> ChunkList;
    ResourceInfo(std::string name, std::string path) {
        Name = name;
        Path = path;
        FileCount = 0;
        TypeCount = 0;
        StringsSize = 0;
        NamesOffset = 0;
        InfoOffset = 0;
        Dummy7Offset = 0;
        DataOffset = 0;
        IdclOffset = 0;
        UnknownCount = 0;
        FileCount2 = 0;
        NamesOffsetEnd = 0;
        UnknownOffset = 0;
        UnknownOffset2 = 0;
    }
};

class BlangString {
public:
    unsigned int Hash;
    std::string Identifier;
    std::string Text;
    std::vector<std::byte> Unknown;
    BlangString() {
        Hash = 0;
        Identifier = "";
        Text = "";
    }
    BlangString(unsigned int hash, std::string identifier, std::string text, std::vector<std::byte> unknown) {
        Hash = hash;
        Identifier = identifier;
        Text = text;
        Unknown = unknown;
    }
};

class BlangFile {
public:
    long UnknownData;
    std::vector<BlangString> Strings;
};

inline bool operator == (ResourceChunk &chunk1, const ResourceChunk &chunk2)
{
    if(chunk1.Name == chunk2.Name)
    return true;
    else
    return false;
}

extern std::string BasePath;
extern std::vector<ResourceInfo> ResourceList;

std::string PathToRes(std::string name);
void ReadChunkInfo(mmap_allocator_namespace::mmappable_vector<std::byte>& mem, int resourceIndex);
int GetChunk(std::string name, int resourceIndex);
void ReplaceChunks(mmap_allocator_namespace::mmappable_vector<std::byte>& mem, int resourceIndex);
void AddChunks(mmap_allocator_namespace::mmappable_vector<std::byte>& mem, int resourceIndex);
void ReadResource(mmap_allocator_namespace::mmappable_vector<std::byte>& mem, int resourceIndex);
int GetResourceInfo(std::string resourceName);
std::vector<std::byte> LongToVector(long number, int numberOfBytes);
std::vector<std::byte> VectorIntegralAdd(std::vector<std::byte>& vect, int numberOfBytes, long numbertoAdd);
long VectorToNumber(std::vector<std::byte>& vect, int numberOfBytes);
std::vector<std::byte> IdCrypt(std::vector<std::byte> fileData, std::string internalPath, bool decrypt);
BlangFile ParseBlang(std::vector<std::byte>& blangBytes, std::string& resourceName);
std::vector<std::byte> WriteBlangToVector(BlangFile blangFile, std::string& resourceName);
std::string RemoveWhitespace(std::string& stringWithWhitespace);
std::string ToLower(std::string& str);

#endif
