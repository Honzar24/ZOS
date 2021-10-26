#pragma once

#include <cstdlib>
#include <ostream>
#include <iomanip>

#include "config.hpp"

class superBlock
{
    public:
    ///login autora FS
    char signature[maxSignatureLenght + 1];

    ///popis vygenerovaného FS              
    char description[maxDescriptionLenght + 1];

    ///celkova velikost VFS v Bytech 
    size_type disk_size;

    ///velikost bloku v Bytech           
    size_type block_size;

    ///pocet bloku        
    size_type block_count;

    ///pocet inodu
    size_type inode_count;

    ///adresa pocatku bitarray i-uzlu
    pointer_type bitarray_inode_start_address;

    ///adresa pocatku bitarray datových bloku
    pointer_type bitarray_data_block_start_address;

    ///adresa pocatku i-uzlu
    pointer_type inode_start_address;

    ///adresa pocatku datovych bloku   
    pointer_type data_start_address;

    superBlock(const char[maxSignatureLenght],const char[maxDescriptionLenght],size_type disk_size,size_type block_size,size_type inode_count,size_type block_count);
    superBlock(size_type disk_size,size_type block_size);
    superBlock(size_type disk_size);
    superBlock();

    friend std::ostream& operator <<(std::ostream& os, superBlock& sb);

    std::ostream& wb(std::ostream& os);

    bool setupFilePointers();
};
