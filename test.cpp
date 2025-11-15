#include <bitset>
#include <cassert>
#include <iostream>

#include "include/WSocketContext.hpp"
#include "include/ASIO_WSocket.hpp"

void testBasicHeader() {
    wsocket::BasicHeader header;
    header.fin            = (true);
    header.rsv1           = (true);
    header.rsv2           = (true);
    header.rsv3           = (true);
    header.opcode         = (0b0101);
    header.payload_length = 124;

    assert(header.fin == true);
    assert(header.rsv1 == true);
    assert(header.rsv2 == true);
    assert(header.rsv3 == true);
    assert(header.opcode == 0b0101);
    assert(header.payload_length == 124);

    uint8_t value[2] = {0};
    memcpy(&value, &header, sizeof(header));
    std::cout << std::bitset<8>(value[0]) << std::bitset<8>(value[1]) << std::endl;
}

void testFrameHeader() {
    {
        wsocket::FrameHeader header;

        uint8_t value[8] = {};
        value[0]         = 0b1100'1000;
        value[1]         = 127;

        memcpy(&header, &value, sizeof(header));
        assert(header.Finished() == true);
        assert(header.Compressed() == true);
        assert(header.Type() == wsocket::FrameHeader::Close);
        assert(header.Length() == 127);
    }
    {
        wsocket::FrameHeader header;

        uint8_t value[10] = {};
        value[0]          = 0b1100'1000;
        value[1]          = 0b1111'1110;
        value[2]          = 0b0000'0000;
        value[3]          = 0b1111'1111;

        value[4] = 0b1111'1111;
        value[5] = 0b1111'1111;
        value[6] = 0b1111'1111;
        value[7] = 0b1111'1111;
        value[8] = 0b1111'1111;
        value[9] = 0b1111'1111;

        memcpy(&header, &value, sizeof(header));
        assert(header.Finished() == true);
        assert(header.Compressed() == true);
        assert(header.Type() == wsocket::FrameHeader::Close);
        std::cout << "length : " << header.Length() << " " << std::bitset<16>(header.Length()) << std::endl;
        assert(header.Length() == 255);
    }
    {
        wsocket::FrameHeader header;

        uint8_t value[10] = {};
        value[0]          = 0b1100'1000;
        value[1]          = 0b1111'1110;
        value[2]          = 0b0000'0100;
        value[3]          = 0b1010'0011;

        value[4] = 0b1010'0101;
        value[5] = 0b1010'0101;
        value[6] = 0b1010'0101;
        value[7] = 0b1010'0101;
        value[8] = 0b1010'0101;
        value[9] = 0b1010'0101;

        memcpy(&header, &value, sizeof(header));
        assert(header.Finished() == true);
        assert(header.Compressed() == true);
        assert(header.Type() == wsocket::FrameHeader::Close);
        std::cout << "length : " << header.Length() << " " << std::bitset<16>(header.Length()) << std::endl;
        assert(header.Length() == 0b0000'0100'1010'0011);
    }
    {
        wsocket::FrameHeader header;

        uint8_t value[10] = {};
        value[0]          = 0b1100'1000;
        value[1]          = 0b1111'1111;

        value[2] = 0b0000'0000;
        value[3] = 0b1111'1111;
        value[4] = 0b1111'1111;
        value[5] = 0b1111'1111;
        value[6] = 0b1111'1111;
        value[7] = 0b1111'1111;
        value[8] = 0b1111'1111;
        value[9] = 0b1111'1111;

        memcpy(&header, &value, sizeof(header));
        assert(header.Finished() == true);
        assert(header.Compressed() == true);
        assert(header.Type() == wsocket::FrameHeader::Close);
        std::cout << "length : " << header.Length() << " " << std::bitset<64>(header.Length()) << std::endl;
        assert(header.Length() == 0x00ff'ffff'ffff'ffff);
    }
    {
        wsocket::FrameHeader header;
        header.Finished(true);
        header.Compressed(true);
        header.Type(wsocket::FrameHeader::Pong);
        header.Length(127);

        uint8_t value[10] = {};
        memcpy(&value[0], &header, sizeof(header));

        assert(value[0] == 0b1100'1010);
        assert(value[1] == 0b0111'1111);
    }
    {
        wsocket::FrameHeader header;
        header.Finished(true);
        header.Compressed(true);
        header.Type(wsocket::FrameHeader::Pong);
        header.Length(256);

        uint8_t value[10] = {};
        memcpy(&value[0], &header, sizeof(header));

        assert(value[0] == 0b1100'1010);
        assert(value[1] == 0b1111'1110);

        assert(value[2] == 0b0000'0001);
        assert(value[3] == 0b0000'0000);
    }
    {
        wsocket::FrameHeader header;
        header.Finished(true);
        header.Compressed(true);
        header.Type(wsocket::FrameHeader::Pong);
        header.Length(264);

        uint8_t value[10] = {};
        memcpy(&value[0], &header, sizeof(header));

        assert(value[0] == 0b1100'1010);
        assert(value[1] == 0b1111'1110);

        assert(value[2] == 0b0000'0001);
        assert(value[3] == 0b0000'1000);
    }
    {
        wsocket::FrameHeader header;
        header.Finished(true);
        header.Compressed(true);
        header.Type(wsocket::FrameHeader::Pong);
        header.Length(264);

        uint8_t value[10] = {};
        memcpy(&value[0], &header, sizeof(header));

        assert(value[0] == 0b1100'1010);
        assert(value[1] == 0b1111'1110);

        assert(value[2] == 0b0000'0001);
        assert(value[3] == 0b0000'1000);
    }
    {
        wsocket::FrameHeader header;
        header.Finished(true);
        header.Compressed(true);
        header.Type(wsocket::FrameHeader::Pong);
        header.Length(std::numeric_limits<uint64_t>::max());

        uint8_t value[10] = {};
        memcpy(&value[0], &header, sizeof(header));

        assert(value[0] == 0b1100'1010);
        assert(value[1] == 0b1111'1111);

        assert(value[2] == 0b1111'1111);
        assert(value[3] == 0b1111'1111);
        assert(value[4] == 0b1111'1111);
        assert(value[5] == 0b1111'1111);
        assert(value[6] == 0b1111'1111);
        assert(value[7] == 0b1111'1111);
        assert(value[8] == 0b1111'1111);
        assert(value[9] == 0b1111'1111);
    }
    {
        wsocket::FrameHeader header;
        header.Finished(true);
        header.Compressed(true);
        header.Type(wsocket::FrameHeader::Pong);
        header.Length(55169595);

        uint8_t value[10] = {};
        memcpy(&value[0], &header, sizeof(header));

        assert(value[0] == 0b1100'1010);
        assert(value[1] == 0b1111'1111);

        assert(value[2] == 0b0000'0000);
        assert(value[3] == 0b0000'0000);
        assert(value[4] == 0b0000'0000);
        assert(value[5] == 0b0000'0000);
        assert(value[6] == 0b0000'0011);
        assert(value[7] == 0b0100'1001);
        assert(value[8] == 0b1101'0010);
        assert(value[9] == 0b0011'1011);

        assert(header.Length() == 55169595);
    }
}

std::string to_string(const wsocket::Buffer &buffer) {
    return std::string(reinterpret_cast<char *>(buffer.buf), buffer.size);
}

void test_SlidingBuffer() {
    {
        wsocket::SlidingBuffer buffer;
        char                   data[] = "abcdefghijklmnopqrst";
        buffer.Feed({reinterpret_cast<uint8_t *>(data), 20});
        auto str = to_string(buffer.GetData());
        std::cout << str << std::endl;
        assert(str == "abcdefghijklmnopqrst");

        buffer.Consume(3);
        str = to_string(buffer.GetData());
        std::cout << str << std::endl;
        assert(str == "defghijklmnopqrst");
    }
    {
        wsocket::SlidingBuffer buffer;
        char                   data[] = "abcdefghijklmnopqrst";
        buffer.Feed({reinterpret_cast<uint8_t *>(data), 20});
        auto str = to_string(buffer.GetData());
        std::cout << str << std::endl;
        assert(str == "abcdefghijklmnopqrst");

        buffer.Consume(4, 3);
        str = to_string(buffer.GetData());
        std::cout << str << std::endl;
        assert(str == "abcdhijklmnopqrst");

        buffer.Consume(3, 14);
        str = to_string(buffer.GetData());
        std::cout << str << std::endl;
        assert(str == "abc");
    }
}

class Client : public wsocket::WSocketContext::Listener {
public:
    void OnError(std::error_code code) override {}

    wsocket::CompressType OnHandshake(const std::vector<wsocket::CompressType> &surpported_compress_type) override {
        std::cout << "OnHandshake " << std::endl;
        return wsocket::CompressType::None;
    }
    void OnClose(int16_t code, const std::string &reason) override {
        std::cout << "OnClose: " << code << ":" << reason << std::endl;
    }
    void OnPing() override { std::cout << "OnPing" << std::endl; }
    void OnPong() override { std::cout << "OnPong" << std::endl; }

    void OnText(std::string_view text, bool finish) override {
        std::cout << "OnText:" << std::string(text) << std::endl;
    }
    void OnBinary(wsocket::Buffer buffer, bool finish) override {
        std::cout << "OnBinary:" << std::string(reinterpret_cast<char *>(buffer.buf), buffer.size) << std::endl;
    }
};

void test_WSocketContext() {
    std::cout << "================== test_WSocketContext ==================" << std::endl;
    wsocket::WSocketContext ctx1;
    wsocket::WSocketContext ctx2;

    Client client1;
    ctx1.ResetListener(&client1);
    Client client2;
    ctx2.ResetListener(&client2);

    ctx1.ResetSendHandler([&](wsocket::Buffer buffer) { ctx2.Feed(buffer); });
    ctx2.ResetSendHandler([&](wsocket::Buffer buffer) { ctx1.Feed(buffer); });

    ctx1.Handshake();

    ctx1.SendText("abcdefghijklmnopqrst");
    ctx2.SendBinary(wsocket::Buffer({reinterpret_cast<uint8_t *>(const_cast<char *>("zxc")), 3}));

    ctx1.Ping();
    ctx2.Ping();
    ctx1.Pong();
    ctx2.Pong();

    ctx1.SendText("abcdefghijklmnopqrst");
    ctx2.SendBinary(wsocket::Buffer({reinterpret_cast<uint8_t *>(const_cast<char *>("zxc")), 3}));

    ctx1.SendText("abcdefghijklmnopqrst");
    ctx2.SendBinary(wsocket::Buffer({reinterpret_cast<uint8_t *>(const_cast<char *>("zxc")), 3}));

    ctx2.Close(wsocket::CloseCode::CLOSE_NORMAL);
    std::cout << "================== test_WSocketContext ==================" << std::endl;
}

#ifdef WITH_ASIO
class TestWSocket : public wsocket::WSocket {
protected:
    explicit TestWSocket(asio::io_context &io_executor) :
        wsocket::WSocket(io_executor.get_executor()), executor_(io_executor) {}
    explicit TestWSocket(asio::io_context &io_executor, asio::ip::tcp::socket &&socket) :
        wsocket::WSocket(std::move(socket)), executor_(io_executor) {}

public:
    static std::shared_ptr<TestWSocket> Create(asio::io_context &io_executor) {
        return std::shared_ptr<TestWSocket>(new TestWSocket(std::move(io_executor)));
    }
    static std::shared_ptr<TestWSocket> Create(asio::io_context &io_executor, asio::ip::tcp::socket &&socket) {
        return std::shared_ptr<TestWSocket>(new TestWSocket(io_executor, std::move(socket)));
    }

    ~TestWSocket() override {}

private:
    void                  OnError(std::error_code code) override { std::cout << "OnError: " << code << std::endl; }
    wsocket::CompressType OnHandshake(const std::vector<wsocket::CompressType> &surpported_compress_type) override {
        std::cout << "OnOnHandshake" << std::endl;
        return wsocket::CompressType::None;
    }
    void OnConnected() override {
        std::cout << "OnConnected" << std::endl;
        this->Text("hello");
    }
    void OnClose(int16_t code, const std::string &reason) override {
        std::cout << "OnClose: " << code << ":" << reason << std::endl;
        if(!executor_.stopped()) {
            executor_.stop();
        }
    }
    void OnPing() override {
        std::cout << "OnPing" << std::endl;
        wsocket::WSocket::OnPing();
        this->Text("hello");
    }
    void OnPong() override { std::cout << "OnPong" << std::endl; }
    void OnText(std::string_view text, bool finish) override {
        std::cout << "OnText:" << text << std::endl;
        this->Close(wsocket::CloseCode::CLOSE_NORMAL);
    }
    void OnBinary(wsocket::Buffer buffer, bool finish) override {
        std::cout << "OnBinary:" << std::string(reinterpret_cast<char *>(buffer.buf), buffer.size) << std::endl;
    }
    asio::io_context &executor_;
};

void test_asio_wsocket() {
    std::cout << "================== test_asio_wsocket start ==================" << std::endl;
    asio::io_context io_executor;

    using tcp = asio::ip::tcp;
    tcp::acceptor server(io_executor, asio::ip::tcp::v4());
    server.bind(tcp::endpoint(asio::ip::tcp::v4(), 12000));
    server.listen();
    server.async_accept([&](asio::error_code ec, asio::ip::tcp::socket peer) {
        if(ec) {
            std::cout << ec.message() << std::endl;
            return;
        }

        std::cout << peer.remote_endpoint().address().to_string() << ":" << peer.remote_endpoint().port() << std::endl;
        auto cli = TestWSocket::Create(io_executor, std::move(peer));
        cli->Start();
    });


    auto client = TestWSocket::Create(io_executor);
    client->Handshake(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), 12000));

    io_executor.run();
    std::cout << "================== test_asio_wsocket start ==================" << std::endl;
}
#endif

#ifdef ASIO_HAS_LOCAL_SOCKETS

class TestUnixWSocket : public wsocket::UnixWSocket {
protected:
    explicit TestUnixWSocket(asio::io_context &io_executor) :
        wsocket::UnixWSocket(io_executor.get_executor()), executor_(io_executor) {}
    explicit TestUnixWSocket(asio::io_context &io_executor, asio::local::stream_protocol::socket &&socket) :
        wsocket::UnixWSocket(std::move(socket)), executor_(io_executor) {}

public:
    static std::shared_ptr<TestUnixWSocket> Create(asio::io_context &io_executor) {
        return std::shared_ptr<TestUnixWSocket>(new TestUnixWSocket(io_executor));
    }
    static std::shared_ptr<TestUnixWSocket> Create(asio::io_context                      &io_executor,
                                                   asio::local::stream_protocol::socket &&socket) {
        return std::shared_ptr<TestUnixWSocket>(new TestUnixWSocket(io_executor, std::move(socket)));
    }

    ~TestUnixWSocket() override {}

private:
    void OnError(std::error_code code) override {
        std::cout << "OnError: " << code << ":" << code.message() << std::endl;
    }
    wsocket::CompressType OnHandshake(const std::vector<wsocket::CompressType> &surpported_compress_type) override {
        std::cout << "OnHandshake" << std::endl;
        return wsocket::CompressType::None;
    }
    void OnConnected() override {
        std::cout << "OnConnected" << std::endl;
        this->Text("hello");
    }
    void OnClose(int16_t code, const std::string &reason) override {
        std::cout << "OnClose: " << code << ":" << reason << std::endl;
        if(!executor_.stopped()) {
            executor_.stop();
        }
    }
    void OnPing() override {
        std::cout << "OnPing" << std::endl;
        wsocket::UnixWSocket::OnPing();
        this->Text("test_asio_unix_wsocket hello");
    }
    void OnPong() override { std::cout << "OnPong" << std::endl; }
    void OnText(std::string_view text, bool finish) override {
        std::cout << "OnText:" << text << std::endl;
        this->Close(wsocket::CloseCode::CLOSE_NORMAL);
    }
    void OnBinary(wsocket::Buffer buffer, bool finish) override {
        std::cout << "OnBinary:" << std::string(reinterpret_cast<char *>(buffer.buf), buffer.size) << std::endl;
    }

    asio::io_context &executor_;
};
void test_asio_unix_wsocket() {
    asio::io_context io_executor;

    using unix_socket = asio::local::stream_protocol;
    unix_socket::acceptor server(io_executor);
    server.open();

#ifdef _WIN32
    std::string path = "H:/Code/CLion/WSocket/test_unix_wsocket.sock";
#else
    std::string path = "/tmp/test_unix_wsocket.sock";
#endif

    std::remove(path.c_str());

    asio::error_code ec;
    server.bind(unix_socket::endpoint(path), ec);
    if(ec) {
        std::cout << "bind error: " << ec.message() << std::endl;
        return;
    }
    server.listen();

    server.async_accept([&](asio::error_code ec, unix_socket::socket peer) {
        if(ec) {
            std::cout << "accept error: " << ec.message() << std::endl;
            return;
        }

        std::cout << "remote path:" << peer.remote_endpoint().path() << std::endl;
        auto cli = TestUnixWSocket::Create(io_executor, std::move(peer));
        cli->Start();
    });


    auto client = TestUnixWSocket::Create(io_executor);
    client->Handshake(unix_socket::endpoint(path));

    io_executor.run();
    server.close();
    std::remove(path.c_str());
}
#endif

#ifdef WITH_ZSTD

class TestZstdWSocket : public wsocket::WSocket {
protected:
    explicit TestZstdWSocket(asio::io_context &io_executor) :
        wsocket::WSocket(io_executor.get_executor()), executor_(io_executor) {}
    explicit TestZstdWSocket(asio::io_context &io_executor, asio::ip::tcp::socket &&socket) :
        wsocket::WSocket(std::move(socket)), executor_(io_executor) {}

public:
    static std::shared_ptr<TestZstdWSocket> Create(asio::io_context &io_executor) {
        return std::shared_ptr<TestZstdWSocket>(new TestZstdWSocket(io_executor));
    }
    static std::shared_ptr<TestZstdWSocket> Create(asio::io_context &io_executor, asio::ip::tcp::socket &&socket) {
        return std::shared_ptr<TestZstdWSocket>(new TestZstdWSocket(io_executor, std::move(socket)));
    }

    ~TestZstdWSocket() override {}

    inline static const char test_text[] =
            "这是一段用于测试压缩算法的文本内容。它包含了一些重复的模式，比如：测试测试、压缩压缩、算法算法。\n"
            "这些重复的模式应该能够被压缩算法有效地处理，从而展现出压缩效果。\n"
            "同时，文本中也包含了一些随机性内容，比如数字: 12345, 67890, 24680, 13579。\n"
            "还有各种标点符号！，？；：‘’\"\"（）【】《》。\n"
            "这段文本足够长，以确保压缩算法有足够的数据来工作，而不会因为头部开销而导致压缩后变大。\n"
            "希望这段文本能帮助您测试Zstd压缩库的性能和效果。\n"
            "您可以多次复制这段文本以增加长度，从而更好地测试压缩率。\n"
            "这是第一部分的结束。\n\n"
            "现在是第二部分的内容，添加更多文本以提高压缩测试的效果。\n"
            "重复的词语可以帮助压缩算法找到模式：数据数据、测试测试、压缩压缩。\n"
            "中文文本中的重复模式：中文中文、文本文本、压缩压缩、算法算法。\n"
            "添加一些技术术语：Zstandard压缩算法、无损数据压缩、LZ77算法、霍夫曼编码。\n"
            "再添加一些英文内容以增加多样性：This is some English text for testing compression algorithms.\n"
            "More English content: Zstd is a fast lossless compression algorithm, targeting real-time "
            "compression scenarios.\n"
            "最后再添加一些数字序列：111222333444555, 999888777666555, 12345678901234567890。\n"
            "这是最后一行，测试文本结束了。希望这段文本足够长，能够有效地测试压缩算法的性能。"
            "这是一段用于测试Zstd压缩算法的文本内容。它包含了许多重复的模式和词语，比如：测试测试、压缩压缩、算法算法、"
            "数据数据、性能性能。\n"
            "重复的词语可以帮助压缩算法找到模式：测试测试、压缩压缩、算法算法、数据数据、性能性能、编码编码、解码解码、"
            "传输传输。\n"
            "Zstd压缩算法是一种快速的无损数据压缩算法，由Facebook开发并开源。Zstd压缩算法是一种快速的无损数据压缩算法。"
            "\n"
            "重复的句子结构：这是一段测试文本，这是一段测试文本，这是一段测试文本。压缩算法压缩算法压缩算法。\n"
            "数字序列重复：1234567890, 1234567890, 1234567890, 1234567890, 1234567890, 1234567890。\n"
            "长重复模式：ABCDEFGHIJKLMNOPQRSTUVWXYZ, ABCDEFGHIJKLMNOPQRSTUVWXYZ, ABCDEFGHIJKLMNOPQRSTUVWXYZ。\n"
            "中文重复：中文中文中文中文中文中文中文中文中文中文中文中文中文中文中文中文中文中文中文中文。\n"
            "技术术语重复：Zstandard压缩算法、无损数据压缩、LZ77算法、霍夫曼编码、熵编码、字典压缩。\n"
            "Zstandard压缩算法、无损数据压缩、LZ77算法、霍夫曼编码、熵编码、字典压缩。\n"
            "更多重复：测试压缩算法、测试压缩算法、测试压缩算法、测试压缩算法、测试压缩算法、测试压缩算法。\n"
            "长文本有助于提高压缩率，长文本有助于提高压缩率，长文本有助于提高压缩率，长文本有助于提高压缩率。\n"
            "最后一段包含大量重复词语：压缩压缩压缩压缩压缩压缩压缩压缩压缩压缩压缩压缩压缩压缩压缩压缩。\n"
            "算法算法算法算法算法算法算法算法算法算法算法算法算法算法算法算法算法算法算法算法算法算法。\n"
            "数据数据数据数据数据数据数据数据数据数据数据数据数据数据数据数据数据数据数据数据数据数据。\n"
            "性能性能性能性能性能性能性能性能性能性能性能性能性能性能性能性能性能性能性能性能性能性能。\n"
            "测试测试测试测试测试测试测试测试测试测试测试测试测试测试测试测试测试测试测试测试测试测试。\n"
            "结束结束结束结束结束结束结束结束结束结束结束结束结束结束结束结束结束结束结束结束结束结束。";

private:
    void OnError(std::error_code code) override {
        std::cout << "OnError: " << code << " " << code.message() << std::endl;
    }
    wsocket::CompressType OnHandshake(const std::vector<wsocket::CompressType> &request_compress_type) override {
        std::cout << "OnHandshake, request compress type: count:" << request_compress_type.size() << "\n:";
        for(auto type : request_compress_type) {
            std::cout << " " << int(type);
        }
        std::cout << std::endl;

        if(request_compress_type.empty()) {
            std::cerr << "no supported compress type" << std::endl;
            return wsocket::CompressType::None;
        }

        return request_compress_type[0];
    }
    void OnConnected() override {
        std::cout << "OnConnected" << std::endl;
        this->Text(test_text);
    }
    void OnClose(int16_t code, const std::string &reason) override {
        std::cout << "OnClose: " << code << ":" << reason << std::endl;
        if(!executor_.stopped()) {
            executor_.stop();
        }
    }
    void OnPing() override {
        std::cout << "OnPing" << std::endl;
        wsocket::WSocket::OnPing();
        this->Text(test_text);
    }
    void OnPong() override { std::cout << "OnPong" << std::endl; }
    void OnText(std::string_view text, bool finish) override {
        std::cout << "OnText:" << text << std::endl;
        assert(text == test_text);
        this->Close(wsocket::CloseCode::CLOSE_NORMAL);
    }
    void OnBinary(wsocket::Buffer buffer, bool finish) override {
        std::cout << "OnBinary:" << std::string(reinterpret_cast<char *>(buffer.buf), buffer.size) << std::endl;
    }

    asio::io_context &executor_;
};

void test_asio_wsocket_zstd() {
    std::cout << "================== test_asio_wsocket_zstd ==================" << std::endl;
    asio::io_context io_executor;

    using tcp = asio::ip::tcp;
    tcp::acceptor server(io_executor, asio::ip::tcp::v4());
    server.bind(tcp::endpoint(asio::ip::tcp::v4(), 12000));
    server.listen();
    server.async_accept([&](asio::error_code ec, asio::ip::tcp::socket peer) {
        if(ec) {
            std::cout << ec.message() << std::endl;
            return;
        }

        std::cout << peer.remote_endpoint().address().to_string() << ":" << peer.remote_endpoint().port() << std::endl;
        auto cli = TestZstdWSocket::Create(io_executor, std::move(peer));
        cli->Start();
    });


    auto client = TestZstdWSocket::Create(io_executor);
    client->Handshake(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), 12000));

    io_executor.run();
    std::cout << "================== test_asio_wsocket_zstd ==================" << std::endl;
}
#endif

int main() {
    std::cout << "================== start ==================" << std::endl;
    try {
        testBasicHeader();
        testFrameHeader();
        test_SlidingBuffer();
        test_WSocketContext();
        test_asio_wsocket();
        test_asio_unix_wsocket();
        test_asio_wsocket_zstd();
    } catch(const std::exception &e) {
        std::cout << "exception: " << e.what() << std::endl;
    }
    std::cout << "================== end ==================" << std::endl;
    return 0;
}
