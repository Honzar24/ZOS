#pragma once

#include <any>

#include "config.hpp"

enum inode_types: counter_type{
    file = 0,
    dir,
};

struct inode_poiters
{
    // 13 primich ukazatelu na databloky
    pointer_type direct0;
    pointer_type direct1;
    pointer_type direct2;
    pointer_type direct3;
    pointer_type direct4;
    pointer_type direct5;
    pointer_type direct6;
    pointer_type direct7;
    pointer_type direct8;
    pointer_type direct9;
    pointer_type direct10;
    pointer_type direct11;
    pointer_type direct12;
    // neprimi ukazatel 1.radu => na blok ukazatelu na databloky
    pointer_type indirect1;
    // neprimi ukazatel 2. radu na blok ukazatelu na ukazatele 1. radu
    pointer_type indirect2;
    // neprimi ukazatel 3. radu na blok ukazatelu na ukazatele 2. radu
    pointer_type indirect3;
};


class inode
{
private:
    size_type generateInodeID();

public:
    //Unikatni identifikator idnodu
    size_type inode_id;
    //Velikost souboru v Bajtech
    size_type fileSize;

    counter_type numHardLinks;

    inode_poiters pointers;

    inode(std::any& data,size_type& dataSize);
    inode();

    
};
