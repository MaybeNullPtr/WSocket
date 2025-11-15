#pragma once
#ifndef WSOCKET__FRAME_HPP
#define WSOCKET__FRAME_HPP

#include <cstdint>

namespace wsocket {

#pragma pack(1)
/**
 * Basic frame header structure (2 bytes total)
 * Contains control flags and basic length information
 *
 * Bit layout:
 * - Bits 0-3: Operation code (message type)
 * - Bit 4: Reserved for future use
 * - Bit 5: Reserved for future use
 * - Bit 6: Reserved for future use (commonly used for compression)
 * - Bit 7: Final frame indicator (1 = last frame in sequence)
 * - Second byte: Basic payload length or extended length indicator
 */
struct BasicHeader {
    uint8_t opcode : 4; // Low 4 bits store opcode
    uint8_t rsv3 : 1;   // Bit 4
    uint8_t rsv2 : 1;   // Bit 5
    uint8_t rsv1 : 1;   // Bit 6
    uint8_t fin : 1;    // Highest bit (bit 7)

    uint8_t payload_length = 0;
};
#pragma pack()

#pragma pack(1)
/**
 * Complete frame header structure with support for extended payload lengths
 *  * Total size varies based on payload length:
 * - 2 bytes: For small payloads (0-253)
 * - 4 bytes: For medium payloads (254-65535)
 * - 10 bytes: For large payloads (≥65536)
 *
 * Uses network byte order (big-endian) for extended length fields
 * to ensure compatibility across different platforms
 */
struct FrameHeader {
public:
    // Frame control methods
    bool Finished() const { return header_.fin; }
    void Finished(bool finish) { this->header_.fin = finish; }

    bool Compressed() const { return header_.rsv1; }
    void Compressed(bool compressed) { this->header_.rsv1 = compressed; }

    // Frame type enumeration and methods
    enum FrameType {
        System = 0x0,
        Text   = 0x1,
        Binary = 0x2,
        Close  = 0x8,
        Ping   = 0x9,
        Pong   = 0xA,
    };
    FrameType Type() const { return static_cast<FrameType>(this->header_.opcode); }
    void      Type(FrameType type) { this->header_.opcode = static_cast<uint8_t>(type); }

    // Payload length handling with automatic encoding selection
    size_t Length() const {
        // Determine actual payload length based on encoding

        if(header_.payload_length == 0b1111'1111) {
            // 64-bit length
            return htonll(this->payload_length_);
        }
        if(header_.payload_length == 0b1111'1110) {
            // 16-bit length
            return htons(this->payload_length_);
        }
        // Direct length
        return header_.payload_length;
    }
    /**
     * Set payload length with automatic encoding selection
     * @param len Actual payload length to set
     */
    void Length(uint64_t len) {
        if(len < 0b1111'1110) {
            this->header_.payload_length = len;
        } else if(len <= std::numeric_limits<uint16_t>::max()) {
            this->header_.payload_length = 0b1111'1110;
            this->payload_length_        = ntohs(len);
        } else {
            this->header_.payload_length = 0b1111'1111;
            this->payload_length_        = ntohll(len);
        }
    }

    /**
     * Calculate total header length in bytes
     * based on current payload length encoding
     */
    int HeaderLength() const {
        // 2 bytes
        static constexpr int basic_len = sizeof(BasicHeader);

        if(header_.payload_length < 0b1111'1110) {
            // 2 bytes
            return basic_len;
        } else if(header_.payload_length == 0b1111'1111) {
            // 2 + 2 bytes
            return basic_len + sizeof(uint16_t);
        } else {
            // 2 + 8 bytes
            return basic_len + sizeof(uint64_t);
        }
    }

private:
    // 2 byte
    BasicHeader header_{}; // Basic header with control flags and opcode
    // 8 byte
    uint64_t    payload_length_ = 0; // Extended payload length field (network byte order)
};
#pragma pack()

struct Frame {
    FrameHeader header;
    Buffer      data;

    static size_t ShortPayload() { return 0b1111'1110; }
    static size_t MiddlePayload() { return std::numeric_limits<uint16_t>::max(); }
};

struct OwnedFrame : Frame {
    ~OwnedFrame() {
        if(data.buf) {
            delete[] data.buf;
        }
    }
};

enum class CloseCode : int {
    CLOSE_NORMAL         = 1000,
    CLOSE_PROTOCOL_ERROR = 1002,
    INTERNAL_ERROR       = 1011,
};

inline const char *CloseMessage(CloseCode code) {
    static std::unordered_map<CloseCode, const char *> CloseMessageMap = {
            {CloseCode::CLOSE_NORMAL, "close normal"},
            {CloseCode::CLOSE_PROTOCOL_ERROR, "close protocol error"},
            {CloseCode::INTERNAL_ERROR, "internal error"},
    };

    auto it = CloseMessageMap.find(code);
    if(it == CloseMessageMap.end()) {
        return nullptr;
    }
    return it->second;
}


} // namespace wsocket

#endif // WSOCKET__FRAME_HPP
