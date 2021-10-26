#pragma once

#include <fstream>
#include <iostream>
#include <sstream>

#include "superBlock.hpp"


/**
 * i - pocet inodu
 * b - pocet data bloku
 * 
 * diagram
 * 0                                                                              diskSize
 * | sizeof(superBlock) | i/8 | b/8 | i * sizeof(inode) | b * blockSize | padding |
 * 
 * padding je nepovinne pole slouzi pouze k dorovnani skutecne velikosti do pozadovane velikosti
 * 
 */


class fileSystem
{
private:
    std::string fileName;
    std::fstream fileStream;
    superBlock sb;
    
public:
    bool format(size_type size);

    fileSystem(std::string& fileName, superBlock sb);
    fileSystem(std::string& fileName);
    ~fileSystem();
};
