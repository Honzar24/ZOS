#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>

#include <fileSystem.hpp>
#include <dirItem.hpp>
#include <log.hpp>

/*
*   Sekce smrti pozor
*   Ano makra. Proc? Tato konstrukce je vyuzivana k debugu bez pouuziti runtime debugeru pouze za pomoci logu programu
*   Takto je mozno v logu primo videt z jake funkce je makro volano to to nevim jak by slo docilit pomoci funkce bez slozite konstrukce
*/

#define STREAMADDRESS(address) "0x" << std::setw(16) << std::left << std::hex << address << std::dec

#define SEEKG(pos)\
    assert((!fileStream.fail()) && "preSEEKG");\
    fileStream.seekg(pos);\
    assert((!fileStream.fail()) && "posSEEKG")

#define SEEKP(pos)\
    assert((!fileStream.fail()) && "preSEEKG");\
    fileStream.seekp(pos);\
    assert((!fileStream.fail()) && "posSEEKP")

#define WRITE(data,sizeBytes)\
    assert((!fileStream.fail()) && "preWRITE");\
    TRACE("Write to  address:" << STREAMADDRESS(fileStream.tellp()) << " size " << sizeBytes);\
    fileStream.write(data, sizeBytes).flush();\
    assert((!fileStream.fail())&& "posWRITE")

#define AWRITE(address,data,sizeBytes)\
    assert((!fileStream.fail()) && "preAWRITE");\
    SEEKP(address);\
    WRITE(data,sizeBytes)

#define READ(data,sizeBytes)\
    assert((!fileStream.fail()) && "preREAD");\
    TRACE("Read from address:" << STREAMADDRESS(fileStream.tellg()) << " size " << sizeBytes);\
    fileStream.read(data, sizeBytes);\
    assert((!fileStream.fail()) && "posREAD")

#define AREAD(address,data,sizeBytes)\
    assert((!fileStream.fail()) && "preAREAD");\
    SEEKG(address);\
    READ(data,sizeBytes);

/*
*   Konec sekce smrti
*/


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
    FATAL("Inode address pool overflow");
    assert(false);
}
bool fileSystem::addDirItem(inode& node, dirItem& item)
{
    std::vector<pointer_type>data;
    getDataPointers(node, data);
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

errorCode fileSystem::ls(size_type inodeID, std::vector<std::string>& dirItems)
{
    inode dir;
    AREAD(sb.inodeAddress() + inodeID * sizeof(inode), reinterpret_cast<char*>(&dir), sizeof(inode));
    if (dir.type != inode::inode_types::dir)
    {
        return errorCode::PATH_NOT_FOUND;
    }
    std::vector<pointer_type> data;
    getDataPointers(dir, data);
    size_t dirItemCount = sb.blockSize / sizeof(dirItem);
    for (auto dataBlock : data)
    {
        SEEKG(dataBlock);
        dirItem curent[dirItemCount];
        READ(reinterpret_cast<char*>(curent), dirItemCount * sizeof(dirItem));
        for (size_t i = 0; i < dirItemCount; i++)
        {
            if (!std::strncmp(curent[i].name, "", fileLiteralLenght) == 0)
            {
                inode curentInode;
                AREAD(sb.inodeAddress() + curent[i].inode_id * sizeof(inode), reinterpret_cast<char*>(&curentInode), sizeof(inode));
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
                qname.append(curent[i].name);
                dirItems.push_back(qname);
            }
        }
    }
    return fileSystem::errorCode::OK;
}


errorCode fileSystem::makeDir(const char dirName[fileLiteralLenght], size_type parentInnodeID)
{
    inode dir = alocateNewInode();
    if (dir.id == 0)
    {
        return errorCode::INODE_POOL_FULL;
    }

    inode parent;
    AREAD(sb.inodeAddress() + parentInnodeID * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));

    dirItem newDir(dirName, parent.id);
    if (!addDirItem(parent, newDir))
    {
        freeInode(dir.id);
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
void fileSystem::getDataPointers(inode& inode, std::vector<pointer_type>& pointers)
{
    for (size_t i = 0; i < directPointersCount; i++)
    {
        if (inode.pointers.direct[i] != 0)
        {
            pointers.push_back(inode.pointers.direct[i]);
        } else
        {
            return;
        }

    }
    size_t pointerCount = sb.blockSize / sizeof(pointer_type);
    for (size_t i = 0; i < indirectPointersCount; i++)
    {
        if (inode.pointers.indirect[i] == 0)
        {
            return;
        }
        pointer_type dataPointres[pointerCount];
        AREAD(inode.pointers.indirect[i],reinterpret_cast<char*>(dataPointres), pointerCount * sizeof(pointer_type));
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

void fileSystem::freeInode(size_type inodeId)
{
    DEBUG("Free inode " << inodeId);
    (inodeBitArray.begin() += inodeId).flip(fileStream);
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

size_t fileSystem::alocateDataBlocks(size_t numberOfDataBlocks, std::vector<pointer_type>& pointers)
{
    std::vector<size_type> blockID;
    auto it = dataBlockBitArray.begin();
    size_type index = 0;
    while (it != dataBlockBitArray.end() && numberOfDataBlocks > 0)
    {
        if (it.getVal(fileStream) == false)
        {
            blockID.push_back(index);
            numberOfDataBlocks--;
        }
        index++;
        it++;
    }
    if (numberOfDataBlocks > 0)
    {
        return 0;
    }
    for (size_t index : blockID)
    {
        pointers.push_back(sb.dataAddress() + index * sb.blockSize);
        (dataBlockBitArray.begin() += index).flip(fileStream);

    }
    return blockID.size();
}

errorCode fileSystem::calcAndFormat(size_type size)
{
    sb.diskSize = size;
    // +2 protoze maximalne si pucim 2 Byty pro bitArray
    assert(size > (sizeof(superBlock) + 2));
    size -= sizeof(superBlock) + 2;
    sb.blockCount = size / ((1 + pomerDataInode) / 8 + pomerDataInode * sizeof(inode) + sb.blockSize);
    sb.inodeCount = pomerDataInode * sb.blockCount;
    assert(sb.blockCount >= minDataBlockCount);
    assert(sb.inodeCount >= minInodeCount);
    if (!sb.setupFilePointers())
    {
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
    // superBlock
    AWRITE(0, reinterpret_cast<char*>(&sb), sizeof(superBlock));
    // inode bitArray
    inodeBitArray = fileBitArray(sb.bitArrayInodeAddress(), sb.inodeCount);
    // data blocks bitArray    
    dataBlockBitArray = fileBitArray(sb.bitArrayDataBlockAddress(), sb.blockCount);

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
    // zabrani vyuzitelne pameti
    fileStream.seekp(sb.dataAddress() + sb.blockCount * sb.blockSize);
    fileStream.write("", 1);
#endif
    createRoot();

    return errorCode::OK;
}
