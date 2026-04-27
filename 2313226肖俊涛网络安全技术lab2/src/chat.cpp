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
#include <vector>

namespace {

constexpr const char* kExitCommand = "quit";

const char* GetRoleName(bool isServer) {
    return isServer ? "Server" : "Client";
}

void RemoveLastUtf8CodePoint(std::string& text) {
    if (text.empty()) {
        return;
    }

    text.pop_back();
    while (!text.empty() &&
           (static_cast<unsigned char>(text.back()) & 0xC0U) == 0x80U) {
        text.pop_back();
    }
}

bool TryConsumeUtf8Sequence(const std::string& text, std::size_t pos, std::size_t& length) {
    const auto ch = static_cast<unsigned char>(text[pos]);
    auto isContinuation = [&](std::size_t index) -> bool {
        return index < text.size() &&
               (static_cast<unsigned char>(text[index]) & 0xC0U) == 0x80U;
    };

    if (ch <= 0x7FU) {
        length = 1;
        return true;
    }
    if ((ch & 0xE0U) == 0xC0U) {
        if (ch < 0xC2U || !isContinuation(pos + 1)) {
            return false;
        }
        length = 2;
        return true;
    }
    if ((ch & 0xF0U) == 0xE0U) {
        if (!isContinuation(pos + 1) || !isContinuation(pos + 2)) {
            return false;
        }
        const auto b1 = static_cast<unsigned char>(text[pos + 1]);
        if ((ch == 0xE0U && b1 < 0xA0U) || (ch == 0xEDU && b1 >= 0xA0U)) {
            return false;
        }
        length = 3;
        return true;
    }
    if ((ch & 0xF8U) == 0xF0U) {
        if (ch > 0xF4U || !isContinuation(pos + 1) ||
            !isContinuation(pos + 2) || !isContinuation(pos + 3)) {
            return false;
        }
        const auto b1 = static_cast<unsigned char>(text[pos + 1]);
        if ((ch == 0xF0U && b1 < 0x90U) || (ch == 0xF4U && b1 > 0x8FU)) {
            return false;
        }
        length = 4;
        return true;
    }
    return false;
}

std::string SanitizeUtf8Text(const std::string& text) {
    std::string sanitized;
    sanitized.reserve(text.size());

    for (std::size_t i = 0; i < text.size();) {
        std::size_t length = 0;
        if (TryConsumeUtf8Sequence(text, i, length)) {
            sanitized.append(text, i, length);
            i += length;
            continue;
        }
        ++i;
    }

    return sanitized;
}

bool IsWhitespaceToken(const std::string& token) {
    return token == " " ||
           token == "\t" ||
           token == "\xC2\xA0" ||
           token == "\xE3\x80\x80";
}

std::string RemoveArtifactSpacesBetweenNonAscii(const std::string& text) {
    std::vector<std::string> tokens;
    for (std::size_t i = 0; i < text.size();) {
        std::size_t length = 0;
        if (!TryConsumeUtf8Sequence(text, i, length)) {
            ++i;
            continue;
        }
        tokens.emplace_back(text.substr(i, length));
        i += length;
    }

    std::string cleaned;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (!IsWhitespaceToken(tokens[i])) {
            cleaned += tokens[i];
            continue;
        }

        std::size_t prev = i;
        while (prev > 0 && IsWhitespaceToken(tokens[prev - 1])) {
            --prev;
        }
        std::size_t next = i + 1;
        while (next < tokens.size() && IsWhitespaceToken(tokens[next])) {
            ++next;
        }

        const bool hasPrev = prev > 0;
        const bool hasNext = next < tokens.size();
        const bool prevIsNonAscii = hasPrev && tokens[prev - 1].size() > 1;
        const bool nextIsNonAscii = hasNext && tokens[next].size() > 1;

        if (prevIsNonAscii && nextIsNonAscii) {
            continue;
        }

        cleaned += tokens[i];
    }

    return cleaned;
}

