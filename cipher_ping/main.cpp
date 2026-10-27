// cipher_ping — Phase 3 usermode client
// Opens CipherComm section + event, sends CMD_PING, prints result.
// Build: cl /EHsc /O2 main.cpp /Fe:cipher_ping.exe
//    or: g++ -O2 -o cipher_ping.exe main.cpp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "shared.h"

static const wchar_t* kSectionName = L"Global\\CipherCommSection";
static const wchar_t* kEventName   = L"Global\\CipherCommEvent";

// Kernel created \\BaseNamedObjects\\... which usermode sees as Global\\...
// Fallback names without Global\\ if needed.
static const wchar_t* kSectionNameAlt = L"CipherCommSection";
static const wchar_t* kEventNameAlt   = L"CipherCommEvent";

#define SECTION_SIZE  (64 * 1024)
#define OFF_REQUEST   0x0000
#define OFF_RESPONSE  0x2000
#define OFF_DATA      0x4000

static HANDLE open_section(void)
{
    HANDLE h = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, kSectionName);
    if (!h)
        h = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, kSectionNameAlt);
    return h;
}

static HANDLE open_event(void)
{
    HANDLE h = OpenEventW(EVENT_ALL_ACCESS, FALSE, kEventName);
    if (!h)
        h = OpenEventW(EVENT_ALL_ACCESS, FALSE, kEventNameAlt);
    return h;
}

static const char* status_str(int32_t s)
{
    switch (s) {
    case ST_OK:            return "ST_OK";
    case ST_INVALID_CMD:   return "ST_INVALID_CMD";
    case ST_INVALID_PARAM: return "ST_INVALID_PARAM";
    case ST_NO_PROCESS:    return "ST_NO_PROCESS";
    case ST_READ_FAIL:     return "ST_READ_FAIL";
    case ST_WRITE_FAIL:    return "ST_WRITE_FAIL";
    case ST_ALLOC_FAIL:    return "ST_ALLOC_FAIL";
    case ST_NOT_READY:     return "ST_NOT_READY";
    default:               return "UNKNOWN";
    }
}

int main(void)
{
    printf("cipher_ping — phase 3\n");

    HANDLE hSection = open_section();
    if (!hSection) {
        printf("[!] OpenFileMapping failed (%lu)\n", GetLastError());
        printf("    Is cipher_drv.sys mapped and DriverEntry finished?\n");
        return 1;
    }
    printf("[+] section open\n");

    HANDLE hEvent = open_event();
    if (!hEvent) {
        printf("[!] OpenEvent failed (%lu)\n", GetLastError());
        CloseHandle(hSection);
        return 1;
    }
    printf("[+] event open\n");

    void* view = MapViewOfFile(hSection, FILE_MAP_ALL_ACCESS, 0, 0, SECTION_SIZE);
    if (!view) {
        printf("[!] MapViewOfFile failed (%lu)\n", GetLastError());
        CloseHandle(hEvent);
        CloseHandle(hSection);
        return 1;
    }
    printf("[+] view mapped @ %p\n", view);

    cipher_request*  req  = (cipher_request*)((char*)view + OFF_REQUEST);
    cipher_response* resp = (cipher_response*)((char*)view + OFF_RESPONSE);

    // clear response so we can see a real reply
    memset(resp, 0, sizeof(*resp));

    memset(req, 0, sizeof(*req));
    req->magic   = CIPHER_MAGIC;
    req->version = CIPHER_VERSION;
    req->cmd     = CMD_PING;
    req->size    = 0;

    printf("[*] sending CMD_PING ...\n");
    if (!SetEvent(hEvent)) {
        printf("[!] SetEvent failed (%lu)\n", GetLastError());
        UnmapViewOfFile(view);
        CloseHandle(hEvent);
        CloseHandle(hSection);
        return 1;
    }

    // poll for response magic + version (driver re-signals event when done)
    int ok = 0;
    for (int i = 0; i < 50; ++i) {
        Sleep(20);
        if (resp->magic == CIPHER_MAGIC && resp->version == CIPHER_VERSION) {
            ok = 1;
            break;
        }
    }

    if (!ok) {
        printf("[!] timeout — no valid response (magic=0x%08X ver=%u)\n",
               resp->magic, resp->version);
        printf("    Driver may not be running or channel layout mismatch.\n");
        UnmapViewOfFile(view);
        CloseHandle(hEvent);
        CloseHandle(hSection);
        return 1;
    }

    printf("[+] response:\n");
    printf("    status  = %d (%s)\n", resp->status, status_str(resp->status));
    printf("    size    = %u\n", resp->size);
    printf("    address = 0x%llX\n", (unsigned long long)resp->address);
    printf("    cr3     = 0x%llX\n", (unsigned long long)resp->cr3);

    if (resp->status == ST_OK)
        printf("\n*** PING OK — channel live ***\n");
    else
        printf("\n*** PING returned non-OK status ***\n");

    UnmapViewOfFile(view);
    CloseHandle(hEvent);
    CloseHandle(hSection);
    return resp->status == ST_OK ? 0 : 2;
}
