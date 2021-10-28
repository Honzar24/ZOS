#include <cstring>
#include <cmath>
#include <cassert>
#include <iostream>

#include <superBlock.hpp>
#include <inode.hpp>


superBlock::superBlock(const char sig [maxSignatureLenght],const char desc[maxDescriptionLenght],size_type diskSize,size_type blockSize,size_type inodeCount,size_type blockCount):
diskSize(diskSize),blockSize(blockSize),inodeCount(inodeCount),blockCount(blockCount)
{
    std::strncpy(signature,sig,maxSignatureLenght);
    signature[maxSignatureLenght] = '\0';
    std::strncpy(description,desc,maxDescriptionLenght);
    description[maxDescriptionLenght] = '\0'; 
    setupFilePointers();   

}

superBlock::superBlock(size_type diskSize,size_type blockSize):superBlock(defaulSignature,defaulDescription,diskSize,blockSize,defaultInodeCount,defaultBlockCount){}
superBlock::superBlock(size_type diskSize):superBlock(diskSize,defaultBlockSize){}
superBlock::superBlock(){
    std::memset(this,0,sizeof(superBlock));
    blockSize = defaultBlockSize;
}

bool superBlock::setupFilePointers(){
    size_t inodeBytesCount = inodeCount % 8 == 0 ? inodeCount / 8 : inodeCount / 8 + 1;
    size_t dataBytesCount = blockCount % 8 == 0 ? blockCount / 8 : blockCount / 8 + 1;
    //file pointery
    bitarray_inode_address = sizeof(superBlock);
    bitarray_data_block_address = bitarray_inode_address + inodeBytesCount;
    inode_address = bitarray_data_block_address + dataBytesCount;
    data_address = inode_address + inodeCount * sizeof(inode);

    size_t dSize = data_address + blockCount * blockSize;
    if(dSize > diskSize)
    {
        std::cerr << "FATAL:Format failed to fit filesystem in given bounds" << std::endl << "Actual size:"<< dSize << " expected: " << diskSize << std::endl;
        return false;
    }

    return true;

}
