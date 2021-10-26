#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <vector>

#include <fileSystem.hpp>
#include <inode.hpp>

void writeBitArray(std::ostream& os,std::vector<bool>& bitArray)
{
    for (auto it = bitArray.cbegin();it != bitArray.cend();)
    {
        char byte = 0;
        for(unsigned char mask = 1; mask > 0 && it != bitArray.cend(); it++, mask <<= 1)
            if(*it)
                byte |= mask; 
        os.write(&byte,sizeof(byte));
    }
}

fileSystem::fileSystem(std::string& fileName): fileName(fileName){
    fileStream.open(fileName,std::ios::out| std::ios::in | std::ios::binary);
    if (!fileStream.is_open())
    {
        std::cerr << "File can not be open by program. " << strerror(errno) << std::endl;
    }   
}
fileSystem::~fileSystem(){
}

bool fileSystem::format(size_type size)
{
    fileStream.open(fileName, std::ios::out | std::ios::in | std::ios::binary | std::ios::trunc);
    if (!fileStream.is_open())
    {
        std::cerr << "File can not be create by program. " << strerror(errno) << std::endl;
        return false;
    }

    sb.disk_size = size;

    size-= sizeof(superBlock);
    
    sb.block_count = size / ((1 + pomerDataInode)/8 + pomerDataInode + sb.block_size);
    sb.inode_count = pomerDataInode * sb.block_count;

    if (!sb.setupFilePointers())
    {
        return false;
    }
    

    sb.wb(fileStream);
    for(size_t i = sizeof(superBlock); i < sb.disk_size; i++)
    {
        char z = 0;
        fileStream.write(&z,sizeof(char));
    }
    //inode bitArray
    std::vector<bool> inodeArray(sb.inode_count);
    inodeArray[0]=true;
    fileStream.seekp(sb.bitarray_inode_start_address);
    writeBitArray(fileStream,inodeArray);
    //data blocks bitArray
    fileStream.seekp(sb.bitarray_data_block_start_address);
    std::vector<bool> dataArray(sb.block_count);
    dataArray[0] = true;
    writeBitArray(fileStream,dataArray);


    //inode section
    fileStream.seekp(sb.inode_start_address);
    char placeholder[sizeof(inode)]="";
    std::memset(placeholder,'=',sizeof(placeholder)-1);
    placeholder[0] = '<';
    placeholder[75] = 'p';
    placeholder[sizeof(placeholder)-2] = '>';
    for (size_t i = 0; i < sb.inode_count; i++)
    {
        fileStream.write(placeholder,sizeof(placeholder));
    }

    //data section
    fileStream.seekp(sb.data_start_address);
    const size_t bsize = sb.block_size;
    char* placeholderd = new char[bsize];
    std::memset(placeholderd,'d',bsize-1);
    placeholderd[0] = '<';
    placeholderd[bsize-2] = '>';
    std::cout << placeholderd << std::endl;
    for (size_t i = 0; i < sb.inode_count; i++)
    {
        fileStream.write(placeholderd,bsize);
        
    }
    

    
    
    return true;
    
}
