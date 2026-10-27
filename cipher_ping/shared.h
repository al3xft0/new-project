#pragma once
/* Usermode copy of cipher_drv ABI */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CIPHER_MAGIC          0x43495048u
#define CIPHER_VERSION        1

#define CMD_PING              1u
#define CMD_READ              2u
#define CMD_WRITE             3u
#define CMD_ALLOC             4u
#define CMD_FREE              5u
#define CMD_GET_CR3           6u

#define ST_OK                 0
#define ST_INVALID_CMD       -1
#define ST_INVALID_PARAM     -2
#define ST_NO_PROCESS        -3
#define ST_READ_FAIL         -4
#define ST_WRITE_FAIL        -5
#define ST_ALLOC_FAIL        -6
#define ST_NOT_READY         -7

#pragma pack(push, 1)
struct cipher_request {
    uint32_t magic;
    uint32_t version;
    uint32_t cmd;
    uint32_t size;
    uint64_t address;
    uint64_t cr3;
    uint64_t pid;
    uint64_t reserved;
};

struct cipher_response {
    uint32_t magic;
    uint32_t version;
    int32_t  status;
    uint32_t size;
    uint64_t address;
    uint64_t cr3;
    uint64_t reserved;
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif
