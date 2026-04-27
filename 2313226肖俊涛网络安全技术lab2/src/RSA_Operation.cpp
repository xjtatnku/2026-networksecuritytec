#include "RSA_Operation.h"

#include <random>
#include <stdexcept>
#include <vector>

namespace {

constexpr std::uint64_t kPreferredPublicExponent = 65537;
constexpr char kDesAlphabet[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

}  // namespace

RsaOperation::RsaOperation() {
    GenerateKeyPair();
}

void RsaOperation::GenerateKeyPair() {
    const auto primes = BuildPrimeTable();
    if (primes.size() < 2) {
        throw std::runtime_error("Failed to build RSA prime table.");
    }

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<std::size_t> dist(0, primes.size() - 1);

    while (true) {
        const std::uint64_t p = primes[dist(generator)];
        std::uint64_t q = primes[dist(generator)];
        while (q == p) {
            q = primes[dist(generator)];
        }

        const std::uint64_t n = p * q;
        const std::uint64_t phi = (p - 1) * (q - 1);

        std::uint64_t e = kPreferredPublicExponent;
        if (Gcd(e, phi) != 1) {
            e = 3;
            while (e < phi && Gcd(e, phi) != 1) {
                e += 2;
            }
        }
        if (e >= phi || Gcd(e, phi) != 1) {
            continue;
        }

        const std::uint64_t d = ModInverse(e, phi);
        if (d == 0) {
            continue;
        }

        publicKey_ = {e, n};
        privateKey_ = {d, n};
        return;
    }
}

const RsaPublicKey& RsaOperation::GetPublicKey() const {
    return publicKey_;
}

const RsaPrivateKey& RsaOperation::GetPrivateKey() const {
    return privateKey_;
}

std::uint64_t RsaOperation::EncryptBlock(std::uint64_t plain, const RsaPublicKey& key) {
    if (plain >= key.n) {
        throw std::runtime_error("RSA plaintext block is too large for the modulus.");
    }
    return ModPow(plain, key.e, key.n);
}

std::uint64_t RsaOperation::DecryptBlock(std::uint64_t cipher, const RsaPrivateKey& key) {
    return ModPow(cipher, key.d, key.n);
}

std::vector<std::uint64_t> RsaOperation::EncryptText(const std::string& plain, const RsaPublicKey& key) {
    std::vector<std::uint64_t> cipher;
    cipher.reserve(plain.size());
    for (unsigned char ch : plain) {
        cipher.push_back(EncryptBlock(ch, key));
    }
    return cipher;
}

std::string RsaOperation::DecryptText(const std::vector<std::uint64_t>& cipher, const RsaPrivateKey& key) {
    std::string plain;
    plain.reserve(cipher.size());
    for (const auto value : cipher) {
        const auto decrypted = DecryptBlock(value, key);
        if (decrypted > 0xFFU) {
            throw std::runtime_error("RSA decrypted block is out of byte range.");
        }
        plain.push_back(static_cast<char>(decrypted));
    }
    return plain;
}

std::string RsaOperation::GenerateRandomDesKey() {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<std::size_t> dist(0, sizeof(kDesAlphabet) - 2);

    std::string key;
    key.reserve(8);
    for (int i = 0; i < 8; ++i) {
        key.push_back(kDesAlphabet[dist(generator)]);
    }
    return key;
}

std::uint64_t RsaOperation::ModPow(std::uint64_t base, std::uint64_t exp, std::uint64_t mod) {
    if (mod == 1) {
        return 0;
    }

    std::uint64_t result = 1 % mod;
    base %= mod;
    while (exp > 0) {
        if (exp & 1U) {
            result = static_cast<std::uint64_t>(
                (static_cast<unsigned __int128>(result) * base) % mod);
        }
        base = static_cast<std::uint64_t>(
            (static_cast<unsigned __int128>(base) * base) % mod);
        exp >>= 1U;
    }
    return result;
}

std::uint64_t RsaOperation::Gcd(std::uint64_t a, std::uint64_t b) {
    while (b != 0) {
        const std::uint64_t t = a % b;
        a = b;
        b = t;
    }
    return a;
}

std::int64_t RsaOperation::ExtendedGcd(std::int64_t a, std::int64_t b, std::int64_t& x, std::int64_t& y) {
    if (b == 0) {
        x = 1;
        y = 0;
        return a;
    }

    std::int64_t x1 = 0;
    std::int64_t y1 = 0;
    const std::int64_t gcd = ExtendedGcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return gcd;
}

std::uint64_t RsaOperation::ModInverse(std::uint64_t a, std::uint64_t mod) {
    std::int64_t x = 0;
    std::int64_t y = 0;
    const std::int64_t gcd = ExtendedGcd(static_cast<std::int64_t>(a),
                                         static_cast<std::int64_t>(mod), x, y);
    if (gcd != 1) {
        return 0;
    }

    const std::int64_t inverse = (x % static_cast<std::int64_t>(mod) +
                                  static_cast<std::int64_t>(mod)) %
                                 static_cast<std::int64_t>(mod);
    return static_cast<std::uint64_t>(inverse);
}

std::vector<std::uint32_t> RsaOperation::BuildPrimeTable() {
    constexpr std::uint32_t kLower = 30000;
    constexpr std::uint32_t kUpper = 65000;

    std::vector<bool> isPrime(kUpper + 1, true);
    isPrime[0] = false;
    isPrime[1] = false;

    for (std::uint32_t i = 2; i * i <= kUpper; ++i) {
        if (!isPrime[i]) {
            continue;
        }
        for (std::uint32_t j = i * i; j <= kUpper; j += i) {
            isPrime[j] = false;
        }
    }

    std::vector<std::uint32_t> primes;
    for (std::uint32_t i = kLower; i <= kUpper; ++i) {
        if (isPrime[i]) {
            primes.push_back(i);
        }
    }

    return primes;
}
