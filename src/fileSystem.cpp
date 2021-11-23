#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <sstream>

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
    assert((!fileStream.fail()) && "preWRITE");TRACE("Write to  address:" << STREAMADDRESS(fileStream.tellp()) << " size " << sizeBytes);fileStream.write(data, sizeBytes);assert((!fileStream.fail())&& "posWRITE")

#define AWRITE(address,data,sizeBytes)\
    assert((!fileStream.fail()) && "preAWRITE");SEEKP(address);WRITE(data,sizeBytes)

#define READ(data,sizeBytes)\
    fileStream.flush(); assert((!fileStream.fail()) && "preREAD");TRACE("Read from address:" << STREAMADDRESS(fileStream.tellg()) << " size " << sizeBytes);fileStream.read(data, sizeBytes);assert((!fileStream.fail()) && "posREAD")

#define AREAD(address,data,sizeBytes)\
    assert((!fileStream.fail()) && "preAREAD");SEEKG(address);READ(data,sizeBytes);

/*
*   Konec sekce smrti
*/

using dirItemP = std::pair<dirItem, pointer_type>;


fileSystem::fileSystem(std::string& fileName, superBlock& sb) : fileName(fileName), fileStream(fileName), sb(sb)
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
    inodeBitArray = fileBitArray(sb.bitArrayInodeAddress(), sb.inodeCount);
    dataBlockBitArray = fileBitArray(sb.bitArrayDataBlockAddress(), sb.blockCount);
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
    pointer_type* pointerBlock = new pointer_type[pointerCount];
    size_t basePointer = fileStream.tellg();
    READ(reinterpret_cast<char*>(pointerBlock), pointerCount * sizeof(pointer_type));
    for (size_t i = 0; i < pointerCount; i++)
    {
        if (pointerBlock[i] == 0)
        {
            auto data = reinterpret_cast<char*>(&pointer);
            DEBUG("pointer to " << STREAMADDRESS(pointer) << " as " << i << " pointer ");
            AWRITE(basePointer + i * sizeof(pointer_type), data, sizeof(pointer_type));
            delete[] pointerBlock;
            return true;
        }
    }
    delete[] pointerBlock;
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
    for (size_t i = 0; i < indirect1PointersCount; i++)
    {
        if (inode.pointers.indirect1[i] == 0)
        {
            inode.pointers.indirect1[i] = alocateDataBlock();
        }

        SEEKG(inode.pointers.indirect1[i]);
        if (addToIndirect(pointer))
        {
            DEBUG("in indirect1 " << i);
            return;
        }
    }
    size_t pointerCount = sb.blockSize / sizeof(pointer_type);
    pointer_type* pointerBlock = new pointer_type[pointerCount];
    for (size_t i = 0; i < indirect2PointersCount; i++)
    {
        if (inode.pointers.indirect2[i] == 0)
        {
            inode.pointers.indirect2[i] = alocateDataBlock();
        }
        AREAD(inode.pointers.indirect2[i], reinterpret_cast<char*>(pointerBlock), pointerCount * sizeof(pointer_type));
        for (size_t j = 0; j < pointerCount; j++)
        {
            if (pointerBlock[j] == 0)
            {
                pointerBlock[j] = alocateDataBlock();
                AWRITE(inode.pointers.indirect2[i] + j * sizeof(pointer_type), reinterpret_cast<char*>(&(pointerBlock[j])), sizeof(pointer_type));
            }
            SEEKG(pointerBlock[j]);
            if (addToIndirect(pointer))
            {
                DEBUG("in indirect2 " << j);
                delete[] pointerBlock;
                return;
            }
        }
    }
    delete[] pointerBlock;
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
        dirItem* curent = new dirItem[dirItemCount];
        READ(reinterpret_cast<char*>(curent), dirItemCount * sizeof(dirItem));
        for (size_t i = 0; i < dirItemCount; i++)
        {
            if (std::strncmp(curent[i].name, item.name, fileLiteralLenght) == 0)
            {
                return false;
            }
        }
        delete[] curent;
    }
    pointer_type pointer;
    size_t freeSpace = data.size() * sb.blockSize - node.fileSize;
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
bool fileSystem::removeDirItem(inode& inode, dirItem& item)
{
    dirItem empty;
    std::vector<dirItemP> dirItems = getDirItems(inode);
    int removed = -1;
    for (size_t i = 0; i < dirItems.size(); i++)
    {
        auto current = std::get<dirItem>(dirItems[i]);
        if (current.inode_id == item.inode_id && std::strcmp(current.name, item.name) == 0)
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
    if (removed + 1 == (int)dirItems.size())
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

std::pair<std::unique_ptr<char[]>, size_t> fileSystem::getData(size_type file)
{
    inode inode;
    AREAD(sb.inodeAddress() + file * sizeof(inode), reinterpret_cast<char*>(&inode), sizeof(inode));
    if (inode.type != inode::inode_types::file)
    {
        DEBUG("Can not get data from non file");
        return std::make_pair(nullptr, 0);
    }
    size_t fileSize = inode.fileSize;
    auto data = new char[fileSize];
    char* cdata = data;
    auto pointers = getDataPointers(inode);
    for (size_t i = 0; i < pointers.size() - 1; i++, cdata += sb.blockSize, fileSize -= sb.blockSize)
    {
        AREAD(pointers[i], cdata, sb.blockSize);
    }
    AREAD(pointers[pointers.size() - 1], cdata, fileSize);
    return std::make_pair(std::unique_ptr<char[]>(data), inode.fileSize);
}

std::pair<errorCode, size_type> fileSystem::touch(size_type dirID, const char fileName[maxFileNameLenght], const char data[], const size_t fileSize)
{
    inode parent;
    std::string filename(fileName);
    AREAD(sb.inodeAddress() + dirID * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    if (parent.type != inode::inode_types::dir)
    {
        DEBUG("touch can not create file not under directory");
        return std::make_pair(errorCode::PATH_NOT_FOUND, 0);
    }
    auto ret = create(parent, filename, data, fileSize);
    return std::make_pair(ret.first, ret.second.id);
}

std::pair<errorCode, inode> fileSystem::create(inode& dir, std::string& fileName, const char* data, const size_t fileSize)
{
    DEBUG("Creating file");
    inode file;
    file = alocateNewInode();
    if (file.id == 0)
    {
        DEBUG("create can not create file inode pool is full");
        return std::make_pair(errorCode::INODE_POOL_FULL, std::move(file));
    }
    file.type = inode::inode_types::file;
    file.fileSize = fileSize;
    dirItem dirItem(fileName.c_str(), file.id);
    if (!addDirItem(dir, dirItem))
    {
        DEBUG("create parrent dir can not add file");
        freeInode(file);
        return std::make_pair(errorCode::EXIST, std::move(file));
    }
    size_t blockCount = file.fileSize % sb.blockSize == 0 ? file.fileSize / sb.blockSize : file.fileSize / sb.blockSize + 1;
    auto dataP = alocateDataBlocks(blockCount);
    if (dataP.size() == 0 && blockCount != 0)
    {
        DEBUG("create can not create file not have enough free space");
        freeInode(file);
        return std::make_pair(errorCode::CAN_NOT_CREATE_FILE, std::move(file));
    }
    for (size_t i = 0; i < blockCount; i++)
    {
        char* current = new char[sb.blockSize];
        std::memset(current, '\0', sb.blockSize);
        std::memcpy(current, data, sb.blockSize);
        AWRITE(dataP[i], current, sb.blockSize);
        data += sb.blockSize;
        addPointer(file, dataP[i]);
        delete[] current;
    }
    AWRITE(sb.inodeAddress() + file.id * sizeof(inode), reinterpret_cast<char*>(&file), sizeof(inode));
    return std::make_pair(errorCode::OK, std::move(file));
}

errorCode fileSystem::cp(size_type parentID, c_file_name_t srcItemName, size_type destInodeID, c_file_name_t destName)
{
    inode src, parent, destParent;
    AREAD(sb.inodeAddress() + parentID * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    if (parent.type != inode::inode_types::dir)
    {
        DEBUG("cp parent directory is not presend");
        return errorCode::PATH_NOT_FOUND;
    }
    dirItem srcItem, destItem;
    for (dirItemP dirItemPair : getValidDirItems(parent))
    {
        srcItem = std::get<dirItem>(dirItemPair);
        if (std::strcmp(srcItem.name, srcItemName) == 0)
        {
            break;
        }
    }
    AREAD(sb.inodeAddress() + srcItem.inode_id * sizeof(inode), reinterpret_cast<char*>(&src), sizeof(inode));
    if (src.type != inode::inode_types::file)
    {
        DEBUG("cp can copy only files");
        return errorCode::FILE_NOT_FOUND;
    }
    AREAD(sb.inodeAddress() + destInodeID * sizeof(inode), reinterpret_cast<char*>(&destParent), sizeof(inode));
    if (destParent.type != inode::inode_types::dir)
    {
        DEBUG("cp parent dir is not found");
        return errorCode::PATH_NOT_FOUND;
    }
    std::string destname(destName);
    auto destRet = create(destParent, destname, "", 0);
    if (std::get<errorCode>(destRet) != errorCode::OK)
    {
        return std::get<errorCode>(destRet);
    }
    return cp(src, std::get<inode>(destRet));
}

errorCode fileSystem::mv(size_type parentID, c_file_name_t srcItemName, size_type destInodeID, c_file_name_t destName)
{
    inode src, parent, destParent;
    AREAD(sb.inodeAddress() + parentID * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    if (parent.type != inode::inode_types::dir)
    {
        DEBUG("mv parent directory is not found");
        return errorCode::PATH_NOT_FOUND;
    }
    dirItem srcItem, destItem;
    for (dirItemP dirItemPair : getValidDirItems(parent))
    {
        srcItem = std::get<dirItem>(dirItemPair);
        if (std::strcmp(srcItem.name, srcItemName) == 0)
        {
            break;
        }
    }
    AREAD(sb.inodeAddress() + srcItem.inode_id * sizeof(inode), reinterpret_cast<char*>(&src), sizeof(inode));
    if (src.type != inode::inode_types::file)
    {
        DEBUG("mv can copy only files");
        return errorCode::FILE_NOT_FOUND;
    }
    AREAD(sb.inodeAddress() + destInodeID * sizeof(inode), reinterpret_cast<char*>(&destParent), sizeof(inode));
    if (destParent.type != inode::inode_types::dir)
    {
        DEBUG("mv parent dir is not found");
        return errorCode::PATH_NOT_FOUND;
    }
    std::string destname(destName);
    auto destRet = create(destParent, destname, "", 0);
    if (std::get<errorCode>(destRet) != errorCode::OK)
    {
        return std::get<errorCode>(destRet);
    }
#ifndef NLOG
    if (src.numHardLinks > 0)
    {
        INFO("mv moving hard pointed inode making all hard link invalid");
    }
#endif
    return mv(parent, src, srcItem, std::get<inode>(destRet));
}

errorCode fileSystem::rm(size_type parentID, c_file_name_t itemName)
{
    inode inode, parent;
    dirItem item;
    AREAD(sb.inodeAddress() + parentID * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    for (auto itemp : getValidDirItems(parent))
    {
        item = std::get<dirItem>(itemp);
        if (std::strcmp(item.name, itemName) == 0)
        {
            break;
        }
    }
    AREAD(sb.inodeAddress() + item.inode_id * sizeof(inode), reinterpret_cast<char*>(&inode), sizeof(inode));
    if (inode.type != inode::inode_types::file)
    {
        DEBUG("rm can be only used on file");
        return errorCode::FILE_NOT_FOUND;
    }
    return rm(parent, inode, item);
}

errorCode fileSystem::cp(inode& src, inode& dest)
{
    DEBUG("Coping file inode(" << src.id << ") to file inode(" << dest.id << ")");
    auto data = getDataPointers(src);
    auto nData = alocateDataBlocks(data.size());
    if (data.size() > nData.size())
    {
        FATAL("Can not copy file not have enough free space");
        freeInode(dest);
        return errorCode::CAN_NOT_CREATE_FILE;
    }
    assert(dest.fileSize == 0);
    for (size_t i = 0; i < data.size(); i++)
    {
        char* current = new char[sb.blockSize];
        AREAD(data[i], current, sb.blockSize);
        AWRITE(nData[i], current, sb.blockSize);
        addPointer(dest, nData[i]);
        delete[] current;
    }
    dest.fileSize = src.fileSize;
    dest.type = src.type;
    AWRITE(sb.inodeAddress() + dest.id * sizeof(inode), reinterpret_cast<char*>(&dest), sizeof(inode));
    return errorCode::OK;
}

errorCode fileSystem::mv(inode& parent, inode& src, dirItem& srcItem, inode& dest)
{
    DEBUG("Moving file(" << srcItem.name << ") inode(" << srcItem.inode_id << ") to file inode(" << dest.id << ")");
    if (!removeDirItem(parent, srcItem))
    {
        DEBUG("mv parent can not remove the src");
        return errorCode::FILE_NOT_FOUND;
    }
    auto data = getDataPointers(src);
    dest.pointers = src.pointers;
    dest.fileSize = src.fileSize;
    dest.numHardLinks = 0;
    AWRITE(sb.inodeAddress() + dest.id * sizeof(inode), reinterpret_cast<char*>(&dest), sizeof(inode));
    freeInode(src);
    return errorCode::OK;
}

errorCode fileSystem::rm(inode& parent, inode& node, dirItem& item)
{

    if (node.type != inode::inode_types::file)
    {
        DEBUG("rm can be only used on file");
        return errorCode::FILE_NOT_FOUND;
    }
    if (!removeDirItem(parent, item))
    {
        return errorCode::FILE_NOT_FOUND;
    }
    DEBUG("rm file(" << item.name << ") with inode(" << item.inode_id << ") from dir with inode(" << parent.id << ")");
    if (node.numHardLinks != 0)
    {
        DEBUG("to this data still points some pointers only decrement pointer count");
        node.numHardLinks--;
        AWRITE(sb.inodeAddress() + node.id * sizeof(inode), reinterpret_cast<char*>(&node), sizeof(inode));
        return fileSystem::errorCode::OK;
    }
    DEBUG("Last pointer to this file removing data");
    auto data = getDataPointers(node);
    for (auto pointer : data)
    {
        freeDataBlock(pointer);
    }
    for (size_t i = 0; i < indirect1PointersCount; i++)
    {
        freeDataBlock(node.pointers.indirect1[i]);
    }
    freeInode(node);
    inode empty;
    AWRITE(sb.inodeAddress() + node.id * sizeof(inode), reinterpret_cast<char*>(&empty), sizeof(inode));
    return errorCode::OK;
}

error_string_pair fileSystem::ls(size_type inodeID)
{
    inode dir;
    AREAD(sb.inodeAddress() + inodeID * sizeof(inode), reinterpret_cast<char*>(&dir), sizeof(inode));
    if (dir.type != inode::inode_types::dir)
    {
        DEBUG("ls can be only used on dirs");
        return std::make_pair(errorCode::PATH_NOT_FOUND, "");
    }
    std::stringstream out;
    std::vector<dirItemP> dirs = getDirItems(dir);
    for (auto curent : dirs)
    {
        if (!std::strncmp(std::get<dirItem>(curent).name, "", fileLiteralLenght) == 0)
        {
            inode curentInode;
            AREAD(sb.inodeAddress() + std::get<dirItem>(curent).inode_id * sizeof(inode), reinterpret_cast<char*>(&curentInode), sizeof(inode));
            switch (curentInode.type)
            {
            case inode::inode_types::dir:
                out << '+';
                break;
            case inode::inode_types::file:
                out << '-';
                break;

            default:
                out << '?';
                break;
            }
            out << std::get<dirItem>(curent).name << std::endl;
        }
    }
    return std::make_pair(fileSystem::errorCode::OK, out.str());
}

error_string_pair fileSystem::info(size_type parentID, c_file_name_t name)
{
    inode parent, inode;
    AREAD(sb.inodeAddress() + parentID * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    if (parent.type != inode::inode_types::dir)
    {
        DEBUG("info can not find parent of inode");
        return std::make_pair(errorCode::FILE_NOT_FOUND, std::string());
    }
    auto dirItems = getValidDirItems(parent);
    bool found = false;
    dirItem item;
    file_name_t sName;
    std::strcpy(sName, name);
    if (std::strcmp("", name) == 0)
    {
        std::strcpy(sName, ".");
    }
    for (auto i : dirItems)
    {
        item = std::get<dirItem>(i);
        if (std::strcmp(item.name, sName) == 0)
        {
            AREAD(sb.inodeAddress() + item.inode_id * sizeof(inode), reinterpret_cast<char*>(&inode), sizeof(inode));
            found = true;
            break;
        }
    }
    if (!found)
    {
        DEBUG("info can not found dir item with name: " << name);
        return std::make_pair(errorCode::FILE_NOT_FOUND, std::string());
    }
    std::stringstream out;
    if (inode.id == 0)
    {
        name = "/";
    }
    out << name << " - ";
    out << inode.fileSize << " - ";
    out << inode.id << " - ";
    auto pointers = getDataPointers(inode);
    out << pointers.size() << " data blocks on addresses[";
    auto it = pointers.begin();
    if (it != pointers.end())
    {
        out << STREAMADDRESS(*it);
        it++;
    }
    for (; it != pointers.end();it++)
    {
        out << "," << STREAMADDRESS(*it);
    }
    out << "]";
    return std::make_pair(errorCode::OK, out.str());
}

errorCode fileSystem::ln(size_type srcID, size_type destDirID, const c_file_name_t fileName)
{
    inode parent, src;
    AREAD(sb.inodeAddress() + destDirID * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    if (parent.type != inode::inode_types::dir)
    {
        DEBUG("ln can not add link to not dir inode");
        return errorCode::CAN_NOT_CREATE_FILE;
    }
    AREAD(sb.inodeAddress() + srcID * sizeof(inode), reinterpret_cast<char*>(&src), sizeof(inode));
    if (src.type != inode::inode_types::file)
    {
        DEBUG("ln can not add link to not dir inode");
        return errorCode::CAN_NOT_CREATE_FILE;
    }
    dirItem linkItem(fileName, srcID);
    bool added = addDirItem(parent, linkItem);
    assert(added);
    src.numHardLinks++;
    AWRITE(sb.inodeAddress() + srcID * sizeof(inode), reinterpret_cast<char*>(&src), sizeof(inode));
    return errorCode::OK;
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

errorCode fileSystem::rmdir(size_type dirID, c_file_name_t dirName)
{
    inode dir;
    AREAD(sb.inodeAddress() + dirID * sizeof(inode), reinterpret_cast<char*>(&dir), sizeof(inode));
    if (dir.type != inode::inode_types::dir)
    {
        DEBUG("rmdir can be only called on dirs");
        return errorCode::PATH_NOT_FOUND;
    }
    std::vector<dirItemP> subDirs = getValidDirItems(dir);
    if (subDirs.size() > 2)
    {
        DEBUG("rmdir can not remove dir that is not empty");
        return errorCode::NOT_EMPTY;
    }
    inode parent;
    dirItem selfitem;
    std::strcpy(selfitem.name, dirName);
    selfitem.inode_id = dirID;
    AREAD(sb.inodeAddress() + std::get<dirItem>(subDirs[1]).inode_id * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    bool remove = removeDirItem(parent, selfitem);
    assert(remove || "removeDirItem");
    AWRITE(sb.inodeAddress() + parent.id * sizeof(inode), reinterpret_cast<char*>(&parent), sizeof(inode));
    freeDataBlock(dir.pointers.direct[0]);
    freeInode(dir);
    return errorCode::OK;
}

std::vector<Dirent> fileSystem::readDir(size_type dirID)
{
    std::vector<Dirent> dirents;
    inode dir;
    AREAD(sb.inodeAddress() + dirID * sizeof(inode), reinterpret_cast<char*>(&dir), sizeof(inode));
    if (dir.type != inode::inode_types::dir)
    {
        DEBUG("readDir can be only called on dirs");
        return dirents;
    }
    for (auto dirItemP : getValidDirItems(dir))
    {
        dirItem di = std::get<dirItem>(dirItemP);
        inode inode;
        AREAD(sb.inodeAddress() + di.inode_id * sizeof(inode), reinterpret_cast<char*>(&inode), sizeof(inode));
        Dirent item{ di.inode_id,inode.type,"?" };
        std::strncpy(item.name, di.name, fileLiteralLenght);
        dirents.emplace_back(item);
    }
    return dirents;
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
    pointer_type* dataPointres = new pointer_type[pointerCount];
    for (size_t i = 0; i < indirect1PointersCount; i++)
    {
        if (inode.pointers.indirect1[i] == 0)
        {
            return pointers;
        }

        AREAD(inode.pointers.indirect1[i], reinterpret_cast<char*>(dataPointres), pointerCount * sizeof(pointer_type));
        for (size_t j = 0; j < pointerCount; j++)
        {
            if (dataPointres[j] != 0)
            {
                pointers.push_back(dataPointres[j]);
            }
        }
    }
    pointer_type* dataPointersPointres = new pointer_type[pointerCount];
    for (size_t i = 0; i < indirect2PointersCount; i++)
    {
        if (inode.pointers.indirect2[i] == 0)
        {
            return pointers;
        }
        AREAD(inode.pointers.indirect2[i], reinterpret_cast<char*>(dataPointersPointres), pointerCount * sizeof(pointer_type));
        for (size_t j = 0; j < pointerCount; j++)
        {
            if (dataPointersPointres[j] == 0)
                continue;
            AREAD(dataPointersPointres[j], reinterpret_cast<char*>(dataPointres), pointerCount * sizeof(pointer_type));
            for (size_t k = 0; k < pointerCount; k++)
            {
                if (dataPointres[k] == 0)
                    continue;
                pointers.push_back(dataPointres[k]);

            }
        }
    }
    delete[] dataPointersPointres;
    delete[] dataPointres;
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
        dirItem* curent = new dirItem[dirItemCount];
        AREAD(dataBlock, reinterpret_cast<char*>(curent), dirItemCount * sizeof(dirItem));
        for (size_t i = 0; i < dirItemCount; i++)
        {
            if (std::strcmp(curent[i].name, "") != 0)
            {
                auto pair = std::make_pair(curent[i], dataBlock + i * sizeof(dirItem));
                dirItems.push_back(pair);
            }
        }
        delete[] curent;
    }
    return dirItems;
}

std::vector<dirItemP> fileSystem::getValidDirItems(inode& inode)
{
    auto vector = getDirItems(inode);
    for (auto it = vector.begin();it != vector.end();)
    {
        if ((inodeBitArray.begin() += std::get<dirItem>(*it).inode_id).getVal(fileStream) == false)
        {
            auto item = std::get<dirItem>(*it);
            WARN("Found dir item with name:" << item.name << " that points to not valid inode(" << item.inode_id << ") removing it");
            removeDirItem(inode, item);
            it = vector.erase(it);
        } else
        {
            it++;
        }
    }
    return vector;
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
    auto it = (inodeBitArray.begin() += inodeId);
    if (it.getVal(fileStream) == false)
    {
        return;
    }
    DEBUG("Free inode " << inodeId);
    it.flip(fileStream);
    char empty[sizeof(inode)];
#ifndef NDEBUG
    std::memset(empty, 'f', sizeof(empty));
    empty[0] = '<';
    empty[sizeof(empty) - 1] = '>';
#else
    std::memset(empty, '\0', sizeof(empty));
#endif
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
    DEBUG("Zeroing Data Block " << index);
    char* zero = new char[sb.blockSize];
    std::memset(zero, '\0', sb.blockSize);
    AWRITE(pointer, zero, sb.blockSize);
    delete[] zero;
    return pointer;
}

void fileSystem::freeDataBlock(pointer_type dataPointer)
{
    if (dataPointer < sb.dataAddress())
    {
        return;
    }
    auto it = ((dataBlockBitArray.begin()) += (dataPointer - sb.dataAddress()) / sb.blockSize);
    if (it.getVal(fileStream) == false)
    {
        return;
    }
    DEBUG("Free data block on " << STREAMADDRESS(dataPointer));
    it.flip(fileStream);
    char* empty = new char[sb.blockSize];
#ifndef NDEBUG
    std::memset(empty, 'f', sb.blockSize);
    empty[0] = '<';
    empty[sizeof(empty) - 1] = '>';
#else
    std::memset(empty, '\0', sb.blockSize);
#endif
    AWRITE(dataPointer, empty, sb.blockSize);
    delete[] empty;
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
        return errorCode::CAN_NOT_CREATE_FILE;
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
