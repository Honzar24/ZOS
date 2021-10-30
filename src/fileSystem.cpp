#include <cstdlib>
#include <cstdint>
#include <cstring>

#include <fileSystem.hpp>
#include <dir_item.hpp>


fileSystem::fileSystem(std::string& fileName, superBlock& sb) : fileName(fileName), sb(sb), fileStream(fileName)
{
    if (sb.diskSize == 0)
    {
        sb.setupFilePointers();
        format();
    } else
    {
        calcAndFormat(sb.diskSize);
    }
    inodeBitArray = fileBitArray(sb.bitArrayInodeAddress(), sb.inodeCount);
    dataBlockBitArray = fileBitArray(sb.bitArrayDataBlockAddress(), sb.blockCount);
}
fileSystem::fileSystem(std::string& fileName) : fileName(fileName), fileStream(fileName, std::ios::out | std::ios::in | std::ios::binary)
{
#ifndef NDEBUG
    if (!fileStream.is_open())
    {
        std::cerr << "WARN:File " << fileName << " can not be open by program. " << strerror(errno) << std::endl;
    }
#endif
    fileStream.seekg(0).read(reinterpret_cast<char*>(&sb), sizeof(superBlock));
}
void addDataPointer(inode& inode, pointer_type pointer)
{
    for (size_t index = 0; index < directPointersCount; index++)
    {
        if (inode.pointers.direct[index] == 0)
        {
            inode.pointers.direct[index] = pointer;
            return;
        }

    }
    assert(false);

}

void fileSystem::createRoot()
{
    auto root = alocateNewInode();
    root.type = inode::dir;
    dirItem dirItems[2];
    dirItems[0] = dirItem("..", root.id);
    dirItems[1] = dirItem(".", root.id);
    root.fileSize += 2 * sizeof(dirItem);
    assert(sb.blockSize >= 2 * sizeof(dirItem));
    root.pointers.direct[0] = alocateDataBlock();
    fileStream.seekp(sb.inodeAddress() + root.id * sizeof(inode));
    fileStream.write(reinterpret_cast<char*>(&root), sizeof(root));
    fileStream.seekp(root.pointers.direct[0]);
    fileStream.write(reinterpret_cast<char*>(dirItems), sizeof(dirItems));
}

errorCode fileSystem::makeDir(const char dirName[fileLiteralLenght], size_type parentInnodeID)
{
    inode parent;
    fileStream.seekg(sb.inodeAddress() + parentInnodeID * sizeof(inode)).read(reinterpret_cast<char*>(&parent), sizeof(inode));

    std::vector<pointer_type>data;
    getDataPointers(parent, data);
    std::vector<dirItem> contains;
    for (auto dataBlock : data)
    {
        fileStream.seekg(dataBlock);
        size_t blockSize = sb.blockSize;
        while (blockSize >= sizeof(dirItem))
        {
            dirItem curent;
            fileStream.read(reinterpret_cast<char*>(&curent), sizeof(dirItem));
            blockSize -= sizeof(dirItem);
            if (std::strncmp(curent.name, dirName, fileLiteralLenght) == 0)
            {
                return errorCode::EXIST;
            }
        }
    }
    pointer_type pointer;
    int freeSpace = sb.blockSize - parent.fileSize;
    if (freeSpace >= sizeof(dirItem))
    {
        pointer = data.back();
        pointer += parent.fileSize;
    } else {
        parent.fileSize += freeSpace;
        pointer = alocateDataBlock();
    }

    inode dir = alocateNewInode();
    if (dir.id == 0)
    {
        return errorCode::INODE_POOL_FULL;
    }
    dir.type = inode::dir;
    dirItem dirItems[2];
    dirItems[0] = dirItem("..", parent.id);
    dirItems[1] = dirItem(".", dir.id);
    dir.fileSize += 2 * sizeof(dirItem);
    dir.pointers.direct[0] = alocateDataBlock();

    dirItem newDir(dirName, parent.id);
    fileStream.seekp(pointer).write(reinterpret_cast<char*>(&newDir), sizeof(dirItem));
    parent.fileSize += sizeof(dirItem);

    fileStream.seekp(sb.inodeAddress() + dir.id * sizeof(inode)).write(reinterpret_cast<char*>(&dir), sizeof(inode));
    fileStream.seekp(sb.inodeAddress() + parent.id * sizeof(inode)).write(reinterpret_cast<char*>(&parent), sizeof(inode));

    fileStream.seekp(dir.pointers.direct[0]).write(reinterpret_cast<char*>(&dirItems), 2 * sizeof(dirItem));

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
            break;
        }
        id++;
    }
    return inode(id < sb.inodeCount ? id : 0);
}

void fileSystem::freeInode(size_type inodeId)
{
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
    return sb.dataAddress() + index * sb.blockSize;
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
    #ifndef NDEBUG
        std::cerr << "FATAL:File " << fileName << " can not be create by program. " << strerror(errno) << std::endl;
    #endif
        return errorCode::CANNOT_CREATE_FILE;
    }
    // superBlock
    fileStream.write(reinterpret_cast<char*>(&sb), sizeof(superBlock));
    // inode bitArray
    inodeBitArray = fileBitArray(sb.bitArrayInodeAddress(), sb.inodeCount);
    // data blocks bitArray    
    dataBlockBitArray = fileBitArray(sb.bitArrayDataBlockAddress(), sb.blockCount);

#ifndef NDEBUG
    // inode section
    fileStream.seekp(sb.inodeAddress());
    char placeholder[sizeof(inode)];
    std::memset(placeholder, '=', sizeof(placeholder));
    placeholder[0] = '<';
    char text[] = " empty  inode ";
    std::memcpy(placeholder + sizeof(placeholder) / 2 - (sizeof(text) - 1) / 2, text, sizeof(text) - 1);
    placeholder[sizeof(placeholder) - 1] = '>';
    for (size_t i = 0; i < sb.inodeCount; i++)
    {
        fileStream.write(placeholder, sizeof(placeholder));
    }
    // data section
    fileStream.seekp(sb.dataAddress());
    const size_t bsize = sb.blockSize;
    char* placeholderd = new char[bsize];
    std::memset(placeholderd, 'd', bsize);
    placeholderd[0] = '<';
    placeholderd[bsize - 1] = '>';
    for (size_t i = 0; i < sb.blockCount; i++)
    {
        fileStream.write(placeholderd, bsize);
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
