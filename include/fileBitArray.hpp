#pragma once

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <bitset>

class fileBitIterator
{
public:
    using ValueType = bool;

private:
    // prave zpracovavany
    size_t byte;
    size_t bit = 0;
    // pocatek v souboru
    int start;

public:
    fileBitIterator(int start, size_t Byte, size_t bit) :
        start(start),
        byte(Byte),
        bit(bit)
    {};
    fileBitIterator() : fileBitIterator(0, 0, 0) {};

    inline fileBitIterator& operator+=(size_t adv)
    {
        byte += adv / 8;
        bit += adv % 8;
        if (bit > 7)
        {
            bit %= 8;
            byte++;
        }
        return *this;
    }
    inline fileBitIterator& operator++()
    {
        if (++bit == 8)
        {
            byte++;
            bit = 0;
        }
        return *this;
    }
    inline fileBitIterator& operator++(int)
    {
        auto ret = this;
        if (++bit == 8)
        {
            byte++;
            bit = 0;
        }
        return *ret;
    }
    inline fileBitIterator& operator-=(size_t dec)
    {
        byte -= dec / 8;
        bit -= dec % 8;
        if (bit < 0)
        {
            bit %= 8;
            byte--;
        }
        return *this;
    }
    inline fileBitIterator& operator--()
    {
        if (--bit == -1)
        {
            byte--;
            bit = 7;
        }
        return *this;
    }
    inline fileBitIterator& operator--(int)
    {
        auto ret = this;
        if (--bit == -1)
        {
            byte--;
            bit = 7;
        }
        return *ret;
    }
    inline void sflip(std::fstream& fileStream)
    {
        auto currpos = fileStream.tellg();
        auto curwpos = fileStream.tellp();
        flip(fileStream);
        fileStream.seekg(currpos);
        fileStream.seekp(curwpos);
    }
    inline void sflipByte(std::fstream& fileStream)
    {
        auto currpos = fileStream.tellg();
        auto curwpos = fileStream.tellp();
        flipByte(fileStream);
        fileStream.seekg(currpos);
        fileStream.seekp(curwpos);
    }
    inline void flip(std::fstream& fileStream)
    {
        fileStream.seekg(start + byte - 1);
        std::bitset<8> currentByte;
        fileStream.read(reinterpret_cast<char*>(&currentByte), 1);
        currentByte[bit].flip();
        fileStream.seekp(start + byte - 1);
        fileStream.write(reinterpret_cast<char*>(&currentByte), 1);
    }
    inline void flipByte(std::fstream& fileStream)
    {
        fileStream.seekg(start + byte - 1);
        std::bitset<8> currentByte;
        fileStream.read(reinterpret_cast<char*>(&currentByte), 1);
        currentByte.flip();
        fileStream.seekp(start + byte - 1);
        fileStream.write(reinterpret_cast<char*>(&currentByte), 1);
    }
    inline size_t cByte()
    {
        return byte -1;
    }
    inline size_t cbit()
    {
        return bit;
    }
    inline ValueType getVal(std::fstream& fileStream)
    {
        auto curpos = fileStream.tellg();
        fileStream.seekg(start + byte - 1);
        std::bitset<8> cByte;
        fileStream.read(reinterpret_cast<char*>(&cByte), 1);
        bool ret = cByte[bit];
        fileStream.seekg(curpos);
        return ret;
    }
    inline bool operator==(fileBitIterator o)
    {
        return start == o.start && byte == o.byte && bit == o.bit;
    }
    inline bool operator!=(fileBitIterator o)
    {
        return !(*this == o);
    }
};
class fileBitArray
{
private:
    int start;
    size_t bitSize;
    size_t byteSize;

public:
    using Iterator = fileBitIterator;
    fileBitArray(){};
    fileBitArray(int start, size_t bitSize) :
        bitSize(bitSize),
        byteSize(bitSize / 8 + 1),
        start(start)
    {};

    inline fileBitArray& setStart(const int& nstart){
        start = nstart;
        return *this;
    };

    inline Iterator begin()
    {
        return Iterator(start, 1, 0);
    };
    inline Iterator end()
    {
        return Iterator(start, byteSize, bitSize % 8);
    };
    inline void clear(std::fstream& fileStream)
    {
        char* zero = new char[byteSize];
        std::memset(zero,'\0',byteSize);
        fileStream.seekp(0).write(zero,byteSize);
        delete[] zero;
        fileStream.seekp(0);
    };
};
