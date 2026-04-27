#include "chat.h"

#include <exception>
#include <iostream>
#include <optional>
#include <string>

namespace {

void PrintUsage() {
    std::cout
        << "Usage:\n"
        << "  ./RSA_DES_chat server [--port 8888] [--show-cipher yes|no]\n"
        << "  ./RSA_DES_chat client [--ip 127.0.0.1] [--port 8888] [--show-cipher yes|no]\n";
}

bool ParseUnsignedShort(const std::string& text, std::uint16_t& value) {
    try {
        const auto parsed = std::stoul(text);
        if (parsed > 65535) {
            return false;
        }
        value = static_cast<std::uint16_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool ParseYesNo(const std::string& text, bool& value) {
    if (text == "yes" || text == "y" || text == "Y" || text == "YES") {
        value = true;
        return true;
    }
    if (text == "no" || text == "n" || text == "N" || text == "NO") {
        value = false;
        return true;
    }
    return false;
}

bool AskShowCipher() {
    while (true) {
        std::cout << "Show cipher text during chat? (yes/no): " << std::flush;
        std::string answer;
        if (!std::getline(std::cin, answer)) {
            return false;
        }

        bool showCipher = false;
        if (ParseYesNo(answer, showCipher)) {
            return showCipher;
        }

        std::cout << "Please input yes or no.\n";
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    const std::string firstArg = argv[1];
    if (firstArg == "--help" || firstArg == "-h") {
        PrintUsage();
        return 0;
    }

    ChatConfig config;
    const std::string mode = firstArg;
    if (mode == "server" || mode == "s") {
        config.isServer = true;
    } else if (mode == "client" || mode == "c") {
        config.isServer = false;
    } else {
        PrintUsage();
        return 1;
    }

    std::optional<bool> showCipherOption;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--ip" && i + 1 < argc) {
            config.serverIp = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            if (!ParseUnsignedShort(argv[++i], config.serverPort)) {
                std::cerr << "Invalid port.\n";
                return 1;
            }
        } else if (arg == "--show-cipher") {
            if (i + 1 < argc) {
                bool showCipher = false;
                if (ParseYesNo(argv[i + 1], showCipher)) {
                    config.showCipherHex = showCipher;
                    showCipherOption = showCipher;
                    ++i;
                } else {
                    config.showCipherHex = true;
                    showCipherOption = true;
                }
            } else {
                config.showCipherHex = true;
                showCipherOption = true;
            }
        } else if (arg == "--help" || arg == "-h") {
            PrintUsage();
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            PrintUsage();
            return 1;
        }
    }

    std::cout << "Current identity: " << (config.isServer ? "Server" : "Client") << '\n';
    if (showCipherOption.has_value()) {
        std::cout << "Show cipher text: " << (config.showCipherHex ? "yes" : "no") << '\n';
    } else {
        config.showCipherHex = AskShowCipher();
    }

    try {
        Chat chat(config);
        chat.Run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
