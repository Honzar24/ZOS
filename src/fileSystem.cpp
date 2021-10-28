#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <vector>

#include <fileSystem.hpp>
#include <inode.hpp>

fileSystem::fileSystem(std::string& fileName, superBlock& sb):fileName(fileName),sb(sb){
    if (sb.diskSize == 0)
    {
        sb.setupFilePointers();
        format();
    }
    else
    {
        calcAndFormat(sb.diskSize);
    }
    
    
}
fileSystem::fileSystem(std::string& fileName): fileName(fileName){
    fileStream.open(fileName,std::ios::out| std::ios::in | std::ios::binary);
    #ifndef NDEBUG
        if (!fileStream.is_open())
        {
            std::cerr << "WARN:File " << fileName << " can not be open by program. " << strerror(errno) << std::endl;
        }
    #endif   
}
fileSystem::~fileSystem(){
}

char* createBitArray(size_t bytes){
    char* bitArray = new char[bytes];
    std::memset(bitArray,0,bytes);
    return bitArray;
}

bool fileSystem::calcAndFormat(size_type size)
{
    sb.diskSize = size;
    // +2 protoze maximalne si pucim 2 Byty pro ByteArray
    assert(size > (sizeof(superBlock) + 2));     
    size-= sizeof(superBlock) + 2;  
    sb.blockCount = size / ((1 + pomerDataInode)/8 + pomerDataInode * sizeof(inode) + sb.blockSize);
    sb.inodeCount = pomerDataInode * sb.blockCount;
    assert(sb.blockCount >= minDataBlockCount);
    assert(sb.inodeCount >= minInodeCount);
    if (!sb.setupFilePointers())
    {
        return false;
    }
    return format();   
}

bool fileSystem::format(){
    fileStream.close();
    fileStream.open(fileName, std::ios::out | std::ios::in | std::ios::binary | std::ios::trunc);
    if (!fileStream.is_open())
    {
        std::cerr << "FATAL:File " << fileName << " can not be create by program. " << strerror(errno) << std::endl;
        return false;
    }
    //superBlock
    fileStream.write((char*)&sb,sizeof(superBlock));
    //inode bitArray
    fileStream.seekp(sb.bitarrayInodeAddress());
    size_t bytes = sb.bitarrayInodeAddressBytes();
    char* mem = createBitArray(bytes);
    fileStream.write(mem,bytes);
    delete[] mem;
    //data blocks bitArray
    fileStream.seekp(sb.bitarrayDataBlockAddress());
    bytes = sb.bitarrayDataBlockAddressBytes();
    mem = createBitArray(bytes);
    fileStream.write(mem,bytes);
    delete[] mem;
    #ifndef NDEBUG
        //inode section
        fileStream.seekp(sb.inodeAddress());
        char placeholder[sizeof(inode)];
        std::memset(placeholder,'=',sizeof(placeholder));
        placeholder[0] = '<';
        char text[] = " empty  inode ";
        std::memcpy(placeholder + sizeof(placeholder) / 2 - (sizeof(text) - 1) / 2,text,sizeof(text) - 1);
        placeholder[sizeof(placeholder) - 1] = '>';
        for (size_t i = 0; i < sb.inodeCount; i++)
        {
            fileStream.write(placeholder,sizeof(placeholder));
        }
        //data section
        fileStream.seekp(sb.dataAddress());
        const size_t bsize = sb.blockSize;
        char* placeholderd = new char[bsize];
        std::memset(placeholderd,'d',bsize);
        placeholderd[0] = '<';
        placeholderd[bsize - 1] = '>';
        for (size_t i = 0; i < sb.blockCount; i++)
        {
            fileStream.write(placeholderd,bsize);
            
        }
        delete[] placeholderd;
    #else
        //zabrani vyuzitelne pameti
        fileStream.seekp(sb.dataAddress() + sb.blockCount * sb.blockSize);
        fileStream.write("",1);
    #endif

    return true;
}
