#pragma once

#include <cstring>

#include "config.hpp"


class dirItem
{
public:
    //Nazev souboru/slozky
    char name[fileLiteralLenght];
    //Prizazeny inode
    size_type inode_id;

    inline dirItem() {
        memset(this->name, '\0', fileLiteralLenght);
        inode_id = 0;
    };
    inline dirItem(const char name[fileLiteralLenght], size_type& id) :inode_id(id)
    {
        memset(this->name, '\0', fileLiteralLenght);
        strncpy(this->name, name, fileLiteralLenght - 1);
    }
};
