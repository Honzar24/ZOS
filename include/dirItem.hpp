#pragma once

#include <cstring>

#include "config.hpp"


class dirItem
{
public:
    //Prizazeny inode
    size_type inode_id;
    //Nazev souboru/slozky
    char name[fileLiteralLenght];    

    inline dirItem()
    {
        std::memset(this, '\0', sizeof(dirItem));
        inode_id = 0;
    };
    inline dirItem(const char name[fileLiteralLenght], size_type id) :dirItem()
    {
        inode_id = id;
        std::strncpy(this->name, name, fileLiteralLenght - 1);
    }
};