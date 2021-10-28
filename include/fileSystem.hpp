#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <bitset>

#include "superBlock.hpp"


/**
 * i - pocet inodu
 * b - pocet data bloku
 * 
 * diagram
 * 0                                                                                      diskSize
 * | sizeof(superBlock) | i/8(+1) | b/8(+1) | i * sizeof(inode) | b * blockSize | padding |
 * 
 * pokud i a b nejsou delitelne 8 tak jejich velikost je celociselne deleni + 1
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
    /**
     * @brief 
     * 
     * @param bitArray 
     * @return std::pair<pointer_type,size_t> pozice kurzoru ve istreamu a hodnota <0;8> urcujici pozici volneho bitu v Bytu
     *  pokud pozice je 0 bitArray nema volny bit
     */
    std::pair<pointer_type,size_t> findFeeBit(pointer_type& bitArray);
    /**
     * @brief zabrani prvni volne inody na disku
     * 
     * @return size_type inodeId nove inody 0 znamena ze alokace selhala
     */
    size_type alocateNewInode();
    /**
     * @brief nastaveni bitu o existenci inodu na 0
     * 
     * @param inodeId inodeId
     */
    void freeInode(size_type inodeId);
    
public:

    
    bool format(size_type size);
    void format();
    bool makeDir(char dirName[fileLiteralLenght],size_type parentInnodeID);
    /**
     * @brief naformatuje soubor podle pozadavku superbloku(disksize,<block size>,<block count>,<inode count>)
     * prarametry v <> jsou nepovinne ale lze je nastavit
     * 
     * @param fileName 
     * @param sb 
     */
    fileSystem(std::string& fileName, superBlock& sb);
    /**
     * @brief pokusi se nacist jiz vytvoreni filesytem
     * 
     * @param fileName jmeno souboru filesystemu
     */
    fileSystem(std::string& fileName);
    ~fileSystem();
};
