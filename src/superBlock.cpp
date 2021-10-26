#include <cstring>
#include <cmath>
#include <cassert>

#include <superBlock.hpp>
#include <inode.hpp>


superBlock::superBlock(const char sig [maxSignatureLenght],const char desc[maxDescriptionLenght],size_type disk_size,size_type block_size,size_type inode_count,size_type block_count):
disk_size(disk_size),block_size(block_size),inode_count(inode_count),block_count(block_count)
{
    std::strncpy(signature,sig,maxSignatureLenght);
    signature[maxSignatureLenght] = '\0';
    std::strncpy(description,desc,maxDescriptionLenght);
    description[maxDescriptionLenght] = '\0'; 

    

}

superBlock::superBlock(size_type disk_size,size_type block_size):superBlock(defaulSignature,defaulDescription,disk_size,block_size,defaultInodeCount,defaultBlockCount){}
superBlock::superBlock(size_type disk_size):superBlock(disk_size,defaultBlockSize){}
superBlock::superBlock(){
    std::memset(this,'\0',sizeof(superBlock));
    block_size = defaultBlockSize;
}

std::ostream& superBlock::wb(std::ostream& os)
    {
        os.write((char*)this,sizeof(superBlock));
        return os;
    }

bool superBlock::setupFilePointers(){
    if (inode_count % 8 == 0 || block_count % 8 == 0)
    {
        return false;
    }
    //file pointery
    bitarray_inode_start_address = sizeof(superBlock);
    bitarray_data_block_start_address = bitarray_inode_start_address + inode_count / 8;
    inode_start_address = bitarray_data_block_start_address + block_count / 8;
    data_start_address = inode_start_address + inode_count * sizeof(inode);

    if(data_start_address + block_count * block_size < disk_size)
    {
            return false;
    }

    return true;

}