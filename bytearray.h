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

        // ����ʵ�֣�С���ڴ�洢������vector��vector�������ݲ�����
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

        // write �̶�����
        void writeFint8(int8_t value);
        void writeFuint8(uint8_t value);
        void writeFint16(int16_t value);
        void writeFuint16(uint16_t value);
        void writeFint32(int32_t value);
        void writeFuint32(uint32_t value);
        void writeFint64(int64_t value);
        void writeFuint64(uint64_t value);

        // write �ɱ䳤�� ѹ��
        void writeInt32(int32_t value);
        void writeUint32(uint32_t value);
        void writeInt64(int64_t value);
        void writeUint64(uint64_t value);

        // write ������������
        void writeFloat(float value);
        void writeDouble(double value);      
        void writeStringF16(const std::string& value); // ����int16���ȴ洢�����ٴ洢�ַ�������
        void writeStringF32(const std::string& value); // ����int32
        void writeStringF64(const std::string& value); // ����int64
        void writeStringVint(const std::string& value); // ��������
        void writeStringWithoutLength(const std::string& value); // �޳���

        // read �̶�����
        int8_t    readFint8();
        uint8_t   readFuint8();
        int16_t   readFint16();
        uint16_t  readFuint16();
        int32_t   readFint32();
        uint32_t  readFuint32();
        int64_t   readFint64();
        uint64_t  readFuint64();

        // read �ɱ䳤�� ѹ��
        int32_t   readInt32();
        uint32_t  readUint32();
        int64_t   readInt64();
        uint64_t  readUint64();

        // read ������������
        float     readFloat();
        double    readDouble();
        std::string readStringF16();
        std::string readStringF32();
        std::string readStringF64();
        std::string readStringVint();

        // �ڲ�����
        void clear();

        void write(const void* buf, size_t size);
        void read(void* buf, size_t size);
        void read(void* buf, size_t size, size_t position) const;

        size_t getSize() const { return m_size; }
        size_t getBaseSize() const { return m_baseSize; }
        size_t getReadSize() const { return m_size - m_position; }
        size_t getPosition() const { return m_position; }
        void setPosition(size_t v);
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

        // IO vec�����������ṹ�����ƣ���������ת��iovec��ͨ��socket�����Ϊ����
        uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const; // �ӵ�ǰλ�ö�
        uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const; // ��ָ��λ�ö�
        uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);
    private:
        void addCapacity(size_t size);
        size_t getCapacity() const { return m_capacity - m_position; }
    private:
        size_t m_baseSize; // ������С
        size_t m_position; // ȫ�ֲ���λ��
        size_t m_capacity; // ÿ���ڵ����һ���������
        size_t m_size; // ��ǰ�Ĵ�С
        int8_t m_endian; // �����С��
        Node* m_root;
        Node* m_cur; // ��ǰ����ָ��
	};
}