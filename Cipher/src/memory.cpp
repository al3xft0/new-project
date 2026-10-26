#include "memory.h"
#include <cstring>
#include <cstdio>

static char g_lastErr[192] = {};
const char* Mem::LastError() { return g_lastErr; }
static void SetErr(const char* s) {
    snprintf(g_lastErr, sizeof(g_lastErr), "%s", s ? s : "");
}
static void SetErrCode(const char* stage, DWORD err) {
    snprintf(g_lastErr, sizeof(g_lastErr), "%s (win32 %lu)", stage, (unsigned long)err);
}

static bool EnableDebugPrivilege() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        return false;
    LUID luid{};
    if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) {
        CloseHandle(token);
        return false;
    }
    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    BOOL ok = AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    CloseHandle(token);
    return ok && GetLastError() != ERROR_NOT_ALL_ASSIGNED;
}

std::optional<GameProcess> Mem::Attach(const wchar_t* exeName) {
    SetErr("");
    EnableDebugPrivilege();

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        SetErrCode("process snapshot failed", GetLastError());
        return std::nullopt;
    }

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

    if (!pid) {
        // helpful: scan for anything Fortnite* so UI can hint
        HANDLE snap2 = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap2 != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe2{ sizeof(pe2) };
            if (Process32FirstW(snap2, &pe2)) {
                do {
                    if (wcsstr(pe2.szExeFile, L"Fortnite") || wcsstr(pe2.szExeFile, L"fortnite")) {
                        char narrow[128]{};
                        WideCharToMultiByte(CP_UTF8, 0, pe2.szExeFile, -1, narrow, 128, nullptr, nullptr);
                        snprintf(g_lastErr, sizeof(g_lastErr),
                                 "Shipping.exe not found — saw %s (pid %lu)", narrow, (unsigned long)pe2.th32ProcessID);
                        CloseHandle(snap2);
                        return std::nullopt;
                    }
                } while (Process32NextW(snap2, &pe2));
            }
            CloseHandle(snap2);
        }
        SetErr("FortniteClient-Win64-Shipping.exe not running");
        return std::nullopt;
    }

    GameProcess gp;
    gp.pid = pid;
    gp.name = exeName;

    // Not PROCESS_ALL_ACCESS — only what we need
    const DWORD kFull =
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION;
    const DWORD kRead =
        PROCESS_VM_READ | PROCESS_QUERY_INFORMATION;

    SetLastError(0);
    gp.handle = OpenProcess(kFull, FALSE, pid);
    if (!gp.handle) {
        DWORD e1 = GetLastError();
        EnableDebugPrivilege();
        SetLastError(0);
        gp.handle = OpenProcess(kFull, FALSE, pid);
        if (!gp.handle) {
            DWORD e2 = GetLastError();
            gp.handle = OpenProcess(kRead, FALSE, pid);
            if (!gp.handle) {
                SetErrCode("OpenProcess denied — run as Admin / AC blocked", e2 ? e2 : e1);
                return std::nullopt;
            }
            // read-only handle is still usable for ESP
        }
    }

    // Module base
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
        HMODULE mods[1024];
        DWORD needed = 0;
        if (EnumProcessModules(gp.handle, mods, sizeof(mods), &needed) && needed >= sizeof(HMODULE)) {
            gp.base = reinterpret_cast<uintptr_t>(mods[0]);
        }
    }

    if (!gp.base) {
        DWORD e = GetLastError();
        CloseHandle(gp.handle);
        gp.handle = nullptr;
        SetErrCode("module base failed (AC may strip module list)", e);
        return std::nullopt;
    }

    SetErr("");
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
