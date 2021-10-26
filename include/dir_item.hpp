#pragma once

#include <cstring>

#include "config.hpp"


class dir_item
{
    public:
    //Nazev souboru/slozky
    char name[fileLiteralLenght];
    //Prizazeny inode
    size_type inode_id;

    inline dir_item(char nname[fileLiteralLenght],size_type& id):inode_id(id)
    {
        memset(name,'\0',fileLiteralLenght);
        strncpy(name,nname,fileLiteralLenght - 1);                
    }
};
