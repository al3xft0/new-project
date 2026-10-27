#pragma once
// Shared between kernel payload and usermode client
// Keep this header identical on both sides.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Magic so usermode can confirm it is talking to our driver
#define CIPHER_MAGIC          0x43495048u  /* 'CIPH' */
#define CIPHER_VERSION        1

// Commands
enum cipher_cmd : uint32_t {
    CMD_PING          = 1,
    CMD_READ          = 2,
    CMD_WRITE         = 3,
    CMD_ALLOC         = 4,
    CMD_FREE          = 5,
    CMD_GET_CR3       = 6,
    CMD_GET_PROCESS   = 7,
};

// Status codes returned in cipher_response::status
enum cipher_status : int32_t {
    ST_OK             = 0,
    ST_INVALID_CMD    = -1,
    ST_INVALID_PARAM  = -2,
    ST_NO_PROCESS     = -3,
    ST_READ_FAIL      = -4,
    ST_WRITE_FAIL     = -5,
    ST_ALLOC_FAIL     = -6,
    ST_NOT_READY      = -7,
};

#pragma pack(push, 1)
struct cipher_request {
    uint32_t magic;       // CIPHER_MAGIC
    uint32_t version;     // CIPHER_VERSION
    uint32_t cmd;         // cipher_cmd
    uint32_t size;        // size of data following / bytes to r/w
    uint64_t address;     // target VA
    uint64_t cr3;         // target process CR3 (0 = current / system)
    uint64_t pid;         // optional process id for GET_CR3 / GET_PROCESS
    uint64_t reserved;
    // for WRITE the payload bytes follow this header in the same buffer
};

struct cipher_response {
    uint32_t magic;
    uint32_t version;
    int32_t  status;      // cipher_status
    uint32_t size;        // bytes returned / allocated
    uint64_t address;     // for ALLOC: kernel VA; for READ: echo
    uint64_t cr3;         // for GET_CR3
    uint64_t reserved;
    // for READ the data follows this header
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif
