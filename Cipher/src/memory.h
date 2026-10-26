#pragma once
#include "win_pre.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct GameProcess {
    DWORD   pid = 0;
    HANDLE  handle = nullptr;
    uintptr_t base = 0;
    std::wstring name;
};

namespace Mem {
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

namespace Decrypt {
    inline uint64_t Bswap64(uint64_t x) {
        const uint32_t lo = (uint32_t)x;
        const uint32_t hi = (uint32_t)(x >> 32);
        const uint32_t slo = (lo << 24) | ((lo >> 8) & 0x00FF0000u) | ((lo << 8) & 0x0000FF00u) | (lo >> 24);
        const uint32_t shi = (hi << 24) | ((hi >> 8) & 0x00FF0000u) | ((hi << 8) & 0x0000FF00u) | (hi >> 24);
        return ((uint64_t)slo << 32) | shi;
    }

    inline uint64_t GWorld(uint64_t base, HANDLE h) {
        uint64_t enc = Mem::Read<uint64_t>(h, base + 0x1B096DE8);
        uint64_t w = Bswap64(enc ^ 0xAFB5C90D00000000ull);
        return (w >= 0x10000 && w < 0x00007FFFFFFFFFFFfull) ? w : 0;
    }
}
