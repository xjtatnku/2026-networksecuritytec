#pragma once

#include "DES_Operation.h"
#include "RSA_Operation.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

struct ChatConfig {
    bool isServer = false;
    std::string serverIp = "127.0.0.1";
    std::uint16_t serverPort = 8888;
    bool showCipherHex = false;
};

class Chat {
public:
    explicit Chat(ChatConfig config);
    ~Chat();

    void Run();

private:
    enum class PacketType : std::uint8_t {
        kRsaPublicKey = 1,
        kEncryptedDesKey = 2,
        kChatCipherText = 3,
    };

    ChatConfig config_;
    int serverSocket_ = -1;
    int peerSocket_ = -1;
    std::atomic<bool> isRunning_{false};
    std::thread receiveThread_;
    DesOperation des_;
    RsaOperation rsa_;
    std::string sessionKey_;

    void RunServer();
    void RunClient();
    void SendLoop();
    void ReceiveLoop();
    void Close();

    bool PerformServerHandshake();
    bool PerformClientHandshake();
    bool CreateServer();
    bool ConnectToServer();
    bool SendFrame(PacketType type, const std::vector<std::uint8_t>& payload);
    bool ReceiveFrame(PacketType& type, std::vector<std::uint8_t>& payload);
    bool SendAll(const std::uint8_t* data, std::size_t length);
    bool RecvAll(std::uint8_t* data, std::size_t length);

    static std::vector<std::uint8_t> SerializePublicKey(const RsaPublicKey& publicKey);
    static bool DeserializePublicKey(const std::vector<std::uint8_t>& payload, RsaPublicKey& publicKey);
    static std::vector<std::uint8_t> SerializeEncryptedKey(const std::vector<std::uint64_t>& encryptedKey);
    static bool DeserializeEncryptedKey(const std::vector<std::uint8_t>& payload,
                                       std::vector<std::uint64_t>& encryptedKey);
    static std::string BytesToString(const std::vector<std::uint8_t>& bytes);
    static std::vector<std::uint8_t> StringToBytes(const std::string& text);
    static std::string ToHex(const std::vector<std::uint8_t>& data);
};