std::string NormalizeUserInput(const std::string& raw) {
    std::string normalized;
    normalized.reserve(raw.size());

    for (std::size_t i = 0; i < raw.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(raw[i]);

        if (ch == '\b' || ch == 0x7FU) {
            RemoveLastUtf8CodePoint(normalized);
            continue;
        }

        if (ch == 0x1BU) {
            if (i + 1 < raw.size()) {
                const unsigned char next = static_cast<unsigned char>(raw[i + 1]);
                if (next == '[') {
                    i += 2;
                    while (i < raw.size()) {
                        const unsigned char seq = static_cast<unsigned char>(raw[i]);
                        if (seq >= 0x40U && seq <= 0x7EU) {
                            break;
                        }
                        ++i;
                    }
                    continue;
                }
                if (next == 'O') {
                    ++i;
                    if (i + 1 < raw.size()) {
                        ++i;
                    }
                    continue;
                }
            }
            continue;
        }

        if (ch < 0x20U && ch != '\t') {
            continue;
        }

        normalized.push_back(static_cast<char>(ch));
    }

    return RemoveArtifactSpacesBetweenNonAscii(SanitizeUtf8Text(normalized));
}

}  // namespace

Chat::Chat(ChatConfig config)
    : config_(std::move(config)) {
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

    if (!PerformServerHandshake()) {
        Close();
        return;
    }

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
    if (!PerformClientHandshake()) {
        Close();
        return;
    }

    isRunning_ = true;
    receiveThread_ = std::thread(&Chat::ReceiveLoop, this);
    SendLoop();
    Close();
}

