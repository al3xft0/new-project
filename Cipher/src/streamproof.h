#pragma once
#include <windows.h>

// Windows SDK (Win10 2004+) already defines these as macros.
// Re-declaring them as const DWORD turns into "const DWORD 0x0 = 0x0"
// which is the C2059 you saw. Just use the values (or the macros).

namespace StreamProof {
    inline void Set(HWND hwnd, bool enabled) {
        // 0x11 = WDA_EXCLUDEFROMCAPTURE, 0x0 = WDA_NONE
        SetWindowDisplayAffinity(hwnd, enabled ? 0x11 : 0x0);
    }
}
