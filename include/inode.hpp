#pragma once



#include "config.hpp"

struct inode_poiters
{
    //prime ukazatele na databloky
    pointer_type direct[directPointersCount];
    // neprimi ukazatel 1.radu => na blok ukazatelu na databloky
    pointer_type indirect1[indirect1PointersCount];
    // neprimi ukazatel 2.radu => 1.radu => na blok ukazatelu na databloky
    pointer_type indirect2[indirect1PointersCount];
};


class inode
{
public:
    enum inode_types : counter_type {
        nodef = 0,
        file,
        dir
    };


    size_type id;
    inode_types type;
    //Velikost souboru v Bajtech
    size_type fileSize;
    //Pocet linku ukazujici na tento inode
    counter_type numHardLinks;
    inode_poiters pointers;
public:
    inline inode(size_type id, inode_types type, size_type fileSize) :id(id), type(type), fileSize(fileSize), numHardLinks(0)
    {
        std::memset(&pointers, 0, sizeof(inode_poiters));
    }
    inline inode(size_type id) : inode(id, nodef, 0) {};
    inline inode() : inode(0, nodef, 0) {};
};
