#pragma once



#include "config.hpp"

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
public:
    enum inode_types: counter_type{
    nodef=0,
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
    inode(size_type inode_id,inode_types type,size_type fileSize);
    inline inode():inode(0,nodef,0){};    
};
