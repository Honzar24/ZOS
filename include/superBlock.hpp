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

        //pocet inodu
    size_type inodeCount;

    //pocet bloku        
    size_type blockCount;



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
    /**
     * @brief Vytvori pouzitelny super block pro dane prametry pokud to nelze
     *
     * @param sig
     * @param desc
     * @param diskSize
     * @param blockSize
     * @param inodeCount
     * @param blockCount
     */
    superBlock(const char sig[maxSignatureLenght], const char desc[maxDescriptionLenght], size_type diskSize, size_type blockSize, size_type inodeCount, size_type blockCount);
    /**
     * @brief Vytvori pouzitelny super block pro dane prametry a dopocita jak velky diskovi prostu je potreba
     *
     * @param sig
     * @param desc
     * @param blockSize
     * @param inodeCount
     * @param blockCount
     */
    inline superBlock(const char sig[maxSignatureLenght], const char desc[maxDescriptionLenght], size_type blockSize, size_type inodeCount, size_type blockCount) :superBlock(sig, desc, 0, blockSize, inodeCount, blockCount) {};


    inline superBlock(size_type diskSize, size_type blockSize) : superBlock()
    {
        this->diskSize = diskSize;
        this->blockSize = blockSize;
    };
    inline superBlock(size_type diskSize) : superBlock()
    {
        this->diskSize = diskSize;
    };
    /**
     * @brief Vytvori superBlock s parametry 0 az na block size
     *
     */
    superBlock();

    inline pointer_type bitArrayInodeAddress()
    {
        return bitarray_inode_address;
    }
    inline pointer_type bitArrayDataBlockAddress()
    {
        return bitarray_data_block_address;
    }
    inline pointer_type inodeAddress()
    {
        return inode_address;
    }
    inline pointer_type dataAddress()
    {
        return data_address;
    }
    inline size_t bitarrayInodeAddressBytes()
    {
        return bitarray_data_block_address - bitarray_inode_address;
    }
    inline size_t bitarrayDataBlockAddressBytes()
    {
        return inode_address - bitarray_data_block_address;
    }
    inline size_t getBlockConnt(size_type requiredSize)
    {
        return requiredSize % blockSize != 0 ? requiredSize / blockSize : requiredSize / 8 + 1;
    };
    bool setupFilePointers();
};
