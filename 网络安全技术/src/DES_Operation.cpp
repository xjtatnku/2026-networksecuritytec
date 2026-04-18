#include "DES_Operation.h"

#include <algorithm>
#include <stdexcept>

const std::uint8_t DesOperation::IP[64] = {
    58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7};

const std::uint8_t DesOperation::IP_INV[64] = {
    40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9, 49, 17, 57, 25};

const std::uint8_t DesOperation::E[48] = {
    32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9, 8, 9, 10, 11,
    12, 13, 12, 13, 14, 15, 16, 17, 16, 17, 18, 19, 20, 21, 20, 21,
    22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1};

const std::uint8_t DesOperation::P[32] = {
    16, 7, 20, 21, 29, 12, 28, 17,
    1, 15, 23, 26, 5, 18, 31, 10,
    2, 8, 24, 14, 32, 27, 3, 9,
    19, 13, 30, 6, 22, 11, 4, 25};

const std::uint8_t DesOperation::PC1[56] = {
    57, 49, 41, 33, 25, 17, 9,
    1, 58, 50, 42, 34, 26, 18,
    10, 2, 59, 51, 43, 35, 27,
    19, 11, 3, 60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15,
    7, 62, 54, 46, 38, 30, 22,
    14, 6, 61, 53, 45, 37, 29,
    21, 13, 5, 28, 20, 12, 4};

const std::uint8_t DesOperation::PC2[48] = {
    14, 17, 11, 24, 1, 5, 3, 28,
    15, 6, 21, 10, 23, 19, 12, 4,
    26, 8, 16, 7, 27, 20, 13, 2,
    41, 52, 31, 37, 47, 55, 30, 40,
    51, 45, 33, 48, 44, 49, 39, 56,
    34, 53, 46, 42, 50, 36, 29, 32};

const std::uint8_t DesOperation::LS[16] = {
    1, 1, 2, 2, 2, 2, 2, 2,
    1, 2, 2, 2, 2, 2, 2, 1};

const std::uint8_t DesOperation::S[8][4][16] = {
    {
        {14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7},
        {0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8},
        {4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0},
        {15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13},
    },
    {
        {15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10},
        {3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5},
        {0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15},
        {13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9},
    },
    {
        {10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8},
        {13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1},
        {13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7},
        {1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12},
    },
    {
        {7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15},
        {13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9},
        {10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4},
        {3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14},
    },
    {
        {2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9},
        {14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6},
        {4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14},
        {11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3},
    },
    {
        {12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11},
        {10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8},
        {9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6},
        {4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13},
    },
    {
        {4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1},
        {13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6},
        {1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2},
        {6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12},
    },
    {
        {13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7},
        {1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2},
        {7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8},
        {2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11},
    }};

DesOperation::DesOperation() {
    SetKey("SecLab01");
}

DesOperation::DesOperation(const std::string& key) {
    SetKey(key);
}

void DesOperation::SetKey(const std::string& key) {
    key_.fill(0);
    for (std::size_t i = 0; i < key_.size() && i < key.size(); ++i) {
        key_[i] = static_cast<std::uint8_t>(key[i]);
    }
    GenerateSubKeys();
}

std::vector<std::uint8_t> DesOperation::EncryptString(const std::string& plainText) const {
    return EncryptBytes(std::vector<std::uint8_t>(plainText.begin(), plainText.end()));
}

std::string DesOperation::DecryptToString(const std::vector<std::uint8_t>& cipherText) const {
    const auto plainBytes = DecryptBytes(cipherText);
    return std::string(plainBytes.begin(), plainBytes.end());
}

std::vector<std::uint8_t> DesOperation::EncryptBytes(const std::vector<std::uint8_t>& plainText) const {
    std::vector<std::uint8_t> padded = plainText;
    std::uint8_t padding = static_cast<std::uint8_t>(8 - (padded.size() % 8));
    if (padding == 0) {
        padding = 8;
    }
    padded.insert(padded.end(), padding, padding);

    std::vector<std::uint8_t> cipherText(padded.size(), 0);
    for (std::size_t offset = 0; offset < padded.size(); offset += 8) {
        DESBlock(padded.data() + offset, cipherText.data() + offset, true);
    }
    return cipherText;
}

std::vector<std::uint8_t> DesOperation::DecryptBytes(const std::vector<std::uint8_t>& cipherText) const {
    if (cipherText.empty() || cipherText.size() % 8 != 0) {
        throw std::runtime_error("Invalid DES cipher text length.");
    }

    std::vector<std::uint8_t> plainText(cipherText.size(), 0);
    for (std::size_t offset = 0; offset < cipherText.size(); offset += 8) {
        DESBlock(cipherText.data() + offset, plainText.data() + offset, false);
    }

    const std::uint8_t padding = plainText.back();
    if (padding == 0 || padding > 8 || padding > plainText.size()) {
        throw std::runtime_error("Invalid DES padding.");
    }
    for (std::size_t i = plainText.size() - padding; i < plainText.size(); ++i) {
        if (plainText[i] != padding) {
            throw std::runtime_error("Corrupted DES padding.");
        }
    }
    plainText.resize(plainText.size() - padding);
    return plainText;
}

