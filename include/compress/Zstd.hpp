#pragma once
#ifndef WSOCKET__ZSTD_HPP
#define WSOCKET__ZSTD_HPP

#ifdef WITH_ZSTD

#include <vector>

#include <zstd.h>

#include "Compress.hpp"


namespace wsocket {

class ZstdContext : public CompressContext {
public:
    ZstdContext() = default;
    ~ZstdContext() override {
        ZSTD_freeCCtx(cctx_);
        ZSTD_freeDCtx(dctx_);
    }

    bool Open();

    void CompressionLevel(int level);
    void CheckSum(bool check);
    void ThreadCount(size_t count);

    //============= CompressContext start =============//
    std::shared_ptr<CompressContext> Create() override;

    std::string  Name() const override { return "zstd"; }
    CompressType Type() const override { return CompressType::Zstd; }

    Buffer Compress(const Buffer &buf) override;
    Buffer Decompress(const Buffer &buf) override;
    //============= CompressContext end =============//


private:
    ZSTD_CCtx *cctx_{nullptr};
    ZSTD_DCtx *dctx_{nullptr};

    std::vector<uint8_t> cbuf_;
    std::vector<uint8_t> dbuf_;
};

bool ZstdContext::Open() {
    cctx_ = ZSTD_createCCtx();
    dctx_ = ZSTD_createDCtx();
    if(cctx_ == nullptr || dctx_ == nullptr) {
        return false;
    }
    return true;
}

void ZstdContext::CompressionLevel(int level) {
    if(level < ::ZSTD_minCLevel()) {
        level = ::ZSTD_minCLevel();
    } else if(level > ::ZSTD_maxCLevel()) {
        level = ::ZSTD_maxCLevel();
    }
    auto res = ::ZSTD_CCtx_setParameter(cctx_, ZSTD_c_compressionLevel, level);
    if(ZSTD_isError(res)) {
        printf("Zstd error: %s\n", ZSTD_getErrorName(res));
    }
}

void ZstdContext::CheckSum(bool check) {
    auto res = ::ZSTD_CCtx_setParameter(cctx_, ZSTD_c_checksumFlag, check);
    if(ZSTD_isError(res)) {
        printf("Zstd error: %s\n", ZSTD_getErrorName(res));
    }
}

void ZstdContext::ThreadCount(size_t count) {
    auto res = ::ZSTD_CCtx_setParameter(cctx_, ZSTD_c_nbWorkers, count);
    if(ZSTD_isError(res)) {
        printf("Zstd error: %s\n", ZSTD_getErrorName(res));
    }
}

std::shared_ptr<CompressContext> ZstdContext::Create() {
    auto ptr = std::make_shared<ZstdContext>();
    if(ptr->Open()) {
        return ptr;
    }
    return nullptr;
}

//============= CompressContext start =============//

Buffer ZstdContext::Compress(const Buffer &buf) {
    auto want_len = ZSTD_compressBound(buf.size);
    if(cbuf_.size() < want_len) {
        cbuf_.resize(want_len);
    }
    auto real_len = ZSTD_compress2(cctx_, cbuf_.data(), cbuf_.size(), buf.buf, buf.size);
    if(ZSTD_isError(real_len)) {
        printf("Zstd compress2 error: %s\n", ZSTD_getErrorName(real_len));
        return Buffer{};
    }
    return Buffer{cbuf_.data(), real_len};
}

Buffer ZstdContext::Decompress(const Buffer &buf) {
    auto want_len = ZSTD_getFrameContentSize(buf.buf, buf.size);
    if(want_len == ZSTD_CONTENTSIZE_ERROR || want_len == ZSTD_CONTENTSIZE_UNKNOWN) {
        if(ZSTD_isError(want_len)) {
            printf("Zstd decompress error: %s\n", ZSTD_getErrorName(want_len));
            return Buffer{};
        }
    }

    if(dbuf_.size() < want_len) {
        dbuf_.resize(want_len);
    }
    auto real_len = ZSTD_decompressDCtx(dctx_, dbuf_.data(), dbuf_.size(), buf.buf, buf.size);
    if(ZSTD_isError(real_len)) {
        printf("Zstd decompress error: %s\n", ZSTD_getErrorName(real_len));
        return Buffer{};
    }
    return Buffer{dbuf_.data(), real_len};
}

//============= CompressContext end =============//

} // namespace wsocket

#endif


#endif // WSOCKET__ZSTD_HPP
