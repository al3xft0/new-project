#pragma once
#include <Windows.h>

namespace StreamProof {
    // WDA_EXCLUDEFROMCAPTURE = 0x11 (Win10 2004+)
    inline bool Set(HWND hwnd, bool enabled) {
        if (!hwnd) return false;
        const DWORD WDA_NONE = 0x0;
        const DWORD WDA_EXCLUDEFROMCAPTURE = 0x11;
        return SetWindowDisplayAffinity(hwnd, enabled ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE) != FALSE;
    }
}
