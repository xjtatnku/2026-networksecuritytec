#pragma once

#include "DES_Operation.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

struct ChatConfig {
    bool isServer = false;
    std::string serverIp = "127.0.0.1";
    std::uint16_t serverPort = 8888;
    std::string key = "SecLab01";
    bool showCipherHex = false;
};

class Chat {
public:
    explicit Chat(ChatConfig config);
    ~Chat();

    void Run();

private:
    ChatConfig config_;
    int serverSocket_ = -1;
    int peerSocket_ = -1;
    std::atomic<bool> isRunning_{false};
    std::thread receiveThread_;
    DesOperation des_;

    void RunServer();
    void RunClient();
    void SendLoop();
    void ReceiveLoop();
    void Close();

    bool CreateServer();
    bool ConnectToServer();
    bool SendPacket(const std::vector<std::uint8_t>& cipherText);
    bool ReceivePacket(std::vector<std::uint8_t>& cipherText);
    bool SendAll(const std::uint8_t* data, std::size_t length);
    bool RecvAll(std::uint8_t* data, std::size_t length);

    static std::string ToHex(const std::vector<std::uint8_t>& data);
};
