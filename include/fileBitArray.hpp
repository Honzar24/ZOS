#pragma once

#include <cstdlib>
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
    std::fstream& fileStream;

public:
    fileBitIterator(std::fstream& fs, int start, size_t Byte, size_t bit) : fileStream(fs),
        start(start),
        byte(Byte),
        bit(bit)
    {};
    fileBitIterator(std::fstream& fs) : fileBitIterator(fs, fs.tellg(), 0, 0) {};

    inline fileBitIterator& operator+=(const size_t adv)
    {
        byte += adv / 8;
        bit += adv % 8;
        if (bit > 7)
        {
            bit %= 8;
            byte++;
        }
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
        auto ret = *this;
        if (++bit == 8)
        {
            byte++;
            bit = 0;
        }
        return ret;
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
        auto ret = *this;
        if (--bit == -1)
        {
            byte--;
            bit = 7;
        }
        return ret;
    }
    inline void sflip()
    {
        auto currpos = fileStream.tellg();
        auto curwpos = fileStream.tellp();
        flip();
        fileStream.seekg(currpos);
        fileStream.seekp(curwpos);
    }
    inline void flip()
    {
        fileStream.seekg(start + byte - 1);
        std::bitset<8> currentByte;
        fileStream.read(reinterpret_cast<char*>(&currentByte), 1);
        currentByte[bit].flip();
        fileStream.seekp(start + byte - 1);
        fileStream.write(reinterpret_cast<char*>(&currentByte), 1);
    }
    inline ValueType operator*()
    {
        auto curpos = fileStream.tellg();
        fileStream.seekg(start + byte - 1);
        std::bitset<8> cByte;
        fileStream.read(reinterpret_cast<char*>(&cByte), 1);
        bool ret = cByte[bit];
        fileStream.seekg(curpos);
        return ret;
    }
    inline ValueType operator[](size_t index)
    {
        auto curpos = fileStream.tellg();
        fileStream.seekg(start + index / 8);
        std::bitset<8> cByte;
        fileStream.read(reinterpret_cast<char*>(&cByte), 1);
        bool ret = cByte[index % 8];
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
    std::fstream& fileStream;
    int start;
    size_t bitSize;
    size_t byteSize;

public:
    using Iterator = fileBitIterator;

    fileBitArray() = delete;
    fileBitArray(std::fstream& fs, size_t bitSize) : fileStream(fs),
        bitSize(bitSize),
        byteSize(bitSize / 8 + 1),
        start(fs.tellg())
    {};

    inline const Iterator begin()
    {
        return Iterator(fileStream, start, 1, 0);
    };
    inline const Iterator end()
    {
        return Iterator(fileStream, start, byteSize, bitSize % 8);
    };
};