void DesOperation::GenerateSubKeys() {
    std::uint8_t keyBits[64] = {0};
    ByteToBit(key_.data(), keyBits, 8);

    std::array<std::uint8_t, 28> left{};
    std::array<std::uint8_t, 28> right{};
    for (int i = 0; i < 28; ++i) {
        left[i] = keyBits[PC1[i] - 1];
        right[i] = keyBits[PC1[i + 28] - 1];
    }

    for (int round = 0; round < 16; ++round) {
        std::rotate(left.begin(), left.begin() + LS[round], left.end());
        std::rotate(right.begin(), right.begin() + LS[round], right.end());

        std::uint8_t subKeyBits[48] = {0};
        for (int i = 0; i < 48; ++i) {
            const int index = PC2[i] - 1;
            subKeyBits[i] = (index < 28) ? left[index] : right[index - 28];
        }
        BitToByte(subKeyBits, subKeys_[round].data(), 6);
    }
}

void DesOperation::F(const std::array<std::uint8_t, 32>& right,
                     const std::array<std::uint8_t, 6>& subKey,
                     std::array<std::uint8_t, 32>& result) const {
    std::uint8_t expanded[48] = {0};
    for (int i = 0; i < 48; ++i) {
        expanded[i] = right[E[i] - 1];
    }

    std::uint8_t subKeyBits[48] = {0};
    ByteToBit(subKey.data(), subKeyBits, 6);
    Xor(expanded, subKeyBits, 48);

    std::uint8_t sBoxOutput[32] = {0};
    for (int box = 0; box < 8; ++box) {
        const int offset = box * 6;
        const int row = expanded[offset] * 2 + expanded[offset + 5];
        const int col = expanded[offset + 1] * 8 +
                        expanded[offset + 2] * 4 +
                        expanded[offset + 3] * 2 +
                        expanded[offset + 4];
        const std::uint8_t value = S[box][row][col];
        for (int bit = 0; bit < 4; ++bit) {
            sBoxOutput[box * 4 + bit] = (value >> (3 - bit)) & 0x01U;
        }
    }

    for (int i = 0; i < 32; ++i) {
        result[i] = sBoxOutput[P[i] - 1];
    }
}

void DesOperation::DESBlock(const std::uint8_t* input, std::uint8_t* output, bool isEncrypt) const {
    std::uint8_t inputBits[64] = {0};
    ByteToBit(input, inputBits, 8);

    std::uint8_t permuted[64] = {0};
    for (int i = 0; i < 64; ++i) {
        permuted[i] = inputBits[IP[i] - 1];
    }

    std::array<std::uint8_t, 32> left{};
    std::array<std::uint8_t, 32> right{};
    for (int i = 0; i < 32; ++i) {
        left[i] = permuted[i];
        right[i] = permuted[i + 32];
    }

    for (int round = 0; round < 16; ++round) {
        std::array<std::uint8_t, 32> previousRight = right;
        std::array<std::uint8_t, 32> fResult{};
        const auto& subKey = subKeys_[isEncrypt ? round : (15 - round)];
        F(right, subKey, fResult);

        for (int i = 0; i < 32; ++i) {
            right[i] = left[i] ^ fResult[i];
        }
        left = previousRight;
    }

    std::uint8_t preOutput[64] = {0};
    for (int i = 0; i < 32; ++i) {
        preOutput[i] = right[i];
        preOutput[i + 32] = left[i];
    }

    std::uint8_t finalBits[64] = {0};
    for (int i = 0; i < 64; ++i) {
        finalBits[i] = preOutput[IP_INV[i] - 1];
    }

    BitToByte(finalBits, output, 8);
}

void DesOperation::ByteToBit(const std::uint8_t* bytes, std::uint8_t* bits, int byteLength) {
    for (int i = 0; i < byteLength; ++i) {
        for (int bit = 0; bit < 8; ++bit) {
            bits[i * 8 + bit] = (bytes[i] >> (7 - bit)) & 0x01U;
        }
    }
}

void DesOperation::BitToByte(const std::uint8_t* bits, std::uint8_t* bytes, int byteLength) {
    for (int i = 0; i < byteLength; ++i) {
        bytes[i] = 0;
        for (int bit = 0; bit < 8; ++bit) {
            bytes[i] |= static_cast<std::uint8_t>(bits[i * 8 + bit] << (7 - bit));
        }
    }
}

void DesOperation::Xor(std::uint8_t* left, const std::uint8_t* right, int length) {
    for (int i = 0; i < length; ++i) {
        left[i] ^= right[i];
    }
}
