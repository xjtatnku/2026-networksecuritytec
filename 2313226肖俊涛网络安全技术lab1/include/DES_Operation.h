#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class DesOperation {
public:
    DesOperation();
    explicit DesOperation(const std::string& key);

    void SetKey(const std::string& key);

    std::vector<std::uint8_t> EncryptBytes(const std::vector<std::uint8_t>& plainText) const;
    std::vector<std::uint8_t> DecryptBytes(const std::vector<std::uint8_t>& cipherText) const;

    std::vector<std::uint8_t> EncryptString(const std::string& plainText) const;
    std::string DecryptToString(const std::vector<std::uint8_t>& cipherText) const;

private:
    std::array<std::uint8_t, 8> key_{};
    std::array<std::array<std::uint8_t, 6>, 16> subKeys_{};

    static const std::uint8_t IP[64];
    static const std::uint8_t IP_INV[64];
    static const std::uint8_t E[48];
    static const std::uint8_t P[32];
    static const std::uint8_t PC1[56];
    static const std::uint8_t PC2[48];
    static const std::uint8_t LS[16];
    static const std::uint8_t S[8][4][16];

    void GenerateSubKeys();
    void F(const std::array<std::uint8_t, 32>& right,
           const std::array<std::uint8_t, 6>& subKey,
           std::array<std::uint8_t, 32>& result) const;
    void DESBlock(const std::uint8_t* input, std::uint8_t* output, bool isEncrypt) const;

    static void ByteToBit(const std::uint8_t* bytes, std::uint8_t* bits, int byteLength);
    static void BitToByte(const std::uint8_t* bits, std::uint8_t* bytes, int byteLength);
    static void Xor(std::uint8_t* left, const std::uint8_t* right, int length);
};
