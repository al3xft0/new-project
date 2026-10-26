#include "memory.h"
#include <cstring>

std::optional<GameProcess> Mem::Attach(const wchar_t* exeName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return std::nullopt;

    PROCESSENTRY32W pe{ sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeName) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    if (!pid) return std::nullopt;

    GameProcess gp;
    gp.pid = pid;
    gp.name = exeName;
    gp.handle = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        FALSE, pid);
    if (!gp.handle) return std::nullopt;

    HANDLE msnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (msnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W me{ sizeof(me) };
        if (Module32FirstW(msnap, &me)) {
            do {
                if (_wcsicmp(me.szModule, exeName) == 0) {
                    gp.base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                    break;
                }
            } while (Module32NextW(msnap, &me));
        }
        CloseHandle(msnap);
    }
    if (!gp.base) {
        CloseHandle(gp.handle);
        return std::nullopt;
    }
    return gp;
}

void Mem::Close(GameProcess& p) {
    if (p.handle) { CloseHandle(p.handle); p.handle = nullptr; }
    p.pid = 0;
    p.base = 0;
}

bool Mem::IsValid(const GameProcess& p) {
    if (!p.handle || !p.pid) return false;
    DWORD code = 0;
    if (!GetExitCodeProcess(p.handle, &code)) return false;
    return code == STILL_ACTIVE;
}

bool Mem::ReadRaw(HANDLE h, uintptr_t addr, void* buf, size_t size) {
    if (!h || !addr || !buf || !size) return false;
    SIZE_T read = 0;
    return ReadProcessMemory(h, reinterpret_cast<LPCVOID>(addr), buf, size, &read) && read == size;
}

bool Mem::WriteRaw(HANDLE h, uintptr_t addr, const void* buf, size_t size) {
    if (!h || !addr || !buf || !size) return false;
    SIZE_T written = 0;
    return WriteProcessMemory(h, reinterpret_cast<LPVOID>(addr), buf, size, &written) && written == size;
}

uintptr_t Mem::ReadPtr(HANDLE h, uintptr_t addr) {
    uintptr_t v = 0;
    Read(h, addr, v);
    return v;
}

static bool MatchPattern(const uint8_t* data, const uint8_t* pat, const char* mask, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (mask[i] == 'x' && data[i] != pat[i]) return false;
    }
    return true;
}

std::optional<uintptr_t> Mem::PatternScan(HANDLE h, uintptr_t start, size_t size, const char* ida) {
    // ida style: "48 8B 05 ?? ?? ?? ?? 48 85 C0"
    std::vector<uint8_t> pat;
    std::string mask;
    for (const char* p = ida; *p; ) {
        while (*p == ' ') ++p;
        if (!*p) break;
        if (p[0] == '?' && (p[1] == '?' || p[1] == ' ' || !p[1])) {
            pat.push_back(0);
            mask.push_back('?');
            p += (p[1] == '?') ? 2 : 1;
        } else {
            char byte[3] = { p[0], p[1], 0 };
            pat.push_back(static_cast<uint8_t>(strtoul(byte, nullptr, 16)));
            mask.push_back('x');
            p += 2;
        }
    }
    if (pat.empty()) return std::nullopt;

    const size_t chunk = 0x10000;
    std::vector<uint8_t> buf(chunk + pat.size());
    for (size_t off = 0; off < size; off += chunk) {
        size_t toRead = chunk + pat.size();
        if (off + toRead > size) toRead = size - off;
        if (toRead < pat.size()) break;
        if (!ReadRaw(h, start + off, buf.data(), toRead)) continue;
        for (size_t i = 0; i + pat.size() <= toRead; i++) {
            if (MatchPattern(buf.data() + i, pat.data(), mask.c_str(), pat.size()))
                return start + off + i;
        }
    }
    return std::nullopt;
}
