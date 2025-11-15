#pragma once
#ifndef WSOCKET__SLIDINGBUFFER_HPP
#define WSOCKET__SLIDINGBUFFER_HPP

#include <cassert>
#include <cstring> // memcpy/memmove

#include <memory>


namespace wsocket {

/**
 * Simple buffer structure for raw data handling
 */
struct Buffer {
    uint8_t *buf{};
    size_t   size{};
};

class SlidingBuffer {
public:
    SlidingBuffer() = default;
    explicit SlidingBuffer(size_t len) { Resize(len); }
    ~SlidingBuffer() = default;

    SlidingBuffer(const SlidingBuffer &)            = delete;
    SlidingBuffer &operator=(const SlidingBuffer &) = delete;

    // Adjust buffer size, preserving existing data
    void Resize(size_t len);

    // Get current data in the buffer
    Buffer GetData() const { return {buffer_.get(), buffer_used_}; }
    // Get amount of used data in the buffer
    size_t GetDataLen() const { return buffer_used_; }
    // Get total buffer size
    size_t GetSize() const { return buffer_size_; }

    // Prepare buffer space for writing
    Buffer PrepareWrite() const { return Buffer{buffer_.get() + buffer_used_, buffer_size_ - buffer_used_}; }

    // Commit written data to the buffer
    void CommitWrite(size_t len) { buffer_used_ += len; }

    /**
     * Append data to the sliding buffer
     * @param buf Buffer containing data to append
     */
    void Feed(const Buffer &buf);

    /**
     * Remove data from the start of the buffer
     * @param len Number of bytes to remove
     */
    void Consume(size_t len);
    /**
     * Remove data from a specific position in the buffer
     * @param start Starting index to remove data from
     * @param len Number of bytes to remove
     */
    void Consume(size_t start, size_t len);

private:
    std::unique_ptr<uint8_t[]> buffer_;
    size_t                     buffer_size_ = 0;
    size_t                     buffer_used_ = 0;
};


void SlidingBuffer::Resize(size_t len) {
    if(buffer_used_ > len) {
        len = buffer_used_;
    }

    std::unique_ptr<uint8_t[]> tmp = std::make_unique<uint8_t[]>(len);

    if(buffer_ && buffer_used_ > 0) {
        std::memcpy(tmp.get(), buffer_.get(), buffer_used_);
    }

    std::swap(this->buffer_, tmp);
    buffer_size_ = len;
}

void SlidingBuffer::Feed(const Buffer &buf) {
    if(buf.size + buffer_used_ > buffer_size_) {
        Resize(buf.size + buffer_used_);
    }

    std::memcpy(buffer_.get() + buffer_used_, buf.buf, buf.size);
    buffer_used_ += buf.size;
}

void SlidingBuffer::Consume(size_t len) {
    buffer_used_ -= len;

    if(buffer_used_ > 0)
        std::memmove(buffer_.get(), buffer_.get() + len, buffer_used_);
}

void SlidingBuffer::Consume(size_t start, size_t len) {
    assert(buffer_used_ >= len);

    int64_t move_len = buffer_used_ - len - start;
    assert(move_len >= 0);

    buffer_used_ -= len;
    if(buffer_used_ > 0 && move_len > 0)
        std::memmove(buffer_.get() + start, buffer_.get() + start + len, move_len);
}


} // namespace wsocket

#endif // WSOCKET__SLIDINGBUFFER_HPP
