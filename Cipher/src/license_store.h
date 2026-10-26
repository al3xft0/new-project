#pragma once
#include "license_crypto.h"
#include <string>
#include <vector>

// Simple JSON-ish line store (no third-party JSON dep): keys.db next to admin exe

namespace LicenseStore {

struct Record {
    License::KeyInfo info;
};

bool Load(const std::wstring& path, std::vector<Record>& out);
bool Save(const std::wstring& path, const std::vector<Record>& recs);

// next free id
uint32_t NextId(const std::vector<Record>& recs);

std::vector<uint32_t> RevokedIds(const std::vector<Record>& recs);

// Admin password check (HMAC of password vs stored hash file or compile-time)
bool CheckAdminPassword(const std::string& password);

} // namespace LicenseStore
