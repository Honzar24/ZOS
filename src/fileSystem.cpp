#include <cstdlib>
#include <cstdint>
#include <cstring>

#include <fileSystem.hpp>
#include <dir_item.hpp>

using errorCode = fileSystem::errorCode;

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
    size_t blockCount = root.fileSize % sb.blockSize == 0 ? root.fileSize / sb.blockCount : root.fileSize / sb.blockCount + 1;
    std::vector<pointer_type> pointers;
    assert(alocateDataBlocks(blockCount, pointers) == blockCount);
    root.pointers.direct[0] = pointers[0];
    fileStream.seekp(sb.inodeAddress() + root.id * sizeof(inode));
    fileStream.write(reinterpret_cast<char*>(&root), sizeof(root));
    fileStream.seekp(root.pointers.direct[0]);
    fileStream.write(reinterpret_cast<char*>(dirItems), sizeof(dirItems));
}

errorCode fileSystem::makeDir(char dirName[fileLiteralLenght], size_type parentInnodeID)
{

    return errorCode::OK;
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
    return inode(id <= sb.inodeCount ? id : 0);
}

void fileSystem::freeInode(size_type inodeId)
{
    (inodeBitArray.begin()+=inodeId).flip(fileStream);
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
