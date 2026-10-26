#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <optional>

// Offline HMAC-SHA256 license tokens for Cipher.
// CHANGE kLicenseSecret before shipping. Admin + client must match.

namespace License {

// 32-byte secret — replace in both admin and client builds
inline constexpr uint8_t kLicenseSecret[32] = {
    0x43,0x69,0x70,0x68,0x65,0x72,0x4c,0x69,0x63,0x65,0x6e,0x73,0x65,0x4b,0x65,0x79,
    0x53,0x65,0x63,0x72,0x65,0x74,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39
};

// Admin gate secret (different string hashed) — change this too
inline constexpr char kAdminPasswordDefault[] = "change-me-admin-cipher";

enum class Status : uint8_t {
    Valid = 0,
    Expired = 1,
    Revoked = 2,
    Invalid = 3,
    BadFormat = 4
};

struct KeyInfo {
    std::string key;          // display form CIPHER-....
    uint32_t    id = 0;       // unique id embedded in payload
    uint32_t    createdUnix = 0;
    uint16_t    durationDays = 0;
    uint32_t    expiresUnix = 0;
    Status      status = Status::Invalid;
    std::string note;
};

// ---- sha256 / hmac (compact, no external dep) ----
void Sha256(const uint8_t* data, size_t len, uint8_t out[32]);
void HmacSha256(const uint8_t* key, size_t keyLen,
                const uint8_t* data, size_t dataLen, uint8_t out[32]);

std::string ToBase32(const uint8_t* data, size_t len);
bool FromBase32(const std::string& s, std::vector<uint8_t>& out);

// Build signed key: durationDays from now (or from createdUnix if non-zero)
std::string GenerateKey(uint32_t id, uint16_t durationDays, uint32_t createdUnix = 0);

// Parse + verify signature + expiry. revokedIds checked by caller or pass list.
Status ValidateKey(const std::string& keyStr, KeyInfo& out,
                   const std::vector<uint32_t>& revokedIds = {});

const char* StatusString(Status s);
// UI colors: Valid green, Revoked red, etc.
void StatusColor(Status s, float outRgba[4]);

// Normalize user input (strip spaces, upper)
std::string NormalizeKey(const std::string& in);

} // namespace License