void Chat::SendLoop() {
    const char* selfName = GetRoleName(config_.isServer);
    std::cout << "Secure channel is ready. Input messages and press Enter. Type 'quit' to exit.\n";

    std::string message;
    while (isRunning_) {
        std::cout << '[' << selfName << "]> " << std::flush;
        if (!std::getline(std::cin, message)) {
            isRunning_ = false;
            break;
        }

        message = NormalizeUserInput(message);

        if (message == kExitCommand) {
            std::cout << selfName << " is closing the connection.\n";
            isRunning_ = false;
            break;
        }

        const auto cipherText = des_.EncryptString(message);
        if (config_.showCipherHex) {
            std::cout << "[cipher hex] " << ToHex(cipherText) << '\n';
        }

        if (!SendFrame(PacketType::kChatCipherText, cipherText)) {
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
        PacketType type = PacketType::kChatCipherText;
        std::vector<std::uint8_t> payload;
        if (!ReceiveFrame(type, payload)) {
            if (isRunning_) {
                std::cerr << peerName << " disconnected.\n";
            }
            isRunning_ = false;
            break;
        }

        try {
            if (type != PacketType::kChatCipherText) {
                throw std::runtime_error("Unexpected packet type received after RSA handshake.");
            }
            if (payload.empty() || payload.size() % 8 != 0) {
                throw std::runtime_error("Invalid DES ciphertext payload length.");
            }

            const std::string plainText = NormalizeUserInput(des_.DecryptToString(payload));
            if (config_.showCipherHex) {
                std::cout << '[' << peerName << " cipher hex] " << ToHex(payload) << '\n';
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

bool Chat::PerformServerHandshake() {
    rsa_.GenerateKeyPair();
    if (!SendFrame(PacketType::kRsaPublicKey, SerializePublicKey(rsa_.GetPublicKey()))) {
        std::cerr << "failed to send RSA public key.\n";
        return false;
    }

    PacketType type = PacketType::kRsaPublicKey;
    std::vector<std::uint8_t> payload;
    if (!ReceiveFrame(type, payload)) {
        std::cerr << "failed to receive encrypted DES key.\n";
        return false;
    }
    if (type != PacketType::kEncryptedDesKey) {
        std::cerr << "unexpected packet during server handshake.\n";
        return false;
    }

    std::vector<std::uint64_t> encryptedKey;
    if (!DeserializeEncryptedKey(payload, encryptedKey)) {
        std::cerr << "invalid encrypted DES key payload.\n";
        return false;
    }

    try {
        sessionKey_ = RsaOperation::DecryptText(encryptedKey, rsa_.GetPrivateKey());
    } catch (const std::exception& ex) {
        std::cerr << "failed to decrypt DES session key: " << ex.what() << '\n';
        return false;
    }

    if (sessionKey_.size() != 8) {
        std::cerr << "negotiated DES session key must be exactly 8 bytes.\n";
        return false;
    }

    des_.SetKey(sessionKey_);
    std::cout << "RSA handshake completed. DES session key was distributed automatically.\n";
    return true;
}

bool Chat::PerformClientHandshake() {
    PacketType type = PacketType::kRsaPublicKey;
    std::vector<std::uint8_t> payload;
    if (!ReceiveFrame(type, payload)) {
        std::cerr << "failed to receive RSA public key.\n";
        return false;
    }
    if (type != PacketType::kRsaPublicKey) {
        std::cerr << "unexpected packet during client handshake.\n";
        return false;
    }

    RsaPublicKey publicKey;
    if (!DeserializePublicKey(payload, publicKey)) {
        std::cerr << "invalid RSA public key payload.\n";
        return false;
    }

    sessionKey_ = RsaOperation::GenerateRandomDesKey();
    if (sessionKey_.size() != 8) {
        std::cerr << "generated DES session key must be exactly 8 bytes.\n";
        return false;
    }

    const auto encryptedKey = RsaOperation::EncryptText(sessionKey_, publicKey);
    if (!SendFrame(PacketType::kEncryptedDesKey, SerializeEncryptedKey(encryptedKey))) {
        std::cerr << "failed to send encrypted DES key.\n";
        return false;
    }

    des_.SetKey(sessionKey_);
    std::cout << "RSA handshake completed. DES session key was generated and shared automatically.\n";
    return true;
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

bool Chat::SendFrame(PacketType type, const std::vector<std::uint8_t>& payload) {
    const std::uint8_t typeByte = static_cast<std::uint8_t>(type);
    const std::uint32_t payloadLength = static_cast<std::uint32_t>(payload.size());
    const std::uint32_t networkLength = htonl(payloadLength);

    if (!SendAll(&typeByte, sizeof(typeByte))) {
        return false;
    }
    if (!SendAll(reinterpret_cast<const std::uint8_t*>(&networkLength), sizeof(networkLength))) {
        return false;
    }
    if (payloadLength == 0) {
        return true;
    }
    return SendAll(payload.data(), payload.size());
}

bool Chat::ReceiveFrame(PacketType& type, std::vector<std::uint8_t>& payload) {
    std::uint8_t typeByte = 0;
    if (!RecvAll(&typeByte, sizeof(typeByte))) {
        return false;
    }

    if (typeByte < static_cast<std::uint8_t>(PacketType::kRsaPublicKey) ||
        typeByte > static_cast<std::uint8_t>(PacketType::kChatCipherText)) {
        std::cerr << "invalid packet type: " << static_cast<int>(typeByte) << '\n';
        return false;
    }
    type = static_cast<PacketType>(typeByte);

    std::uint32_t networkLength = 0;
    if (!RecvAll(reinterpret_cast<std::uint8_t*>(&networkLength), sizeof(networkLength))) {
        return false;
    }

    const std::uint32_t payloadLength = ntohl(networkLength);
    if (payloadLength > 4096) {
        std::cerr << "invalid packet length: " << payloadLength << '\n';
        return false;
    }

    payload.assign(payloadLength, 0);
    if (payloadLength == 0) {
        return true;
    }
    return RecvAll(payload.data(), payload.size());
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

std::vector<std::uint8_t> Chat::SerializePublicKey(const RsaPublicKey& publicKey) {
    return StringToBytes(std::to_string(publicKey.e) + " " + std::to_string(publicKey.n));
}

bool Chat::DeserializePublicKey(const std::vector<std::uint8_t>& payload, RsaPublicKey& publicKey) {
    std::istringstream iss(BytesToString(payload));
    return static_cast<bool>(iss >> publicKey.e >> publicKey.n);
}

std::vector<std::uint8_t> Chat::SerializeEncryptedKey(const std::vector<std::uint64_t>& encryptedKey) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < encryptedKey.size(); ++i) {
        if (i != 0) {
            oss << ' ';
        }
        oss << encryptedKey[i];
    }
    return StringToBytes(oss.str());
}

bool Chat::DeserializeEncryptedKey(const std::vector<std::uint8_t>& payload,
                                   std::vector<std::uint64_t>& encryptedKey) {
    encryptedKey.clear();
    std::istringstream iss(BytesToString(payload));
    std::uint64_t value = 0;
    while (iss >> value) {
        encryptedKey.push_back(value);
    }

    if (encryptedKey.size() != 8) {
        return false;
    }
    return iss.eof() && !encryptedKey.empty();
}

std::string Chat::BytesToString(const std::vector<std::uint8_t>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

std::vector<std::uint8_t> Chat::StringToBytes(const std::string& text) {
    return std::vector<std::uint8_t>(text.begin(), text.end());
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
