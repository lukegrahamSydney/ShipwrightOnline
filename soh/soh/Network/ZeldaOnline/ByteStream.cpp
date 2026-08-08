
#include <cstring>
#include <cstdlib>
#include "ByteStream.hpp"

ByteStream::ByteStream() {
    buffer = 0;
    Clear(200);
}

ByteStream::ByteStream(const ByteStream& data) {
    buffer = 0;
    Clear(data.Length());
    Write(data.Text(), data.Length());
}

ByteStream::ByteStream(ByteStream&& data) noexcept
    : buffer(data.buffer), buffSize(data.buffSize), count(data.count), readPos(data.readPos), writePos(data.writePos) {
    data.buffer = 0;
    data.Clear();
}

ByteStream::ByteStream(const std::string& str) {
    buffer = 0;
    Clear((unsigned int)(str.length()));
    Write(str.data(), (unsigned int)(str.length()));
}

ByteStream::ByteStream(const char* str) {
    size_t s = strlen(str);
    buffer = 0;
    Clear((unsigned int)(s));
    Write(str, (unsigned int)(s));
}

ByteStream::ByteStream(const char* data, size_t dataSize) {
    buffer = 0;
    Clear((unsigned int)(dataSize));
    Write(data, (unsigned int)(dataSize));
}

ByteStream operator+(const ByteStream& stream1, const ByteStream& stream2) {
    return ByteStream(stream1) << stream2;
}

ByteStream& ByteStream::operator=(const ByteStream& data) {
    if (this != &data) {
        Clear(data.Length());
        Write(data.Text(), data.Length());
    }
    return *this;
}

ByteStream& ByteStream::operator=(ByteStream&& data) noexcept {
    if (this != &data) {
        free(buffer);
        buffer = data.buffer;
        buffSize = data.buffSize;
        count = data.count;
        readPos = data.readPos;
        writePos = data.writePos;
        data.buffer = 0;
        data.Clear();
    }
    return *this;
}

ByteStream& ByteStream::operator+=(const ByteStream& data) {
    return *this << data;
}

ByteStream& ByteStream::operator<<(const ByteStream& data) {
    if (this == &data)
        return *this << ByteStream(data);

    Write(data.Text(), data.Length());
    return *this;
}

bool operator==(const ByteStream& stream1, const ByteStream& stream2) {
    if (stream1.Length() == stream2.Length())
        return stream1.compare(stream2) == 0;
    return false;
}

bool operator!=(const ByteStream& stream1, const ByteStream& stream2) {
    return !(stream1 == stream2);
}

bool operator<(const ByteStream& stream1, const ByteStream& stream2) {
    return stream1.compare(stream2) < 0;
}

bool operator<=(const ByteStream& stream1, const ByteStream& stream2) {
    return stream1.compare(stream2) <= 0;
}

bool operator>(const ByteStream& stream1, const ByteStream& stream2) {
    return stream1.compare(stream2) > 0;
}

bool operator>=(const ByteStream& stream1, const ByteStream& stream2) {
    return stream1.compare(stream2) >= 0;
}

void ByteStream::Clear(unsigned int size) {
    count = readPos = writePos = 0;
    unsigned int want = (size > 0 ? size : 10);

    if (buffer != 0 && buffSize >= want) {
        buffer[0] = 0;
        return;
    }

    free(buffer);
    buffSize = want;
    buffer = (char*)malloc(buffSize + 1);
    if (!buffer)
    {
        buffSize = 0;
        abort();
    }
    buffer[0] = 0;
}

void ByteStream::Compact() {
    if (readPos == 0)
        return;
    unsigned int remaining = count - readPos;
    if (remaining)
        memmove(buffer, &buffer[readPos], remaining);
    count = writePos = remaining;
    readPos = 0;
    buffer[count] = 0;
}

void ByteStream::Grow(unsigned int additional) {
    unsigned int newSize = writePos + additional + 50 + (count / 2);
    char* newBuffer = (char*)realloc(buffer, newSize + 1);
    if (!newBuffer)
        abort();
    buffer = newBuffer;
    buffSize = newSize;
}

unsigned int ByteStream::Write(const char* source, unsigned int length) {
    if (writePos + length >= buffSize)
        Grow(length);

    memcpy(&buffer[writePos], source, length);
    writePos += length;
    count = (writePos > count ? writePos : count);
    buffer[count] = 0;
    return length;
}

unsigned int ByteStream::Write(const ByteStream& b) {
    if (this == &b)
        return Write(ByteStream(b));
    return Write(b.Text(), b.Length());
}

unsigned int ByteStream::Read(char* destination, unsigned int length) {
    unsigned int len = (count - readPos < length ? count - readPos : length);
    if (len) {
        memcpy(destination, &buffer[readPos], len);
        readPos += len;
    }
    return len;
}

ByteStream ByteStream::Read(unsigned int length) {
    unsigned int len = (count - readPos < length ? count - readPos : length);
    ByteStream retVal(&buffer[readPos], len);
    readPos += len;
    return retVal;
}

std::string ByteStream::ReadString(unsigned int length) {
    unsigned int bytesLeft = Length() - TellRead();
    length = std::min(length, bytesLeft);

    if (length > 0) {
        std::string retVal(&buffer[readPos], length);
        readPos += length;
        return retVal;
    }
    return "";
}

std::string ByteStream::ReadLine() {
    std::string retVal;
    while (readPos < count) {
        if (buffer[readPos] != '\n')
            retVal += buffer[readPos++];
        else {
            ++readPos;
            break;
        }
    }
    return retVal;
}

bool ByteStream::LoadFile(const std::string& fName) {
    char readBuf[0xFFF];
    FILE* fHandle = fopen(fName.c_str(), "rb");
    if (fHandle) {
        size_t size = 0;
        Clear();
        while ((size = fread(readBuf, 1, sizeof(readBuf), fHandle)) > 0)
            Write(readBuf, (unsigned int)(size));

        fclose(fHandle);
        return true;
    }
    return false;
}

bool ByteStream::SaveFile(const std::string& fName) const {
    FILE* fHandle = fopen(fName.c_str(), "wb");
    if (fHandle) {
        size_t written = fwrite(buffer, 1, Length(), fHandle);
        fclose(fHandle);
        return written == Length();
    }
    return false;
}
