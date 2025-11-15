#pragma once
#ifndef WSOCKET__WSOCKET_CONTEXT_HPP
#define WSOCKET__WSOCKET_CONTEXT_HPP

#include <cstdint>
#include <cstring>

#include <optional>
#include <unordered_set>
#include <functional>
#include <limits>


#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <endian.h>
#include <arpa/inet.h>
#define ntohll(x) be64toh(x)
#define htonll(x) htobe64(x)
#endif


#include "SlidingBuffer.hpp"
#include "Error.h"
#include "Frame.hpp"
#include "compress/Compress.hpp"
#include "compress/CompressManager.hpp"


namespace wsocket {

class FrameParser {
public:
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void OnFrame(const Frame &frame) {}
    };

    explicit FrameParser(Listener *listener = nullptr) : listener_(listener) {}

    void   SetReceiveBufferSize(size_t len) { buffer_.Resize(len); }
    Buffer PrepareWrite() const { return buffer_.PrepareWrite(); }
    void   CommitWrite(size_t len) { buffer_.CommitWrite(len); }

    void Feed(const Buffer &buf) { buffer_.Feed(buf); }

    bool ParseOne() {
        auto raw_data = buffer_.GetData();

        if(raw_data.size < 2) {
            // need more data
            return false;
        }

        FrameHeader *header = reinterpret_cast<FrameHeader *>(raw_data.buf);

        if(header->HeaderLength() > raw_data.size) {
            // need more data
            return false;
        }

        if(header->HeaderLength() + header->Length() > raw_data.size) {
            // need more data
            return false;
        }

        Frame frame;

        frame.header    = *header;
        frame.data.size = header->Length();
        frame.data.buf  = raw_data.buf + header->HeaderLength();

        if(this->listener_) {
            listener_->OnFrame(frame);
        }

        buffer_.Consume(header->HeaderLength() + header->Length());

        return true;
    }

    void ResetListener(Listener *listener) { listener_ = listener; }

private:
    SlidingBuffer buffer_;
    Listener     *listener_{nullptr};
};


class WSocketContext : public FrameParser::Listener {
    enum class State {
        Init,
        Closed,
        Closing,
        Connecting,
        Connected,
        Error,
    } state_ = State::Init;

    static constexpr int64_t RECEIVE_BUFFER_DEFAULT = 8 * 1024; // 8k

public:
    WSocketContext() : parser_(this) { parser_.SetReceiveBufferSize(RECEIVE_BUFFER_DEFAULT); }
    ~WSocketContext() override {}

    State GetState() const { return state_; }

    Buffer PrepareWrite() const { return parser_.PrepareWrite(); }
    void   CommitWrite(size_t len) {
        parser_.CommitWrite(len);
        ParseProcess();
    }

    void Feed(const Buffer &buf) {
        parser_.Feed(buf);
        this->ParseProcess();
    }

    void Handshake() {
        assert(state_ == State::Init);
        this->state_               = State::Connecting;
        auto supported_compressors = CompressManager::Instance().GetSupportedCompressors();

        // send system handshake frame
        Frame frame;
        frame.header.Finished(true);
        frame.header.Type(FrameHeader::System);
        frame.header.Length(supported_compressors.size());

        frame.data.buf  = reinterpret_cast<uint8_t *>(const_cast<char *>(supported_compressors.c_str()));
        frame.data.size = supported_compressors.size();
        this->SendFrame(frame);
    }
    void Handshake(const std::vector<CompressType> &compressors) {
        assert(state_ == State::Init);
        this->state_               = State::Connecting;
        auto supported_compressors = CompressManager::Instance().GetSupportedCompressors(compressors);

        // send system handshake frame
        Frame frame;
        frame.header.Finished(true);
        frame.header.Type(FrameHeader::System);
        frame.header.Length(supported_compressors.size());

        frame.data.buf  = reinterpret_cast<uint8_t *>(const_cast<char *>(supported_compressors.c_str()));
        frame.data.size = supported_compressors.size();
        this->SendFrame(frame);
    }

