#pragma once

#include <vector>
#include <string>

#define BITSTRING_SIZE 4
#define BITSTRING2_SIZE 8
#define CARTPOS_SIZE 100
#define FRAME_SIZE 96
#define JOINTPOS_SIZE 264

class CInputOutputBlock
{
public:
    CInputOutputBlock(int opcode,const char* rcsHandle); // Input block
    CInputOutputBlock(unsigned char* data,int dataSize); // Output block
    virtual ~CInputOutputBlock();

    // When block is INPUT:
    void pushBackReal(double doubleData);
    void pushBackInteger(int intData);
    void pushBackString(const std::string& string);
    void pushBackBuffer(const char* buffer,int bufferSize);
    void pushBackBitstring(const std::string& bitstring4);
    void pushBackBitstring2(const std::string& bitstring8);
    void pushBackCartPos(const std::string& cartPos388);
    void pushBackFrame(const std::string& frame384);
    void pushBackJointPos(const std::string& jointPos1032);

    void* getDataPointer(int* blockSize);

    // When block is OUTPUT:
    unsigned char readByte();
    int readInteger();
    double readReal();
    std::string readString();
    std::string readBuffer(int bufferSize);
    std::string readBitstring();
    std::string readBitstring2();
    std::string readCartPos();
    std::string readFrame();
    std::string readJointPos();

protected:
    std::vector<unsigned char> _data;

    std::vector<unsigned char> _tempData;
    std::vector<int> _tempStringOffsetOffsets;
    std::vector<unsigned char> _tempStringData;

    int _readOffset;
};
