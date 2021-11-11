#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>

#include <fileSystem.hpp>
#include <dirItem.hpp>
#include <log.hpp>

/*
*   Sekce smrti pozor
*   Ano makra. Proc? Tato konstrukce je vyuzivana k debugu bez pouziti runtime debugeru pouze za pomoci logu programu
*   Takto je mozno v logu primo videt z jake funkce je makro volano to to nevim jak by slo docilit pomoci funkce
*/

#define SEEKG(pos)\
    assert((!fileStream.fail()) && "preSEEKG");fileStream.seekg(pos);assert((!fileStream.fail()) && "posSEEKG")

#define SEEKP(pos)\
    assert((!fileStream.fail()) && "preSEEKG");fileStream.seekp(pos);assert((!fileStream.fail()) && "posSEEKP")

#define WRITE(data,sizeBytes)\
    assert((!fileStream.fail()) && "preWRITE");TRACE("Write to  address:" << STREAMADDRESS(fileStream.tellp()) << " size " << sizeBytes);fileStream.write(data, sizeBytes).flush();assert((!fileStream.fail())&& "posWRITE")

#define AWRITE(address,data,sizeBytes)\
    assert((!fileStream.fail()) && "preAWRITE");SEEKP(address);WRITE(data,sizeBytes)

#define READ(data,sizeBytes)\
    assert((!fileStream.fail()) && "preREAD");TRACE("Read from address:" << STREAMADDRESS(fileStream.tellg()) << " size " << sizeBytes);fileStream.read(data, sizeBytes);assert((!fileStream.fail()) && "posREAD")

#define AREAD(address,data,sizeBytes)\
    assert((!fileStream.fail()) && "preAREAD");SEEKG(address);READ(data,sizeBytes);

/*
*   Konec sekce smrti
*/

using dirItemP = std::pair<dirItem, pointer_type>;


fileSystem::fileSystem(std::string& fileName, superBlock& sb) : fileName(fileName), sb(sb), fileStream(fileName)
{
    if (sb.inodeAddress() != 0)
    {
        format();
    } else
    {
        calcAndFormat(sb.diskSize);
    }
}
fileSystem::fileSystem(std::string& fileName) : fileName(fileName), fileStream(fileName, std::ios::out | std::ios::in | std::ios::binary)
{
    if (!fileStream.is_open())
    {
        INFO("File " << fileName << " can not be open by program. " << strerror(errno) << "! Use format to create it");
        return;
    }
    AREAD(0, reinterpret_cast<char*>(&sb), sizeof(superBlock));
}
void fileSystem::createRoot()
{
    DEBUG("Creating root");
    auto root = alocateNewInode();
    root.type = inode::dir;
    dirItem dirItems[2];
    dirItems[0] = dirItem(".", root.id);
    dirItems[1] = dirItem("..", root.id);
    root.fileSize += 2 * sizeof(dirItem);
    addPointer(root, alocateDataBlock());
    AWRITE(sb.inodeAddress() + root.id * sizeof(inode), reinterpret_cast<char*>(&root), sizeof(root));
    AWRITE(root.pointers.direct[0], reinterpret_cast<char*>(dirItems), sizeof(dirItems));
}
bool fileSystem::addToIndirect(pointer_type pointer)
{
    size_t pointerCount = sb.blockSize / sizeof(pointer_type);
    pointer_type pointerBlock[pointerCount];
    size_t basePointer = fileStream.tellg();
    READ(reinterpret_cast<char*>(pointerBlock), pointerCount * sizeof(pointer_type));
    for (size_t i = 0; i < pointerCount; i++)
    {
        if (pointerBlock[i] == 0)
        {
            auto data = reinterpret_cast<char*>(&pointer);
            DEBUG("pointer to " << STREAMADDRESS(pointer) << " as " << i << " pointer ");
            AWRITE(basePointer + i * sizeof(pointer_type), data, sizeof(pointer_type));
            return true;
        }
    }
    return false;
}