    void Close(CloseCode code) { this->Close(static_cast<int16_t>(code), CloseMessage(code)); }
    void Close(int16_t code, const std::string &reason) {
        assert(state_ != State::Closed);

        state_ = State::Closing;

        if(reason.size() + 2 > Frame::ShortPayload()) {
            this->NotifyError(Error::ErrorReasonTooLong);
            this->Close(CloseCode::INTERNAL_ERROR);
            return;
        }
        Frame frame;
        frame.header.Finished(true);
        frame.header.Type(FrameHeader::Close);

        // set body
        auto total_len = 2 + reason.size();
        frame.header.Length(total_len);
        std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(total_len);
        memcpy(buffer.get(), &code, 2);
        memcpy(buffer.get() + 2, reason.c_str(), reason.size());

        frame.data.buf  = buffer.get();
        frame.data.size = 2 + reason.size();

        this->SendFrame(frame);
    }

    void SendText(std::string_view text, bool finish = true) {
        assert(state_ == State::Connected);

        if(text.empty()) {
            this->NotifyError(Error::MessageEmpty);
            return;
        }

        Buffer buffer;
        buffer.buf  = reinterpret_cast<uint8_t *>(const_cast<char *>(text.data()));
        buffer.size = text.size();

        if(this->compress_context_) {
            buffer = this->compress_context_->Compress(buffer);
            if(buffer.buf == nullptr || buffer.size == 0) {
                this->NotifyError(Error::CompressError);
                Close(CloseCode::INTERNAL_ERROR);
                return;
            }
        }

        Frame frame;
        frame.header.Type(FrameHeader::Text);
        frame.header.Length(buffer.size);
        frame.header.Finished(finish);

        frame.data = buffer;

        this->SendFrame(frame);
    }
    void SendBinary(Buffer buffer, bool finish = true) {
        assert(state_ == State::Connected);

        if(buffer.size == 0) {
            this->NotifyError(Error::MessageEmpty);
            return;
        }

        Frame frame;
        frame.header.Type(FrameHeader::Binary);
        frame.header.Length(buffer.size);
        frame.header.Finished(finish);

        frame.data = buffer;
        this->SendFrame(frame);
    }

    void Ping() {
        Frame frame;
        frame.header.Type(FrameHeader::Ping);
        frame.header.Finished(true);
        frame.header.Length(4);

        frame.data.buf  = reinterpret_cast<uint8_t *>(const_cast<char *>("ping"));
        frame.data.size = 4;
        this->SendFrame(frame);
    }
    void Pong() {
        Frame frame;
        frame.header.Type(FrameHeader::Pong);
        frame.header.Length(4);

        frame.data.buf  = reinterpret_cast<uint8_t *>(const_cast<char *>("pong"));
        frame.data.size = 4;
        this->SendFrame(frame);
    }

private:
    void ParseProcess() {
        while(state_ != State::Closed && state_ != State::Error && parser_.ParseOne()) {
        }
    }

    void OnFrame(const Frame &frame) override {
        if(state_ == State::Closed || state_ == State::Error) {
            return;
        }

        switch(frame.header.Type()) {
        case FrameHeader::System:
            this->OnSystemFrame(frame);
            break;
        case FrameHeader::Text:
            this->OnTextFrame(frame);
            break;
        case FrameHeader::Binary:
            this->OnBinaryFrame(frame);
            break;
        case FrameHeader::Close:
            this->OnCloseFrame(frame);
            break;
        case FrameHeader::Ping:
            this->OnPingFrame(frame);
            break;
        case FrameHeader::Pong:
            this->OnPongFrame(frame);
            break;
        }
    }

    void OnSystemFrame(const Frame &frame) {
        auto str            = std::string(reinterpret_cast<const char *>(frame.data.buf), frame.data.size);
        auto compress_types = CompressManager::Instance().GetSupportedCompressTypes(str);

        auto type               = NotifyHandshake(compress_types);
        this->compress_context_ = CompressManager::Instance().GetCompressContext(type);

        if(this->state_ == State::Init) {
            if(this->compress_context_) {
                this->Handshake({this->compress_context_->Type()});
            } else {
                this->Handshake({CompressType::None});
            }
        }
        this->state_ = State::Connected;
        this->NotifyConnected();
    }

    void OnTextFrame(const Frame &frame) { this->NotifyText(frame); }

    void OnBinaryFrame(const Frame &frame) { this->NotifyBinary(frame); }

    void OnPingFrame(const Frame &frame) { this->NotifyPing(); }

    void OnPongFrame(const Frame &frame) { this->NotifyPong(); }

