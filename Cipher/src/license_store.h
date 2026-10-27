#pragma once
#include "license_crypto.h"
#include <string>
#include <vector>

namespace LicenseStore {

struct Record {
    std::string key;
    License::KeyInfo info;
};

bool Load(const std::wstring& path, std::vector<Record>& out);
bool Save(const std::wstring& path, const std::vector<Record>& recs);

uint32_t NextId(const std::vector<Record>& recs);
std::vector<uint32_t> RevokedIds(const std::vector<Record>& recs);

bool CheckAdminPassword(const std::string& password);

} // namespace LicenseStore
