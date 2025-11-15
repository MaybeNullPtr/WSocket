#pragma once
#ifndef WSOCKET__COMPRESS_HPP
#define WSOCKET__COMPRESS_HPP

#include <memory>
#include <string>

#include "../SlidingBuffer.hpp"

namespace wsocket {

enum class CompressType {
    None = 0,
    Zstd = 1,
};

class CompressContext {
public:
    virtual ~CompressContext() = default;

    virtual std::shared_ptr<CompressContext> Create() = 0;

    virtual std::string  Name() const = 0;
    virtual CompressType Type() const = 0;

    virtual std::string Configuration() const { return ""; }
    virtual bool        Configure(std::string const &config) { return true; }

    virtual Buffer Compress(const Buffer &buf)   = 0;
    virtual Buffer Decompress(const Buffer &buf) = 0;
};

} // namespace wsocket

#endif // WSOCKET__COMPRESS_HPP
