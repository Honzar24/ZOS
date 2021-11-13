#pragma once

#include <fstream>
#include <iostream>
#include <vector>
#include <utility>

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
 * padding je nepovinne pole slouzi pouze k dorovnani skutecne velikosti do pozadovane velikosti
 *
 * : muzeme stanovit nejake minimalni pozadavky?
 * jiste
 * podminky:                                        konkretni hodnoty
 *          block size >= 2*sizeof(dirItem) <24>    => >=48
 *          block size % sizeof(pointer_type) == 0  => % 8 == 0
 *
 *
 * : je mozne vytvorit soubor mensi nez pozadovana velikost?
 *	asi jo
 * TODO: co delat pri nedostatku inodu?
 * TODO: co delat pokud dosli data bloky?
 * TODO: co delat pokud dana inoda jiz nemuze na addresovat dalsi block?
 * TODO: vypis self a parent odkazu v ls?
 * TODO: mrdir /?
 * TODO: mazat automaticky neplatne hardlinky?
 *
 *
 * : memory regen pri odstraneni posledni polozky v bloku
 * 	lokani vramci data bloku
 * : globalni memory regen nad inodem?
 * 	neni potreba
 *
 * optimalizace io / minilalizace runtime mem
 * 	optimalizace na velikost datablocku
 *
 *
 */

using file_name_t = const char[maxFileNameLenght];

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
    using error_string_pair = std::pair<errorCode, std::string>;

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
     * @brief odstrani zaznam obsahujici hodnotu removedID
     *
     * @param inode
     * @param removedID
     * @return true odstaneno
     * @return false nepodarilo se najit tento zaznam
     */
    bool removeDirItem(inode& inode, dirItem& dirItem);
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
    void freeInode(inode& inode);
    /**
     * @brief zabere jeden datablock
     * pokud neni dostatek mista na disku ukonci program
     *
     * @return pointer_type ukazatel na block
     */
    pointer_type alocateDataBlock();
    /**
     * @brief vycisti data block a nastavi priznak v bitArray
     *
     */
    void freeDataBlock(pointer_type dataPointer);
    /**
     * @brief pokusi se zabrat pozadovany pocet bloku a vrati jejich id
     * pokud fs nema dostatek vrati 0 a nezabere zadny
     *
     * @param numberOfDataBlocks pocet pozadovanych bloku
     * @return vector pointeru jeho velikost je rovna pocetu zabranych bloku
     */
    std::vector<pointer_type> alocateDataBlocks(size_t numberOfDataBlocks);
    /**
     * @brief vytvori vector data bloku ktere se vazou k danemu inodu
     *
     * @param inode
     * @return vector pointeru na nove zabrane data blocky
     */
    std::vector<pointer_type> getDataPointers(inode& inode);
    /**
     * @brief naplni vector vsemi dirItemy v inodu
     *
     * @param inode
     * @return std::vector<std::pair<dirItem, pointer_type>>  vsech dir itemu patrici k danemu inodu
     */
    std::vector<std::pair<dirItem, pointer_type>> getDirItems(inode& inode);
    /**
     * @brief naplni vector pouze validnimi dirItemy
     * 
     * @param inode 
     * @return std::vector<std::pair<dirItem, pointer_type>> 
     */
    std::vector<std::pair<dirItem, pointer_type>> getValidDirItems(inode& inode);
    /**
     * @brief Propocita "nejlepsi" pocet data bloku pro velikost data bloku a pocet inodu podle pomeru viz. config
     * ze superBloku se vyuzije pouze block size zbyle parametry se dopocitaji nebo doplni z configu
     *
     * @param size velikost vysledneho file systemu
     * @return true vse vporadku system byl naformatovan
     * @return false takovy file system nelze setrojit nebo zapsat
     */
    errorCode calcAndFormat(size_type size);

public:
    /**
     * @brief vytvori soubor od danym
     *
     * @param dirID
     * @param fileName
     * @param data
     * @return errorCode
     */
    errorCode touch(size_type dirID, file_name_t filename, const char data[] = "");

    /**
     * @brief kopiruje src do dest
     *
     * @param srcItem
     * @param destInodeID
     * @return errorCode OK |FILE NOT FOUND (neni zdroj) |PATH NOT FOUND (neexistuje cilova cesta)
     */
    errorCode cp(dirItem& srcItem, size_type destInodeID);
    /**
     * @brief prejmenuje src na dest nebo presune src do dest adresare
     *
     * @param parentID
     * @param srcInodeID
     * @param destInodeID
     * @return errorCode
     */
    errorCode mv(size_type parentID, dirItem& srcItem, size_type destInodeID);
    /**
     * @brief maze soubor
     *
     * @param parentID 
     * @param item 
     * @return errorCode
     */
    errorCode rm(size_type parentID,dirItem& item);

    /**
    * @brief Zalozeni adresare pod parentInnodeID
    *
    * @param dirName
    * @param parentInnodeID
    * @return errorCode OK|EXIST (nelze zalozit, jiz existuje)|PATH NOT FOUND (neexistuje zadana cesta)
    */
    errorCode mkdir(const char dirName[fileLiteralLenght], size_type parentInnodeID);

    /**
     * @brief Smazani prazneho adresare
     *
     * @param inodeID
     * @return errorCode OK|FILE NOT FOUND (neexistujici adresar)|NOT EMPTY (adresar obsahuje podadresare, nebo soubory)
     */
    errorCode rmdir(size_type inodeID);

    /**
     * @brief Zjistreni obsahu adresare v textove podobe
     *
     * @param inodeID ID adresare
     * @param dirItems vector pro vysledek
     * @return errorCode OK|PATH NOT FOUND (neexistujici adresar)
     */
    errorCode ls(size_type inodeID, std::vector<std::string>& dirItems);

    /**
     * @brief zobrazeni informace o inodu
     *
     * @param parentID
     * @param name
     * @return error_string_pair OK | FILE NOT FOUND pokud OK tak v stringu jsou ulozeny informace o inodu
     */
    error_string_pair info(size_type parentID, const file_name_t name);

    /**
     * @brief vytvori hard link na soubor src do adresare destDirID s jmenem fileName
     *
     * @param srcID souboroveho inodu
     * @param destDirID adresar kde chceme hardlink umistit
     * @param fileName 
     * @return errorCode
     */
    errorCode ln(size_type srcID, size_type destDirID, const file_name_t fileName);


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
using error_string_pair = std::pair<errorCode, std::string>;

inline std::ostream& operator<<(std::ostream& os, errorCode errorCode)
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
