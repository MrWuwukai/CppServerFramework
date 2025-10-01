#pragma once

#include <memory>
#include <string>
#include <vector>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "endian.h"

namespace Framework {
	class ByteArray {
		
    public:
        typedef std::shared_ptr<ByteArray> ptr;

        // 链表实现，小块内存存储，不用vector，vector老是扩容操作慢
        struct Node {
            Node(size_t s);
            Node();
            ~Node();

            char* ptr;
            Node* next;
            size_t size;
        };

        ByteArray(size_t base_size = 4096);
        ~ByteArray();

        // write 固定长度
        void writeFint8(int8_t value);
        void writeFuint8(uint8_t value);
        void writeFint16(int16_t value);
        void writeFuint16(uint16_t value);
        void writeFint32(int32_t value);
        void writeFuint32(uint32_t value);
        void writeFint64(int64_t value);
        void writeFuint64(uint64_t value);

        // write 可变长度 压缩
        void writeInt32(int32_t value);
        void writeUint32(uint32_t value);
        void writeInt64(int64_t value);
        void writeUint64(uint64_t value);

        // write 其他数据类型
        void writeFloat(float value);
        void writeDouble(double value);      
        void writeStringF16(const std::string& value); // 长度int16，先存储长度再存储字符串本身
        void writeStringF32(const std::string& value); // 长度int32
        void writeStringF64(const std::string& value); // 长度int64
        void writeStringVint(const std::string& value); // 长度任意
        void writeStringWithoutLength(const std::string& value); // 无长度

        // read 固定长度
        int8_t    readFint8();
        uint8_t   readFuint8();
        int16_t   readFint16();
        uint16_t  readFuint16();
        int32_t   readFint32();
        uint32_t  readFuint32();
        int64_t   readFint64();
        uint64_t  readFuint64();

        // read 可变长度 压缩
        int32_t   readInt32();
        uint32_t  readUint32();
        int64_t   readInt64();
        uint64_t  readUint64();

        // read 其他数据类型
        float     readFloat();
        double    readDouble();
        std::string readStringF16();
        std::string readStringF32();
        std::string readStringF64();
        std::string readStringVint();

        // 内部方法
        void clear();

        void write(const void* buf, size_t size);
        void read(void* buf, size_t size);
        void read(void* buf, size_t size, size_t position) const;

        size_t getSize() const { return m_size; }
        size_t getBaseSize() const { return m_baseSize; }
        size_t getReadSize() const { return m_size - m_position; }
        size_t getPosition() const { return m_position; }
        void setPosition(size_t v);
        //bool setPositionLonger(size_t v);
        bool isLittleEndian() const {return m_endian == THIS_LITTLE_ENDIAN;}
        void setIsLittleEndian(bool val) {
            if (val) {
                m_endian = THIS_LITTLE_ENDIAN;
            }
            else {
                m_endian = THIS_BIG_ENDIAN;
            }
        }

        bool writeToFile(const std::string& name) const;
        bool readFromFile(const std::string& name);  

        std::string toString() const;
        std::string toHexString() const;

        // IO vec跟这里的链表结构很类似，将本链表转成iovec再通过socket传输较为方便
        uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const; // 从当前位置读
        uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const; // 从指定位置读
        uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);
    private:
        void addCapacity(size_t size);
        size_t getCapacity() const { return m_capacity - m_position; }
    private:
        size_t m_baseSize; // 基础大小
        size_t m_position; // 全局操作位置
        size_t m_capacity; // 每个节点加在一块的总容量
        size_t m_size; // 当前的大小
        int8_t m_endian; // 网络大小端
        Node* m_root;
        Node* m_cur; // 当前链表指针
	};
}