    void OnCloseFrame(const Frame &frame) {
        if(frame.data.size < 2) {
            return;
        }

        int16_t     code = 0;
        std::string reason;

        memcpy(&code, frame.data.buf, 2);
        if(frame.data.size > 2) {
            reason.assign(reinterpret_cast<const char *>(frame.data.buf + 2), frame.data.size - 2);
        }

        if(state_ != State::Closing) {
            this->Close(CloseCode::CLOSE_NORMAL);
        }

        state_ = State::Closed;
        NotifyClose(code, reason);
    }

private:
    FrameParser parser_;


public:
    class Listener {
    public:
        virtual ~Listener() = default;

        virtual void OnError(std::error_code code) {}

        virtual CompressType OnHandshake(const std::vector<CompressType> &request_compress_type) {
            return CompressType::None;
        }
        virtual void OnConnected() {}
        virtual void OnClose(int16_t code, const std::string &reason) {}
        virtual void OnPing() {}
        virtual void OnPong() {}

        virtual void OnText(std::string_view text, bool finish) {}
        virtual void OnBinary(Buffer buffer, bool finish) {}
    };
    void ResetListener(Listener *listener) { listener_ = listener; }

    void NotifyError(std::error_code code) {
        if(listener_) {
            listener_->OnError(code);
        }
    }
    CompressType NotifyHandshake(const std::vector<CompressType> &request_compress_type) {
        if(listener_) {
            return listener_->OnHandshake(request_compress_type);
        }
        return CompressType::None;
    }
    void NotifyClose(int16_t code, const std::string &reason) {
        if(listener_) {
            listener_->OnClose(code, reason);
        }
    }
    void NotifyPing() {
        if(listener_) {
            listener_->OnPing();
        }
    }
    void NotifyPong() {
        if(listener_) {
            listener_->OnPong();
        }
    }
    void NotifyText(Frame frame) {
        auto buf = frame.data;

        if(this->compress_context_) {
            buf = this->compress_context_->Decompress(buf);
            if(buf.buf == nullptr || buf.size == 0) {
                this->NotifyError(Error::DecompressError);
                return;
            }
        }

        if(listener_) {
            listener_->OnText(std::string_view(reinterpret_cast<char *>(buf.buf), buf.size), frame.header.Finished());
        }
    }
    void NotifyBinary(Frame frame) {
        auto buf = frame.data;

        if(this->compress_context_) {
            buf = this->compress_context_->Decompress(buf);
            if(buf.buf == nullptr || buf.size == 0) {
                this->NotifyError(Error::DecompressError);
                return;
            }
        }

        if(listener_) {
            listener_->OnBinary(buf, frame.header.Finished());
        }
    }

    void NotifyConnected() {
        if(listener_) {
            listener_->OnConnected();
        }
    }

private:
    Listener *listener_{nullptr};

public:
    using SendHandler = std::function<void(const Buffer &data)>;

    void ResetSendHandler(SendHandler &&handler) { send_handler_ = std::move(handler); }

private:
    void SendFrame(const Frame &frame) {
        assert(frame.header.Length() == frame.data.size);
        size_t total_len = frame.header.HeaderLength() + frame.header.Length();

        std::unique_ptr<uint8_t[]> data(new uint8_t[total_len]);

        memcpy(data.get(), &frame.header, frame.header.HeaderLength());
        memcpy(data.get() + frame.header.HeaderLength(), frame.data.buf, frame.header.Length());

        SendRawData({data.get(), total_len});
    }
    void SendFrames(const std::vector<Frame> &frames) {
        size_t total_len = 0;
        for(auto &frame : frames) {
            assert(frame.header.Length() == frame.data.size);
            total_len += (frame.header.HeaderLength() + frame.data.size);
        }
        std::unique_ptr<uint8_t[]> data(new uint8_t[total_len]);
        size_t                     pos = 0;

        for(auto &frame : frames) {
            memcpy(&data[pos], &frame.header, frame.header.HeaderLength());
            pos += frame.header.HeaderLength();

            memcpy(&data[pos], frame.data.buf, frame.data.size);
            pos += frame.data.size;
        }
        assert(pos == total_len);
        SendRawData({data.get(), total_len});
    }

    void SendRawData(const Buffer &data) {
        if(send_handler_) {
            send_handler_(data);
        }
    }

private:
    SendHandler                      send_handler_;
    std::shared_ptr<CompressContext> compress_context_;
};

} // namespace wsocket

#endif // WSOCKET__WSOCKET_CONTEXT_HPP
