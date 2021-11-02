#pragma once

#include <fstream>
#include <iostream>
#include <vector>

#include "superBlock.hpp"
#include "fileBitArray.hpp"
#include "inode.hpp"
#include "dirItem.hpp"

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
 * TODO: diskSize je velikost souboru
 * TODO: je mozne vytvorit soubor mensi nez pozadovana velikost
 * TODO: co delat pri nedostatku inodu
 * TODO: co delat pokud dosli data bloky
 * TODO: co delat pokud dana inoda jiz nemuze na addresovat dalsi block
 * 
 * optimalizace io / minilalizace runtime mem
 * 
 * 
 * padding je nepovinne pole slouzi pouze k dorovnani skutecne velikosti do pozadovane velikosti
 *
 */



class fileSystem
{
public:
    enum class errorCode :int8_t {
        UNKNOW = -1,
        OK,
        CANNOT_CREATE_FILE,
        FILE_NOT_FOUND,
        PATH_NOT_FOUND,
        NOT_EMPTY,
        EXIST,
        CAN_NOT_CREATE_SUPERBLOCK,
        INODE_POOL_FULL
    };

private:
    std::string fileName;
    std::fstream fileStream;
    superBlock sb;
    fileBitArray inodeBitArray;
    fileBitArray dataBlockBitArray;
    /**
     * @brief prida pointer do databloku
     * 
     * @param pointer 
     * @return true ano podarilo se pridat
     * @return false nepodarilo
     */
    bool addToIndirect(pointer_type pointer);
    /**
     * @brief pokusi se pridat pointer k inodu
     * 
     * @param inode 
     * @param pointer 
     */
    void addPointer(inode& inode, pointer_type pointer);
    /**
     * @brief prida zaznam do data bloku o novem soubodu v adresari
     * 
     * @param inode 
     * @param dirItem 
     * @return true pridano
     * @return false nelze pridat uz dany literal je obsazen
     */
    bool addDirItem(inode& inode, dirItem& dirItem);
    /**
     * @brief vytvori root adresar na nove zformatovanem disku
     *
     */
    void createRoot();
    /**
     * @brief zabrani prvni volne inody na disku
     *
     * @return inode pokud inode.id je 0 tento inode je pozovany za neplatny
     * protoze je to id root adresare a to nemuze byt nikdy uvolneno
     */
    inode alocateNewInode();
    /**
     * @brief nastaveni bitu o existenci inodu na 0
     *
     * @param inodeId inodeId
     */
    void freeInode(size_type inodeId);
    /**
     * @brief zabere jeden datablock
     * pokud neni dostatek mista na disku ukonci program
     *
     * @return pointer_type ukazatel na block
     */
    pointer_type alocateDataBlock();
    /**
     * @brief pokusi se zabrat pozadovany pocet bloku a vrati jejich id
     * pokud fs nema dostatek vrati 0 a nezabere zadny
     *
     * @param numberOfDataBlocks pocet pozadovanych bloku
     * @param vector vector pro ulozeni pointeru
     * @return size_t 0 pocet zabranych bloku
     */
    size_t alocateDataBlocks(size_t numberOfDataBlocks, std::vector<pointer_type>& vector);
    /**
     * @brief vytvori vector data bloku ktere se vazou k danemu inodu
     *
     * @param inode
     * @param pointers kam chceme pointery ulozit
     */
    void getDataPointers(inode& inode, std::vector<pointer_type>& pointers);

public:
    /**
     * @brief Zalozeni adresare pod parentInnodeID
     *
     * @param dirName
     * @param parentInnodeID
     * @return errorCode OK|EXIST|PATH NOT FOUND
     */
    errorCode makeDir(const char dirName[fileLiteralLenght], size_type parentInnodeID);

    /**
     * @brief Propocita "nejlepsi" pocet data bloku pro velikost data bloku a pocet inodu podle pomeru viz. config
     * ze superBloku se vyuzije pouze block size zbyle parametry se dopocitaji nebo doplni z configu
     *
     * @param size velikost vysledneho file systemu
     * @return true vse vporadku system byl naformatovan
     * @return false takovy file system nelze setrojit nebo zapsat
     */
    errorCode calcAndFormat(size_type size);
    /**
     * @brief podle nastaveni v superbloku naformatuje soubor
     *
     * @return errorCode
     */
    errorCode format();
    /**
     * @brief naformatuje soubor podle pozadavku superbloku
     *  A) je zadana pozadovana velikost disku diskSize != 0 nastaveni ostatnich parametru viz, config
     *  B) jsou zadany pozadovane pocty data bloku a inodu a popripade jeste velikost data bloku ale to je nepoviny udaj viz. config
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
    ~fileSystem() = default;
};
using errorCode = fileSystem::errorCode;
inline std::ostream& operator<<(std::ostream& os, errorCode& errorCode)
{
    switch (errorCode)
    {
    case errorCode::OK:
        os << "OK";
        break;
    case errorCode::CANNOT_CREATE_FILE:
        os << "CANNOT CREATE FILE";
        break;
    case errorCode::FILE_NOT_FOUND:
        os << "FILE NOT FOUND";
        break;
    case errorCode::PATH_NOT_FOUND:
        os << "PATH NOT FOUND";
        break;
    case errorCode::NOT_EMPTY:
        os << "NOT EMPTY";
        break;
    case errorCode::EXIST:
        os << "EXIST";
        break;
    case errorCode::CAN_NOT_CREATE_SUPERBLOCK:
        os << "CAN NOT CREATE SUPERBLOCK";
        break;
    case errorCode::INODE_POOL_FULL:
        os << "INODE POOL FULL";
        break;
    case errorCode::UNKNOW:
    default:
        os << "UNKNOW";
        break;
    }
    return os;
};
