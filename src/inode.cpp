#include <inode.hpp>

#include <cstring>

inode::inode(size_type inode_id,inode_types type,size_type fileSize)
:id(inode_id),type(type),fileSize(fileSize),numHardLinks(0)
{
    std::memset(&pointers,0,sizeof(inode_poiters));
}