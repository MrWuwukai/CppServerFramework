#include "bytearray.h"
#include "macro.h"
#include "log.h"

#include <vector>

Framework::Logger::ptr g_logger = LOG_ROOT();

void test() {
#define XX(type, len, write_fun, read_fun, base_len) \
    { \
    std::vector<type> vec; \
    for(int i = 0; i < len; ++i) { \
        vec.push_back(rand()); \
    } \
    Framework::ByteArray::ptr ba(new Framework::ByteArray(base_len)); \
    for(auto& i : vec) { \
        ba->write_fun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i) { \
        type v = ba->read_fun(); \
        ASSERT(v == vec[i]); \
    } \
    ASSERT(ba->getReadSize() == 0); \
    LOG_INFO(g_logger) << #write_fun "/" #read_fun \
                         " (" #type ") len=" << len \
                         << " base_len=" << base_len; \
    }

    XX(int8_t, 100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t, 100, writeFint16, readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t, 100, writeFint32, readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t, 100, writeFint64, readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t, 100, writeInt32, readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t, 100, writeInt64, readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX
}

void test_file() {
#define XX(type, len, write_fun, read_fun, base_len) {\
    std::vector<type> vec; \
    for(int i = 0; i < len; ++i) { \
        vec.push_back(rand()); \
    } \
    Framework::ByteArray::ptr ba(new Framework::ByteArray(base_len)); \
    for(auto& i : vec) { \
        ba->write_fun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i) { \
        type v = ba->read_fun(); \
        ASSERT(v == vec[i]); \
    } \
    ASSERT(ba->getReadSize() == 0); \
    LOG_INFO(g_logger) << #write_fun "/" #read_fun \
                            << " (" << #type << ") len=" << len \
                            << " base_len=" << base_len \
                            << " size=" << ba->getSize(); \
    ba->setPosition(0); \
    ASSERT(ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
    Framework::ByteArray::ptr ba2(new Framework::ByteArray(base_len * 2)); \
    ASSERT(ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
    ba2->setPosition(0); \
    ASSERT(ba->toString() == ba2->toString()); \
    ASSERT(ba->getPosition() == 0); \
    ASSERT(ba2->getPosition() == 0); \
    }

    XX(int8_t, 100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t, 100, writeFint16, readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t, 100, writeFint32, readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t, 100, writeFint64, readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t, 100, writeInt32, readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t, 100, writeInt64, readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX
}

int main(int argc, char** argv) {
    test();
    return 0;
}