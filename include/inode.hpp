#pragma once



#include "config.hpp"

struct inode_poiters
{
    //prime ukazatele na databloky
    pointer_type direct[directPointersCount];
    // neprimi ukazatel 1.radu => na blok ukazatelu na databloky
    pointer_type indirect[indirectPointersCount];
};


class inode
{
public:
    enum inode_types : counter_type {
        nodef = 0,
        file,
        dir,
    };

    //Unikatni identifikator idnodu
    size_type id;
    //file type
    inode_types type;
    //Velikost souboru v Bajtech
    size_type fileSize;
    //Pocet linku ukazujici na teto inode
    counter_type numHardLinks;
    //data pointery
    inode_poiters pointers;
public:
    inline inode(size_type id, inode_types type, size_type fileSize) :id(id), type(type), fileSize(fileSize), numHardLinks(0)
    {
        std::memset(&pointers, 0, sizeof(inode_poiters));
    }
    inline inode(size_type id) : inode(id, nodef, 0) {};
    inline inode() : inode(0, nodef, 0) {};
};
