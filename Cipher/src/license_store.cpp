#include "license_store.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace LicenseStore {

static std::string Narrow(const std::wstring& w) {
    std::string s;
    for (wchar_t c : w) s.push_back((char)(c < 128 ? c : '?'));
    return s;
}

bool Load(const std::wstring& path, std::vector<Record>& out) {
    out.clear();
    std::ifstream f(Narrow(path));
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        // id|status|created|days|expires|key|note
        Record r;
        std::stringstream ss(line);
        std::string tok;
        std::vector<std::string> p;
        while (std::getline(ss, tok, '|')) p.push_back(tok);
        if (p.size() < 6) continue;
        r.info.id = (uint32_t)strtoul(p[0].c_str(), nullptr, 10);
        int st = atoi(p[1].c_str());
        r.info.status = (License::Status)st;
        r.info.createdUnix = (uint32_t)strtoul(p[2].c_str(), nullptr, 10);
        r.info.durationDays = (uint16_t)atoi(p[3].c_str());
        r.info.expiresUnix = (uint32_t)strtoul(p[4].c_str(), nullptr, 10);
        r.info.key = p[5];
        if (p.size() > 6) r.info.note = p[6];
        out.push_back(r);
    }
    return true;
}

bool Save(const std::wstring& path, const std::vector<Record>& recs) {
    std::ofstream f(Narrow(path), std::ios::trunc);
    if (!f) return false;
    f << "# id|status|created|days|expires|key|note\n";
    for (const auto& r : recs) {
        f << r.info.id << '|'
          << (int)r.info.status << '|'
          << r.info.createdUnix << '|'
          << r.info.durationDays << '|'
          << r.info.expiresUnix << '|'
          << r.info.key << '|'
          << r.info.note << '\n';
    }
    return true;
}

uint32_t NextId(const std::vector<Record>& recs) {
    uint32_t m = 1000;
    for (const auto& r : recs) if (r.info.id >= m) m = r.info.id + 1;
    return m;
}

std::vector<uint32_t> RevokedIds(const std::vector<Record>& recs) {
    std::vector<uint32_t> v;
    for (const auto& r : recs)
        if (r.info.status == License::Status::Revoked)
            v.push_back(r.info.id);
    return v;
}

bool CheckAdminPassword(const std::string& password) {
    // HMAC of password with secret; compare to HMAC of default or env
    uint8_t mac[32], expect[32];
    License::HmacSha256(License::kLicenseSecret, sizeof(License::kLicenseSecret),
                        (const uint8_t*)password.data(), password.size(), mac);
    License::HmacSha256(License::kLicenseSecret, sizeof(License::kLicenseSecret),
                        (const uint8_t*)License::kAdminPasswordDefault,
                        strlen(License::kAdminPasswordDefault), expect);
    return memcmp(mac, expect, 32) == 0;
}

} // namespace LicenseStore
