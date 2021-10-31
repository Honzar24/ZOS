#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>

#include <fileSystem.hpp>
#include <dir_item.hpp>
#include <log.hpp>


#define ADDRESSOFSET std::setw(16) << std::right

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
    TRACE("Write to  address:" << ADDRESSOFSET << fileStream.tellp()  << " size " << sizeBytes);\
    fileStream.write(data, sizeBytes).flush();\
    assert((!fileStream.fail())&& "posWRITE")

#define AWRITE(address,data,sizeBytes)\
    assert((!fileStream.fail()) && "preAWRITE");\
    SEEKP(address);\
    WRITE(data,sizeBytes)

#define READ(data,sizeBytes)\
    assert((!fileStream.fail()) && "preREAD");\
    TRACE("Read from address:" << ADDRESSOFSET << fileStream.tellg() << " size " << sizeBytes);\
    fileStream.read(data, sizeBytes);\
    assert((!fileStream.fail()) && "posREAD")

#define AREAD(address,data,sizeBytes)\
    assert((!fileStream.fail()) && "preAREAD");\
    SEEKG(address);\
    READ(data,sizeBytes);


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
#ifndef NDEBUG
    if (!fileStream.is_open())
    {
        INFO("File " << fileName << " can not be open by program. " << strerror(errno));
    }
#endif
    AREAD(0, reinterpret_cast<char*>(&sb), sizeof(superBlock));
}
void fileSystem::createRoot()
{
    TRACE("Creating root");
    auto root = alocateNewInode();
    root.type = inode::dir;
    dirItem dirItems[2];
    dirItems[0] = dirItem("..", root.id);
    dirItems[1] = dirItem(".", root.id);
    root.fileSize += 2 * sizeof(dirItem);
    addPointer(root, alocateDataBlock());
    AWRITE(sb.inodeAddress() + root.id * sizeof(inode), reinterpret_cast<char*>(&root), sizeof(root));
    AWRITE(root.pointers.direct[0], reinterpret_cast<char*>(dirItems), sizeof(dirItems));
}
bool fileSystem::addToIndirect(pointer_type pointer)
{
    for (size_t i = 0; i < sb.blockSize / sizeof(pointer_type); i++)
    {
        pointer_type curppointer = 0;
        READ(reinterpret_cast<char*>(&curppointer), sizeof(pointer_type));
        if (curppointer == 0)
        {
            int pos = fileStream.tellg();
            pos -= sizeof(pointer_type);
            auto data = reinterpret_cast<char*>(&pointer);
            DEBUG("pointer " << pointer << " as " << i << " indirect pointer ");
            AWRITE(pos, data, sizeof(pointer_type));
            return true;
        }
    }
    return false;
}

void fileSystem::addPointer(inode& inode, pointer_type pointer)
{
    DEBUG("Adding pointer " << pointer << " to inode(" << inode.id << ")");
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
    assert(false && "preteceni adresovatelne pameti inodu");
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

    std::vector<pointer_type>data;
    getDataPointers(parent, data);

    for (auto dataBlock : data)
    {
        SEEKG(dataBlock);
        size_t blockSize = sb.blockSize;
        while (blockSize >= sizeof(dirItem))
        {
            dirItem curent;
            READ(reinterpret_cast<char*>(&curent), sizeof(dirItem));
            blockSize -= sizeof(dirItem);
            if (std::strncmp(curent.name, dirName, fileLiteralLenght) == 0)
            {
                return errorCode::EXIST;
            }
        }
    }

    pointer_type pointer;
    int freeSpace = data.size() * sb.blockSize - parent.fileSize;
    if (freeSpace >= sizeof(dirItem))
    {
        pointer = data.back();
        pointer += sb.blockSize - freeSpace;
    } else {
        parent.fileSize += freeSpace;
        pointer = alocateDataBlock();
        addPointer(parent, pointer);
    }
    //pridani pod adresare do data sekce parent inode
    dirItem newDir(dirName, parent.id);
    AWRITE(pointer, reinterpret_cast<char*>(&newDir), sizeof(dirItem));
    //zvetceni zabrane data casti a propsani do parent inody
    parent.fileSize += sizeof(dirItem);
    AWRITE(sb.inodeAddress() + parent.id * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    //vytvoreni odkazu parent self
    dir.type = inode::dir;
    dirItem dirItems[2];
    dirItems[0] = dirItem("..", parent.id);
    dirItems[1] = dirItem(".", dir.id);
    dir.fileSize += 2 * sizeof(dirItem);
    addPointer(dir, alocateDataBlock());
    AWRITE(sb.inodeAddress() + dir.id * sizeof(inode), reinterpret_cast<char*>(&dir), sizeof(inode));
    AWRITE(dir.pointers.direct[0], reinterpret_cast<char*>(&dirItems), 2 * sizeof(dirItem));
    return errorCode::OK;
}
void fileSystem::getDataPointers(inode inode, std::vector<pointer_type>& pointers)
{
    int size = inode.fileSize;
    for (size_t i = 0; i < directPointersCount && size > 0; i++)
    {
        pointers.push_back(inode.pointers.direct[i]);
        size -= sb.blockSize;
    }
    if (size <= 0)
    {
        return;
    }
    for (size_t i = 0; i < indirectPointersCount; i++)
    {
        SEEKG(inode.pointers.indirect[i]);
        size_type read = 0;
        while (read + sizeof(pointer_type) <= sb.blockSize && size > 0)
        {
            pointer_type dataPointer;
            READ(reinterpret_cast<char*>(&dataPointer), sizeof(pointer_type));
            read += sizeof(pointer_type);
            pointers.push_back(dataPointer);
            size -= sb.blockSize;
        }
        if (size <= 0)
        {
            return;
        }
    }
    //preteceni velikosti
    assert(false && "preteceni velikosti souboru");
}

inode fileSystem::alocateNewInode()
{
    size_t id = 0;
    for (auto it = inodeBitArray.begin();it != inodeBitArray.end();it++)
    {
        if (it.getVal(fileStream) == false)
        {
            it.flip(fileStream);
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
    assert(index < sb.blockCount);
    pointer_type pointer = sb.dataAddress() + index * sb.blockSize;
    DEBUG("Alocating Data block " << index << " on address:" << pointer);
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
    // +2 protoze maximalne si pucim 2 Byty pro ByteArray
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
    // superBlock
    AWRITE(0, reinterpret_cast<char*>(&sb), sizeof(superBlock));
    // inode bitArray
    inodeBitArray = fileBitArray(sb.bitArrayInodeAddress(), sb.inodeCount);
    // data blocks bitArray    
    dataBlockBitArray = fileBitArray(sb.bitArrayDataBlockAddress(), sb.blockCount);

#ifndef NDEBUG
    // inode section
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
    // data section
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
