#include "chat.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

constexpr const char* kExitCommand = "quit";
const char* GetRoleName(bool isServer) {
    return isServer ? "Server" : "Client";
}

}  // namespace

Chat::Chat(ChatConfig config)
    : config_(std::move(config)), des_(config_.key) {
    if (config_.key.size() != 8) {
        throw std::runtime_error("DES key must be exactly 8 bytes.");
    }
}

Chat::~Chat() {
    Close();
}

void Chat::Run() {
    if (config_.isServer) {
        RunServer();
        return;
    }
    RunClient();
}

void Chat::RunServer() {
    std::cout << "Role: [Server]\n";
    if (!CreateServer()) {
        return;
    }

    std::cout << "Server is listening on port " << config_.serverPort << ".\n";
    std::cout << "Waiting for client...\n";

    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);
    peerSocket_ = accept(serverSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
    if (peerSocket_ < 0) {
        std::cerr << "accept failed: " << std::strerror(errno) << '\n';
        Close();
        return;
    }

    char clientIp[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
    std::cout << "Client connected: " << clientIp << ':' << ntohs(clientAddr.sin_port) << '\n';

    isRunning_ = true;
    receiveThread_ = std::thread(&Chat::ReceiveLoop, this);
    SendLoop();
    Close();
}

void Chat::RunClient() {
    std::cout << "Role: [Client]\n";
    if (!ConnectToServer()) {
        return;
    }

    std::cout << "Connected to " << config_.serverIp << ':' << config_.serverPort << '\n';
    isRunning_ = true;
    receiveThread_ = std::thread(&Chat::ReceiveLoop, this);
    SendLoop();
    Close();
}

void Chat::SendLoop() {
    const char* selfName = GetRoleName(config_.isServer);
    std::cout << "Input messages and press Enter. Type 'quit' to exit.\n";

    std::string message;
    while (isRunning_) {
        std::cout << '[' << selfName << "]> " << std::flush;
        if (!std::getline(std::cin, message)) {
            isRunning_ = false;
            break;
        }

        if (message == kExitCommand) {
            std::cout << selfName << " is closing the connection.\n";
            isRunning_ = false;
            break;
        }

        const auto cipherText = des_.EncryptString(message);
        if (config_.showCipherHex) {
            std::cout << "[cipher hex] " << ToHex(cipherText) << '\n';
        }

        if (!SendPacket(cipherText)) {
            std::cerr << "send failed or peer disconnected.\n";
            isRunning_ = false;
            break;
        }
    }
}

void Chat::ReceiveLoop() {
    const char* peerName = config_.isServer ? "Client" : "Server";
    const char* selfName = GetRoleName(config_.isServer);
    while (isRunning_) {
        std::vector<std::uint8_t> cipherText;
        if (!ReceivePacket(cipherText)) {
            if (isRunning_) {
                std::cerr << peerName << " disconnected.\n";
            }
            isRunning_ = false;
            break;
        }

        try {
            const std::string plainText = des_.DecryptToString(cipherText);
            if (config_.showCipherHex) {
                std::cout << '[' << peerName << " cipher hex] " << ToHex(cipherText) << '\n';
            }
            std::cout << peerName << ": " << plainText << '\n';

            std::cout << '[' << selfName << "]> " << std::flush;
        } catch (const std::exception& ex) {
            std::cerr << "decrypt failed: " << ex.what() << '\n';
            isRunning_ = false;
            break;
        }
    }
}

bool Chat::CreateServer() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "socket failed: " << std::strerror(errno) << '\n';
        return false;
    }

    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(config_.serverPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "bind failed: " << std::strerror(errno) << '\n';
        Close();
        return false;
    }

    if (listen(serverSocket_, 1) < 0) {
        std::cerr << "listen failed: " << std::strerror(errno) << '\n';
        Close();
        return false;
    }

    return true;
}

bool Chat::ConnectToServer() {
    peerSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (peerSocket_ < 0) {
        std::cerr << "socket failed: " << std::strerror(errno) << '\n';
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(config_.serverPort);
    if (inet_pton(AF_INET, config_.serverIp.c_str(), &serverAddr.sin_addr) != 1) {
        std::cerr << "invalid server ip.\n";
        Close();
        return false;
    }

    if (connect(peerSocket_, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "connect failed: " << std::strerror(errno) << '\n';
        Close();
        return false;
    }

    return true;
}

bool Chat::SendPacket(const std::vector<std::uint8_t>& cipherText) {
    const std::uint32_t payloadLength = static_cast<std::uint32_t>(cipherText.size());
    const std::uint32_t networkLength = htonl(payloadLength);

    if (!SendAll(reinterpret_cast<const std::uint8_t*>(&networkLength), sizeof(networkLength))) {
        return false;
    }
    if (payloadLength == 0) {
        return true;
    }
    return SendAll(cipherText.data(), cipherText.size());
}

bool Chat::ReceivePacket(std::vector<std::uint8_t>& cipherText) {
    std::uint32_t networkLength = 0;
    if (!RecvAll(reinterpret_cast<std::uint8_t*>(&networkLength), sizeof(networkLength))) {
        return false;
    }

    const std::uint32_t payloadLength = ntohl(networkLength);
    if (payloadLength == 0 || payloadLength > 4096 || payloadLength % 8 != 0) {
        std::cerr << "invalid packet length: " << payloadLength << '\n';
        return false;
    }

    cipherText.assign(payloadLength, 0);
    return RecvAll(cipherText.data(), cipherText.size());
}

bool Chat::SendAll(const std::uint8_t* data, std::size_t length) {
    std::size_t sent = 0;
    while (sent < length) {
        const ssize_t result = send(peerSocket_, data + sent, length - sent, 0);
        if (result <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(result);
    }
    return true;
}

bool Chat::RecvAll(std::uint8_t* data, std::size_t length) {
    std::size_t received = 0;
    while (received < length) {
        const ssize_t result = recv(peerSocket_, data + received, length - received, 0);
        if (result <= 0) {
            return false;
        }
        received += static_cast<std::size_t>(result);
    }
    return true;
}

void Chat::Close() {
    const bool wasRunning = isRunning_.exchange(false);

    if (peerSocket_ >= 0) {
        shutdown(peerSocket_, SHUT_RDWR);
        close(peerSocket_);
        peerSocket_ = -1;
    }
    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }

    if (receiveThread_.joinable()) {
        if (wasRunning || !isRunning_) {
            receiveThread_.join();
        }
    }
}

std::string Chat::ToHex(const std::vector<std::uint8_t>& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < data.size(); ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
        if (i + 1 != data.size()) {
            oss << ' ';
        }
    }
    return oss.str();
}
