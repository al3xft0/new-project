#pragma once
#include "win_pre.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

struct GameProcess {
    DWORD   pid = 0;
    HANDLE  handle = nullptr;
    uintptr_t base = 0;
    std::wstring name;
};

namespace Mem {
    // last failure reason for UI (empty on success)
    const char* LastError();
    std::optional<GameProcess> Attach(const wchar_t* exeName = L"FortniteClient-Win64-Shipping.exe");
    void Close(GameProcess& p);
    bool IsValid(const GameProcess& p);

    bool ReadRaw(HANDLE h, uintptr_t addr, void* buf, size_t size);
    bool WriteRaw(HANDLE h, uintptr_t addr, const void* buf, size_t size);

    template<typename T>
    bool Read(HANDLE h, uintptr_t addr, T& out) {
        return ReadRaw(h, addr, &out, sizeof(T));
    }

    template<typename T>
    T Read(HANDLE h, uintptr_t addr) {
        T v{};
        ReadRaw(h, addr, &v, sizeof(T));
        return v;
    }

    uintptr_t ReadPtr(HANDLE h, uintptr_t addr);
    std::optional<uintptr_t> PatternScan(HANDLE h, uintptr_t start, size_t size, const char* pattern);
}
