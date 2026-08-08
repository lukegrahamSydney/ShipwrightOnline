#ifndef BUFFERH
#define BUFFERH
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

class ByteStream {
  private:
    char* buffer;
    unsigned int buffSize, count;
    unsigned int readPos, writePos;
    void Grow(unsigned int additional);

  public:
    enum Origin { ORIGIN_SET, ORIGIN_CUR, ORIGIN_END };
    ByteStream();
    ByteStream(const ByteStream& data);
    ByteStream(ByteStream&& data) noexcept;
    ByteStream(const std::string& str);
    ByteStream(const char* str);
    ByteStream(const char* data, size_t dataSize);

    template <class T> ByteStream(const T& data);
    ~ByteStream() {
        free(buffer);
    }
    ByteStream& operator=(const ByteStream& data);
    ByteStream& operator=(ByteStream&& data) noexcept;
    ByteStream& operator+=(const ByteStream& data);
    ByteStream& operator<<(const ByteStream& data);
    const char& operator[](int index) const {
        return buffer[index];
    }
    char& operator[](int index) {
        return buffer[index];
    }
    template <class T> ByteStream& operator>>(T& data);
    ByteStream& operator<<(const std::string& str) {
        Write(str.data(), (unsigned int)(str.size()));
        return *this;
    }
    ByteStream& operator<<(const char* str) {
        Write(str, (unsigned int)(strlen(str)));
        return *this;
    }
    template <class T> ByteStream& operator<<(const T& data) {
        Write(data);
        return *this;
    }
    std::string ToString() const {
        return std::string(buffer, count);
    }
    char* Text() {
        return buffer;
    }
    const char* Text() const {
        return buffer;
    }
    unsigned int Length() const {
        return count;
    }
    void RewindRead() {
        readPos = 0U;
    }
    void SeekRead(long pos, Origin origin) {
        long target = 0;
        switch (origin) {
            case ORIGIN_CUR:
                target = static_cast<long>(readPos) + pos;
                break;
            case ORIGIN_SET:
                target = pos;
                break;
            case ORIGIN_END:
                target = static_cast<long>(count) - pos;
                break;
        }
        if (target < 0)
            target = 0;
        if (target > static_cast<long>(count))
            target = static_cast<long>(count);
        readPos = (unsigned int)(target);
    }
    int compare(const ByteStream& stream) const {
        unsigned int len = std::min(Length(), stream.Length());
        int r = memcmp(Text(), stream.Text(), len);
        if (r != 0)
            return r;

        if (Length() < stream.Length())
            return -1;
        if (Length() > stream.Length())
            return 1;
        return 0;
    }

    int CompareRange(unsigned int thisPos, const ByteStream& other, unsigned int otherPos, unsigned int length) const {
        if (thisPos + length > Length() || otherPos + length > other.Length())
            return 1;
        return memcmp(Text() + thisPos, other.Text() + otherPos, length);
    }
    unsigned int Write(const char* source, unsigned int length);
    unsigned int Write(const ByteStream& b);
    unsigned int Read(char* destination, unsigned int length);
    ByteStream Read(unsigned int length);
    std::string ReadString(unsigned int length);
    std::string ReadLine();
    std::string ReadString() {
        return ReadString(Length() - readPos);
    }
    void Clear(unsigned int size = 10);
    void Reserve(unsigned int additional) {
        if (writePos + additional >= buffSize)
            Grow(additional);
    }
    bool LoadFile(const std::string& fName);
    bool SaveFile(const std::string& fName) const;
    unsigned int BytesLeft() const {
        return count - readPos;
    }
    void Skip(unsigned int amount) {
        if (amount > BytesLeft()) {
            amount = BytesLeft();
        }
        readPos += amount;
    }

    void WriteVarUInt(unsigned int value) {
        while (value >= 0x80) {
            unsigned char byte = (unsigned char)((value & 0x7F) | 0x80);
            Write(reinterpret_cast<const char*>(&byte), 1);
            value >>= 7;
        }
        unsigned char last = (unsigned char)(value);
        Write(reinterpret_cast<const char*>(&last), 1);
    }
    unsigned int ReadVarUInt() {
        unsigned int value = 0;
        for (int shift = 0; shift < 35 && BytesLeft() >= 1; shift += 7) {
            unsigned char byte = 0;
            Read(reinterpret_cast<char*>(&byte), 1);
            value |= (unsigned int)(byte & 0x7F) << shift;
            if (!(byte & 0x80))
                return value;
        }
        return 0;
    }
    void Compact();
    unsigned int TellRead() const {
        return readPos;
    }
    unsigned int TellWrite() const {
        return writePos;
    }
    template <class T> unsigned int Write(const T& source);
    template <class T> T Read();
};

template <class T> ByteStream::ByteStream(const T& data) {
    buffer = 0;
    Clear(sizeof(T));
    Write(reinterpret_cast<const char*>(&data), sizeof(T));
}

template <class T> unsigned int ByteStream::Write(const T& source) {
    if (writePos + sizeof(T) < buffSize) {
        memcpy(&buffer[writePos], &source, sizeof(T));
        writePos += (unsigned int)(sizeof(T));
        if (writePos > count) {
            count = writePos;
            buffer[count] = 0;
        }
        return (unsigned int)(sizeof(T));
    }
    return Write(reinterpret_cast<const char*>(&source), (unsigned int)(sizeof(T)));
}

template <class T> ByteStream& ByteStream::operator>>(T& data) {
    if (count - readPos >= sizeof(T)) {
        memcpy(&data, &buffer[readPos], sizeof(T));
        readPos += (unsigned int)(sizeof(T));
    } else {
        Read(reinterpret_cast<char*>(&data), (unsigned int)(sizeof(data)));
    }
    return *this;
}

template <class T> T ByteStream::Read() {

    T retVal{};
    if (BytesLeft() >= sizeof(T)) {
        memcpy(&retVal, &buffer[readPos], sizeof(T));
        readPos += (unsigned int)(sizeof(T));
    }
    return retVal;
}

bool operator==(const ByteStream& stream1, const ByteStream& stream2);
bool operator!=(const ByteStream& stream1, const ByteStream& stream2);
bool operator>(const ByteStream& stream1, const ByteStream& stream2);
bool operator>=(const ByteStream& stream1, const ByteStream& stream2);
bool operator<(const ByteStream& stream1, const ByteStream& stream2);
bool operator<=(const ByteStream& stream1, const ByteStream& stream2);
ByteStream operator+(const ByteStream& stream1, const ByteStream& stream2);
#endif
