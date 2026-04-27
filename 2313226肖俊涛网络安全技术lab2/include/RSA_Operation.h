#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct RsaPublicKey {
    std::uint64_t e = 0;
    std::uint64_t n = 0;
};

struct RsaPrivateKey {
    std::uint64_t d = 0;
    std::uint64_t n = 0;
};

class RsaOperation {
public:
    RsaOperation();

    void GenerateKeyPair();
    const RsaPublicKey& GetPublicKey() const;
    const RsaPrivateKey& GetPrivateKey() const;

    static std::uint64_t EncryptBlock(std::uint64_t plain, const RsaPublicKey& key);
    static std::uint64_t DecryptBlock(std::uint64_t cipher, const RsaPrivateKey& key);
    static std::vector<std::uint64_t> EncryptText(const std::string& plain, const RsaPublicKey& key);
    static std::string DecryptText(const std::vector<std::uint64_t>& cipher, const RsaPrivateKey& key);
    static std::string GenerateRandomDesKey();

private:
    RsaPublicKey publicKey_{};
    RsaPrivateKey privateKey_{};

    static std::uint64_t ModPow(std::uint64_t base, std::uint64_t exp, std::uint64_t mod);
    static std::uint64_t Gcd(std::uint64_t a, std::uint64_t b);
    static std::int64_t ExtendedGcd(std::int64_t a, std::int64_t b, std::int64_t& x, std::int64_t& y);
    static std::uint64_t ModInverse(std::uint64_t a, std::uint64_t mod);
    static std::vector<std::uint32_t> BuildPrimeTable();
};
