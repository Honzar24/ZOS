#pragma once

#include <cstdlib>
#include <ostream>
#include <iomanip>

#include "config.hpp"

class superBlock
{
    public:
    //login autora FS
    char signature[maxSignatureLenght + 1];

    //popis vygenerovaného FS              
    char description[maxDescriptionLenght + 1];

    //celkova velikost VFS v Bytech 
    size_type diskSize;

    //velikost bloku v Bytech           
    size_type blockSize;

    //pocet bloku        
    size_type blockCount;

    //pocet inodu
    size_type inodeCount;

    private:
    //adresa pocatku bitarray i-uzlu
    pointer_type bitarray_inode_address;

    //adresa pocatku bitarray datových bloku
    pointer_type bitarray_data_block_address;

    //adresa pocatku i-uzlu
    pointer_type inode_address;

    //adresa pocatku datovych bloku   
    pointer_type data_address;

    public:

    superBlock(const char[maxSignatureLenght],const char[maxDescriptionLenght],size_type diskSize,size_type blockSize,size_type inodeCount,size_type blockCount);
    superBlock(size_type diskSize,size_type blockSize);
    superBlock(size_type diskSize);
    superBlock();

    inline pointer_type bitarrayInodeAddress(){
        return bitarray_inode_address;
    }
    inline pointer_type bitarrayDataBlockAddress(){
        return bitarray_data_block_address;
    }
    inline pointer_type inodeAddress(){
        return inode_address;
    }
    inline pointer_type dataAddress(){
        return data_address;
    }
    inline size_t bitarrayInodeAddressBytes(){
        return bitarray_data_block_address - bitarray_inode_address;
    }
    inline size_t bitarrayDataBlockAddressBytes(){
        return inode_address - bitarray_data_block_address;
    }

    bool setupFilePointers();
};
