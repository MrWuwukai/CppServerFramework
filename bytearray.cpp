#include "bytearray.h"


namespace Framework {
	// Node
    ByteArray::Node::Node(size_t s)
        : ptr(new char[s])
        , size(s)
        , next(nullptr) {
    }

    ByteArray::Node::Node()
        : ptr(nullptr)
        , size(0)
        , next(nullptr) {
    }

    ByteArray::Node::~Node() {
        if (ptr) {
            delete[] ptr;
        }
    }

	// 构造
    ByteArray::ByteArray(size_t base_size)
        :m_baseSize(base_size)
        , m_position(0)
        , m_capacity(base_size)
        , m_size(0)
        , m_root(new Node(base_size))
        , m_cur(m_root) 
        , m_endian(THIS_BIG_ENDIAN){
    }

    ByteArray::~ByteArray() {
        Node* tmp = m_root;
        while (tmp) {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
    }

	// write
    void ByteArray::writeFint8(int8_t value) {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint8(uint8_t value) {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFint16(int16_t value) {
        if (m_endian != THIS_BYTE_ORDER) {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint16(uint16_t value) {
        if (m_endian != THIS_BYTE_ORDER) {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFint32(int32_t value) {
        if (m_endian != THIS_BYTE_ORDER) {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint32(uint32_t value) {
        if (m_endian != THIS_BYTE_ORDER) {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFint64(int64_t value) {
        if (m_endian != THIS_BYTE_ORDER) {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint64(uint64_t value) {
        if (m_endian != THIS_BYTE_ORDER) {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    /*
    这里涉及到了一个在数据编码中非常常见且重要的技巧，叫做 ​ZigZag 编码（ZigZag Encoding）​。
    它的主要目的是将有符号整数（如 int32_t）转换为无符号整数（如 uint32_t）​，从而让它们更适合使用像 Varint 这样的可变长度编码方式进行压缩。

    ​有符号整数（如 int32_t）​​ 的最高位（第31位）是符号位：
    0 表示正数
    1 表示负数
    这就导致一个问题：
    当我们直接对有符号整数使用 Varint 编码时，负数的二进制表示会“看起来很大”，因为它们的最高位（符号位）是 1，而 Varint 编码中最高位是用来表示“是否还有后续字节”的标志位。
    这会导致负数即使数值很小，也会被编码成多个字节，从而浪费空间。

    ZigZag 编码​ 的核心思想是：

    ​将有符号整数的“正负”信息和“数值大小”信息重新映射到一个无符号整数的空间中，使得小的数值（无论是正是负）都能用更少的字节来表示。
    它通过一种巧妙的映射方式，把：

    所有的负数映射成奇数的无符号整数
    所有的正数映射成偶数的无符号整数
    0 仍然映射成 0
    这样就能让数值接近零的整数（无论正负）都用更少的字节表示，而这正是 Varint 编码最擅长的场景！
    */
    template<class T>
    static T EncodeZigzag(const T& v) {
        if (v < 0) {
            return ((T)(-v)) * 2 - 1;
        }
        else {
            return v * 2;
        }
    }
    template<class T>
    static T DecodeZigzag(const T& v) {
        return (v >> 1) ^ -(v & 1);
    }

    void ByteArray::writeInt32(int32_t value) {
        writeUint32(EncodeZigzag(value));
    }

    void ByteArray::writeUint32(uint32_t value) {
        uint8_t tmp[5];
        uint8_t i = 0;
        // ​将一个整数拆分成多个字节来存储，每个字节的最高位（第7位，从0开始数）用作“是否还有后续字节”的标志位。
        // 如果该位为1，表示后面还有更多字节；如果为0，表示这是最后一个字节。剩下的7位用来存储数值部分。
        while (value >= 0x80) {
            tmp[i++] = (value & 0x7F) | 0x80;
            value >>= 7;
        }
        tmp[i++] = value;
        write(tmp, i);
    }

    void ByteArray::writeInt64(int64_t value) {
        writeUint64(EncodeZigzag(value));
    }

    void ByteArray::writeUint64(uint64_t value) {
        uint8_t tmp[10];
        uint8_t i = 0;
        while (value >= 0x80) {
            tmp[i++] = (value & 0x7F) | 0x80;
            value >>= 7;
        }
        tmp[i++] = value;
        write(tmp, i);
    }

    void ByteArray::writeFloat(float value) {
        uint32_t v;
        memcpy(&v, &value, sizeof(value));
        writeFuint32(v);
    }

    void ByteArray::writeDouble(double value) {
        uint64_t v;
        memcpy(&v, &value, sizeof(value));
        writeFuint64(v);
    }

    void ByteArray::writeStringF16(const std::string& value) {
        writeFuint16(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringF32(const std::string& value) {
        writeFuint32(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringF64(const std::string& value) {
        writeFuint64(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringVint(const std::string& value) {
        writeUint64(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringWithoutLength(const std::string& value) {
        write(value.c_str(), value.size());
    }

    // read
    int8_t ByteArray::readFint8() {
        int8_t v;
        read(&v, sizeof(v));
        return v;
    }

    uint8_t ByteArray::readFuint8() {
        uint8_t v;
        read(&v, sizeof(v));
        return v;
    }

    int16_t ByteArray::readFint16() {
        int16_t v;
        read(&v, sizeof(v));
        if (m_endian == THIS_BYTE_ORDER) {
            return v;
        }
        else {
            return byteswap(v);
        }
    }

    uint16_t ByteArray::readFuint16() {
        uint16_t v;
        read(&v, sizeof(v));
        if (m_endian == THIS_BYTE_ORDER) {
            return v;
        }
        else {
            return byteswap(v);
        }
    }

    int32_t ByteArray::readFint32() {
        int32_t v;
        read(&v, sizeof(v));
        if (m_endian == THIS_BYTE_ORDER) {
            return v;
        }
        else {
            return byteswap(v);
        }
    }

    uint32_t ByteArray::readFuint32() {
        uint32_t v;
        read(&v, sizeof(v));
        if (m_endian == THIS_BYTE_ORDER) {
            return v;
        }
        else {
            return byteswap(v);
        }
    }

    int64_t ByteArray::readFint64() {
        int64_t v;
        read(&v, sizeof(v));
        if (m_endian == THIS_BYTE_ORDER) {
            return v;
        }
        else {
            return byteswap(v);
        }
    }

    uint64_t ByteArray::readFuint64() {
        uint64_t v;
        read(&v, sizeof(v));
        if (m_endian == THIS_BYTE_ORDER) {
            return v;
        }
        else {
            return byteswap(v);
        }
    }

    int32_t  ByteArray::readInt32() {
        return DecodeZigzag(readUint32());
    }

    uint32_t ByteArray::readUint32() {
        uint32_t result = 0;
        for (int i = 0; i < 32; i += 7) {
            uint8_t b = readFuint8();
            if (b < 0x80) {
                result |= ((uint32_t)b) << i;
                break;
            }
            else {
                result |= (((uint32_t)(b & 0x7f)) << i);
            }
        }
        return result;
    }

    int64_t  ByteArray::readInt64() {
        return DecodeZigzag(readUint64());
    }

    uint64_t ByteArray::readUint64() {
        uint64_t result = 0;
        for (int i = 0; i < 64; i += 7) {
            uint8_t b = readFuint8();
            if (b < 0x80) {
                result |= ((uint64_t)b) << i;
                break;
            }
            else {
                result |= (((uint64_t)(b & 0x7f)) << i);
            }
        }
        return result;
    }

    float    ByteArray::readFloat() {
        uint32_t v = readFuint32();
        float value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }

    double   ByteArray::readDouble() {
        uint64_t v = readFuint64();
        double value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }

    std::string ByteArray::readStringF16() {
        uint16_t len = readFuint16();
        std::string buff(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringF32() {
        uint32_t len = readFuint32();
        std::string buff(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringF64() {
        uint64_t len = readFuint64();
        std::string buff(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringVint() {
        uint64_t len = readUint64();
        std::string buff(len);
        read(&buff[0], len);
        return buff;
    }

    // 内部方法
    // 只保留根节点
    void ByteArray::clear() {
        m_position = m_size = 0;
        m_capacity = m_baseSize;
        Node* tmp = m_root->next;
        while (tmp) {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
        m_cur = m_root;
        m_root->next = NULL;
    }

    void ByteArray::write(const void* buf, size_t size) {
        if (size == 0) {
            return;
        }
        addCapacity(size);

        // m_position是全局操作位置，这里求得的npos是在本块内的偏移量
        // 例如已经写入了10个块，每个块大小20，m_position=202，就说明在这个第11个块内也已经被写入了两个字节，你在本块内的起始偏移量是2
        size_t npos = m_position % m_baseSize; // 在本块内从哪里开始写
        size_t ncap = m_cur->size - npos; // 本块剩余大小
        size_t bpos = 0; // buf可能很长需要一块一块写，这里记录它写到哪儿

        while (size > 0) {
            if (ncap >= size) { // 如果能一次性写完
                memcpy(m_cur->ptr + npos, buf + bpos, size);
                m_position += size;
                bpos += size;
                size = 0;
                if (npos + size == m_cur->size) {
                    m_cur = m_cur->next;
                    npos = 0; // 新节点的偏移量从0开始
                }
            }
            else { // 不能一次性写完，则把本块塞满
                memcpy(m_cur->ptr + npos, buf + bpos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_cur = m_cur->next;
                ncap = m_cur->size;
                npos = 0;
            }
        }

        if (m_position > m_size) {
            m_size = m_position;
        }
    }

    void ByteArray::read(char* buf, size_t size) {
        if (size > getReadSize()) {
            throw std::out_of_range("not enough len");
        }

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur->size - npos;
        size_t bpos = 0;
        while (size > 0) {
            if (ncap >= size) {
                memcpy(buf + bpos, m_cur->ptr + npos, size);
                if (m_cur->size == npos + size) {
                    m_cur = m_cur->next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            }
            else {
                memcpy(buf + bpos, m_cur->ptr + npos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_cur = m_cur->next;
                ncap = m_cur->size;
                npos = 0;
            }
        }
    }
}