void fileSystem::addPointer(inode& inode, pointer_type pointer)
{
    DEBUG("Adding pointer to " << STREAMADDRESS(pointer) << " to inode(" << inode.id << ")");
    for (size_t i = 0; i < directPointersCount; i++)
    {
        if (inode.pointers.direct[i] == 0)
        {
            inode.pointers.direct[i] = pointer;
            DEBUG("as direct" << i);
            return;
        }

    }
    for (size_t i = 0; i < indirectPointersCount; i++)
    {
        if (inode.pointers.indirect[i] == 0)
        {
            inode.pointers.indirect[i] = alocateDataBlock();
        }

        SEEKG(inode.pointers.indirect[i]);
        if (addToIndirect(pointer))
        {
            DEBUG("in indirect" << i);
            return;
        }
    }
    ERROR("Inode address pool overflow");
    assert(false);
}
bool fileSystem::addDirItem(inode& node, dirItem& item)
{
    std::vector<pointer_type>data = getDataPointers(node);
    size_t dirItemCount = sb.blockSize / sizeof(dirItem);
    for (auto dataBlock : data)
    {
        SEEKG(dataBlock);
        dirItem curent[dirItemCount];
        READ(reinterpret_cast<char*>(curent), dirItemCount * sizeof(dirItem));
        for (size_t i = 0; i < dirItemCount; i++)
        {
            if (std::strncmp(curent[i].name, item.name, fileLiteralLenght) == 0)
            {
                return false;
            }
        }
    }
    pointer_type pointer;
    int freeSpace = data.size() * sb.blockSize - node.fileSize;
    if (freeSpace >= sizeof(dirItem))
    {
        pointer = data.back();
        pointer += sb.blockSize - freeSpace;
    } else {
        node.fileSize += freeSpace;
        pointer = alocateDataBlock();
        addPointer(node, pointer);
    }
    AWRITE(pointer, reinterpret_cast<char*>(&item), sizeof(dirItem));
    //zvetceni zabrane data casti a propsani do parent inody
    node.fileSize += sizeof(dirItem);
    AWRITE(sb.inodeAddress() + node.id * sizeof(inode), reinterpret_cast<char*>(&node), sizeof(inode));
    return true;
}
bool fileSystem::removeDirItem(inode& inode, size_type removedID)
{
    dirItem empty;
    std::vector<dirItemP> dirItems = getDirItems(inode);
    int removed = -1;
    for (size_t i = 0; i < dirItems.size(); i++)
    {
        if (std::get<dirItem>(dirItems[i]).inode_id == removedID)
        {
            DEBUG("Clearing dir item with name " << std::get<dirItem>(dirItems[i]).name << " in inode" << inode.id << " on address " << STREAMADDRESS(std::get<pointer_type>(dirItems[i])));
            AWRITE(std::get<pointer_type>(dirItems[i]), reinterpret_cast<char*>(&empty), sizeof(dirItem));
            inode.fileSize -= sizeof(dirItem);
            removed = i;
            break;
        }

    }
    if (removed == -1)
    {
        return false;
    }
    if (removed + 1 == dirItems.size())
    {
        return true;
    }
    dirItem reloc;
    AREAD(std::get<pointer_type>(dirItems[dirItems.size() - 1]), reinterpret_cast<char*>(&reloc), sizeof(dirItem));
    AWRITE(std::get<pointer_type>(dirItems[dirItems.size() - 1]), reinterpret_cast<char*>(&empty), sizeof(dirItem));
    DEBUG("Relocating diritem to address :" << STREAMADDRESS(std::get<pointer_type>(dirItems[removed])) << " from on address " << STREAMADDRESS(std::get<pointer_type>(dirItems[dirItems.size() - 1])));
    AWRITE(std::get<pointer_type>(dirItems[removed]), reinterpret_cast<char*>(&reloc), sizeof(dirItem));
    return true;
}

errorCode fileSystem::touch(size_type dirID, const char fileName[maxFileNameLenght], const char data[])
{
    DEBUG("Creating file");
    inode parent, file;
    file = alocateNewInode();
    if (file.id == 0)
    {
        DEBUG("touch can not create file inode pool is full");
        return errorCode::INODE_POOL_FULL;
    }
    file.type = inode::inode_types::file;
    file.fileSize = std::strlen(data);
    AREAD(sb.inodeAddress() + dirID * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    dirItem dirItem(fileName, file.id);
    if (!addDirItem(parent, dirItem))
    {
        DEBUG("touch parrent dir can not add file");
        freeInode(file);
        return errorCode::CANNOT_CREATE_FILE;
    }
    int blockCount = file.fileSize % sb.blockSize == 0 ? file.fileSize / sb.blockSize : file.fileSize / sb.blockSize + 1;
    auto dataP = alocateDataBlocks(blockCount);
    if (dataP.size() == 0 && blockCount != 0)
    {
        DEBUG("touch can not create file not have enough free space");
        freeInode(file);
        return errorCode::CANNOT_CREATE_FILE;
    }
    for (size_t i = 0; i < blockCount; i++)
    {
        char current[sb.blockSize];
        std::memset(current, '\0', sb.blockSize);
        std::strncpy(current, data, sb.blockSize);
        AWRITE(dataP[i], current, sb.blockSize);
        data += sb.blockSize;
        addPointer(file, dataP[i]);
    }
    AWRITE(sb.inodeAddress() + file.id * sizeof(inode), reinterpret_cast<char*>(&file), sizeof(inode));
    return errorCode::OK;
}

errorCode fileSystem::cp(size_type srcInodeID, size_type destInodeID)
{
    inode src, dest;
    AREAD(sb.inodeAddress() + srcInodeID * sizeof(inode), reinterpret_cast<char*>(&src), sizeof(inode));
    if (src.type != inode::inode_types::file)
    {
        DEBUG("cp can copy only files");
        return errorCode::FILE_NOT_FOUND;
    }
    AREAD(sb.inodeAddress() + destInodeID * sizeof(inode), reinterpret_cast<char*>(&dest), sizeof(inode));
    if (dest.type != src.type)
    {
        DEBUG("cp can only to prealocated file");
        return errorCode::PATH_NOT_FOUND;
    }
    DEBUG("Coping file inode(" << srcInodeID << ") to file inode(" << destInodeID << ")");
    auto data = getDataPointers(src);
    auto nData = alocateDataBlocks(data.size());
    if (data.size() > nData.size())
    {
        FATAL("Can not copy file not have enough free space");
        return errorCode::CANNOT_CREATE_FILE;
    }
    for (size_t i = 0; i < data.size(); i++)
    {
        char current[sb.blockSize];
        AREAD(data[i], current, sb.blockSize);
        AWRITE(nData[i], current, sb.blockSize);
        addPointer(dest, nData[i]);
    }
    dest.fileSize = src.fileSize;
    dest.type = src.type;
    AWRITE(sb.inodeAddress() + dest.id * sizeof(inode), reinterpret_cast<char*>(&dest), sizeof(inode));
    return errorCode::OK;
}

errorCode fileSystem::mv(size_type parentID, size_type srcInodeID, size_type destInodeID)
{
    inode src, dest, parent;
    AREAD(sb.inodeAddress() + srcInodeID * sizeof(inode), reinterpret_cast<char*>(&src), sizeof(inode));
    if (src.type != inode::inode_types::file && src.type != inode::inode_types::dir)
    {
        DEBUG("mv can not move inode that is not file or directory");
        return errorCode::FILE_NOT_FOUND;
    }
    AREAD(sb.inodeAddress() + destInodeID * sizeof(inode), reinterpret_cast<char*>(&dest), sizeof(inode));
    if (dest.type != src.type)
    {
        DEBUG("mv can not move the incompatible inode types");
        return errorCode::PATH_NOT_FOUND;
    }
    AREAD(sb.inodeAddress() + parentID * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    if (!removeDirItem(parent, src.id))
    {
        DEBUG("mv parent can not remove the src inode");
        return errorCode::FILE_NOT_FOUND;
    }
    DEBUG("Moving file inode(" << srcInodeID << ") to file inode(" << destInodeID << ")");
    auto data = getDataPointers(src);
    dest.pointers = src.pointers;
    dest.fileSize = src.fileSize;
    dest.numHardLinks = src.numHardLinks;
    AWRITE(sb.inodeAddress() + dest.id * sizeof(inode), reinterpret_cast<char*>(&dest), sizeof(inode));
    freeInode(src);
    return errorCode::OK;
}

errorCode fileSystem::rm(size_type parentID, size_type inodeID)
{
    inode inode, parent, empty;
    AREAD(sb.inodeAddress() + inodeID * sizeof(inode), reinterpret_cast<char*>(&inode), sizeof(inode));
    AREAD(sb.inodeAddress() + parentID * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    if (inode.type != inode::inode_types::file)
    {
        DEBUG("rm can be only used on file");
        return errorCode::FILE_NOT_FOUND;
    }
    if (!removeDirItem(parent, inodeID))
    {
        return errorCode::FILE_NOT_FOUND;
    }
    DEBUG("Removing file with inode(" << inodeID << ") from dir with inode(" << parentID << ")");
    auto data = getDataPointers(inode);
    for (auto pointer : data)
    {
        freeDataBlock(pointer);
    }
    freeInode(inode);
    AWRITE(sb.inodeAddress() + inode.id * sizeof(inode), reinterpret_cast<char*>(&empty), sizeof(inode));
    return errorCode::OK;
}

errorCode fileSystem::ls(size_type inodeID, std::vector<std::string>& dirItems)
{
    inode dir;
    AREAD(sb.inodeAddress() + inodeID * sizeof(inode), reinterpret_cast<char*>(&dir), sizeof(inode));
    if (dir.type != inode::inode_types::dir)
    {
        DEBUG("ls can be only used on dirs");
        return errorCode::PATH_NOT_FOUND;
    }
    std::vector<dirItemP> dirs = getDirItems(dir);
    for (auto curent : dirs)
    {
        if (!std::strncmp(std::get<dirItem>(curent).name, "", fileLiteralLenght) == 0)
        {
            inode curentInode;
            AREAD(sb.inodeAddress() + std::get<dirItem>(curent).inode_id * sizeof(inode), reinterpret_cast<char*>(&curentInode), sizeof(inode));
            std::string qname;
            switch (curentInode.type)
            {
            case inode::inode_types::dir:
                qname.append("+");
                break;
            case inode::inode_types::file:
                qname.append("-");
                break;

            default:
                qname.append("?");
                break;
            }
            qname.append(std::get<dirItem>(curent).name);
            dirItems.push_back(qname);
        }
    }
    return fileSystem::errorCode::OK;
}


errorCode fileSystem::mkdir(const char dirName[fileLiteralLenght], size_type parentInnodeID)
{
    inode dir = alocateNewInode();
    if (dir.id == 0)
    {
        ERROR("mkdir can not create new dir inode pool is full");
        return errorCode::INODE_POOL_FULL;
    }

    inode parent;
    AREAD(sb.inodeAddress() + parentInnodeID * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));

    dirItem newDir(dirName, dir.id);
    if (!addDirItem(parent, newDir))
    {
        freeInode(dir);
        DEBUG("mkdir can not create new dir this name is already in use");
        return errorCode::EXIST;
    }

    dir.type = inode::dir;
    dirItem dirItems[2];
    dirItems[0] = dirItem(".", dir.id);
    dirItems[1] = dirItem("..", parent.id);
    dir.fileSize += 2 * sizeof(dirItem);
    addPointer(dir, alocateDataBlock());
    DEBUG("Creating directory with name " << newDir.name << " inode id " << parent.id << "/" << dir.id);
    AWRITE(sb.inodeAddress() + dir.id * sizeof(inode), reinterpret_cast<char*>(&dir), sizeof(inode));
    AWRITE(dir.pointers.direct[0], reinterpret_cast<char*>(&dirItems), 2 * sizeof(dirItem));
    return errorCode::OK;
}

errorCode fileSystem::rmdir(size_type inodeID)
{
    inode dir;
    AREAD(sb.inodeAddress() + inodeID * sizeof(inode), reinterpret_cast<char*>(&dir), sizeof(inode));
    if (dir.type != inode::inode_types::dir)
    {
        DEBUG("rmdir can be only called on dirs");
        return errorCode::PATH_NOT_FOUND;
    }
    std::vector<dirItemP> subDirs = getDirItems(dir);
    if (subDirs.size() > 2)
    {
        DEBUG("rmdir can not remove dir that is not empty");
        return errorCode::NOT_EMPTY;
    }
    inode parent;
    AREAD(sb.inodeAddress() + std::get<dirItem>(subDirs[1]).inode_id * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    bool remove = removeDirItem(parent, dir.id);
    assert(remove || "removeDirItem");
    AWRITE(sb.inodeAddress() + parent.id * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    freeDataBlock(dir.pointers.direct[0]);
    freeInode(dir);
    return errorCode::OK;
}

std::vector<pointer_type> fileSystem::getDataPointers(inode& inode)
{
    std::vector<pointer_type> pointers;
    for (size_t i = 0; i < directPointersCount; i++)
    {
        if (inode.pointers.direct[i] != 0)
        {
            pointers.push_back(inode.pointers.direct[i]);
        } else
        {
            return pointers;
        }

    }
    size_t pointerCount = sb.blockSize / sizeof(pointer_type);
    for (size_t i = 0; i < indirectPointersCount; i++)
    {
        if (inode.pointers.indirect[i] == 0)
        {
            return pointers;
        }
        pointer_type dataPointres[pointerCount];
        AREAD(inode.pointers.indirect[i], reinterpret_cast<char*>(dataPointres), pointerCount * sizeof(pointer_type));
        for (size_t i = 0; i < pointerCount; i++)
        {
            if (dataPointres[i] != 0)
            {
                pointers.push_back(dataPointres[i]);
            }
        }
    }
    FATAL("File is bigger than inode can address");
    assert(false);
    return pointers;
}
std::vector<dirItemP> fileSystem::getDirItems(inode& inode)
{
    std::vector<dirItemP> dirItems;
    if (inode.type != inode::inode_types::dir)
    {
        DEBUG("Can not read dirItems from non dir inode");
        return dirItems;
    }
    std::vector<pointer_type> data = getDataPointers(inode);
    size_t dirItemCount = sb.blockSize / sizeof(dirItem);
    for (auto dataBlock : data)
    {
        dirItem curent[dirItemCount];
        AREAD(dataBlock, reinterpret_cast<char*>(curent), dirItemCount * sizeof(dirItem));
        for (size_t i = 0; i < dirItemCount; i++)
        {
            if (std::strcmp(curent[i].name, "") != 0)
            {
                auto pair = std::make_pair(curent[i], dataBlock + i * sizeof(dirItem));
                dirItems.push_back(pair);
            }
        }
    }
    return dirItems;
}

inode fileSystem::alocateNewInode()
{
    size_t id = 0;
    for (auto it = inodeBitArray.begin();it != inodeBitArray.end();it++)
    {
        if (it.getVal(fileStream) == false)
        {
            it.flip(fileStream);
            DEBUG("Alocating inode " << id);
            return inode(id);
        }
        id++;
    }
    ERROR("Can not find any free inode");
    return inode(0);
}

void fileSystem::freeInode(inode& inode)
{
    size_type inodeId = inode.id;
    DEBUG("Free inode " << inodeId);
    (inodeBitArray.begin() += inodeId).flip(fileStream);
    char empty[sizeof(inode)];
    std::memset(empty, '\0', sizeof(inode));
    AWRITE((sb.inodeAddress() + inodeId * sizeof(inode)), empty, sizeof(inode));
}

pointer_type fileSystem::alocateDataBlock()
{
    size_type index = 0;
    for (auto it = dataBlockBitArray.begin(); it != dataBlockBitArray.end();it++, index++)
    {
        if (it.getVal(fileStream) == false)
        {
            it.flip(fileStream);
            break;
        }
    }
    if (index >= sb.blockCount)
    {
        FATAL("No free data blocks");
        exit(EXIT_FAILURE);
    }
    pointer_type pointer = sb.dataAddress() + index * sb.blockSize;
    DEBUG("Alocating Data block " << index << " on address:" << STREAMADDRESS(pointer));
#ifndef NDEBUG
    DEBUG("Zeroing Data Block " << index);
    char* zero = new char[sb.blockSize];
    std::memset(zero, '\0', sb.blockSize);
    AWRITE(pointer, zero, sb.blockSize);
    delete[] zero;
#endif
    return pointer;
}

void fileSystem::freeDataBlock(pointer_type dataPointer)
{
    DEBUG("Free data block on " << STREAMADDRESS(dataPointer));
    ((dataBlockBitArray.begin()) += (dataPointer - sb.dataAddress()) / sb.blockSize).flip(fileStream);
    char empty[sb.blockSize];
    std::memset(empty, '\0', sb.blockSize);
    AWRITE(dataPointer, empty, sb.blockSize);
}

std::vector<pointer_type> fileSystem::alocateDataBlocks(size_t numberOfDataBlocks)
{

    std::vector<size_type> blockID;
    size_t aloc = numberOfDataBlocks;

    size_type index = 0;
    for (auto it = dataBlockBitArray.begin(); it != dataBlockBitArray.end() && aloc > 0; index++, it++)
    {
        if (it.getVal(fileStream) == false)
        {
            blockID.push_back(index);
            aloc--;
        }
    }
    if (aloc > 0)
    {
        ERROR("Not enough free blocks wanted (" << numberOfDataBlocks << ") but there is only (" << numberOfDataBlocks - aloc << ") no blocks alocated");
        return std::vector<pointer_type>();
    }
    std::vector<pointer_type> pointers;
    pointers.reserve(blockID.size());
    for (size_t index : blockID)
    {
        pointers.push_back(sb.dataAddress() + index * sb.blockSize);
        (dataBlockBitArray.begin() += index).flip(fileStream);

    }
    return pointers;
}

errorCode fileSystem::calcAndFormat(size_type size)
{
    sb.diskSize = size;
    // +2 protoze maximalne si pucim 2 Byty pro bitArray
    assert(size > (sizeof(superBlock) + 2));
    size -= sizeof(superBlock) + 2;
    assert(sb.blockSize >= sizeof(dirItem) * 2);
    assert(sb.blockSize % sizeof(pointer_type) == 0);
    sb.blockCount = size / ((1 + pomerDataInode) / 8 + pomerDataInode * sizeof(inode) + sb.blockSize);
    sb.inodeCount = pomerDataInode * sb.blockCount;
    assert(sb.blockCount >= minDataBlockCount);
    assert(sb.inodeCount >= minInodeCount);
    if (!sb.setupFilePointers())
    {
        FATAL("Can not setup pointers for superblock configuration");
        return errorCode::CAN_NOT_CREATE_SUPERBLOCK;
    }
    return format();
}

errorCode fileSystem::format()
{
    fileStream.close();
    fileStream.open(fileName, std::ios::out | std::ios::in | std::ios::binary | std::ios::trunc);
    if (!fileStream.is_open())
    {
        FATAL("File " << fileName << " can not be create by program. " << strerror(errno));
        return errorCode::CANNOT_CREATE_FILE;
    }
    DEBUG("Formating");
    DEBUG("Super Block");
    AWRITE(0, reinterpret_cast<char*>(&sb), sizeof(superBlock));
    DEBUG("bit Array of Inoodes Address:" << STREAMADDRESS(sb.bitArrayInodeAddress()));
    inodeBitArray = fileBitArray(sb.bitArrayInodeAddress(), sb.inodeCount);
    DEBUG("bit Array of Inoodes Address:" << STREAMADDRESS(sb.bitArrayDataBlockAddress()));
    dataBlockBitArray = fileBitArray(sb.bitArrayDataBlockAddress(), sb.blockCount);
    DEBUG("inode data section on address:" << STREAMADDRESS(sb.inodeAddress()));
    DEBUG("data blocks section on address:" << STREAMADDRESS(sb.dataAddress()));

#ifndef NDEBUG
    DEBUG("Writing placeholder of inodes");
    SEEKP(sb.inodeAddress());
    char placeholder[sizeof(inode)];
    std::memset(placeholder, '=', sizeof(placeholder));
    placeholder[0] = '<';
    char text[] = " empty  inode ";
    std::memcpy(placeholder + sizeof(placeholder) / 2 - (sizeof(text) - 1) / 2, text, sizeof(text) - 1);
    placeholder[sizeof(placeholder) - 1] = '>';
    for (size_t i = 0; i < sb.inodeCount; i++)
    {
        WRITE(placeholder, sizeof(placeholder));
    }
    DEBUG("Writing placeholder of data blocks");
    SEEKP(sb.dataAddress());
    const size_t bsize = sb.blockSize;
    char* placeholderd = new char[bsize];
    std::memset(placeholderd, 'd', bsize);
    placeholderd[0] = '<';
    placeholderd[bsize - 1] = '>';
    for (size_t i = 0; i < sb.blockCount; i++)
    {
        WRITE(placeholderd, bsize);
    }
    delete[] placeholderd;
#else
    fileStream.seekp(sb.dataAddress() + sb.blockCount * sb.blockSize);
    fileStream.write("", 1);
#endif
    createRoot();

    return errorCode::OK;
}
