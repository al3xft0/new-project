#pragma once
#include <stdint.h>

#define CIPHER_MAGIC          0x43495048u
#define CIPHER_VERSION        1
#define CIPHER_READY_MAGIC    0x52454459u
#define CMD_PING              1u
#define ST_OK                 0
#define SECTION_SIZE          (64 * 1024)
#define SEC_OFF_HEADER        0x0000
#define SEC_OFF_REQUEST       0x0100
#define SEC_OFF_RESPONSE      0x0200

#pragma pack(push, 1)
struct cipher_shared_header {
    uint32_t ready_magic;
    uint32_t version;
    volatile uint32_t submit;
    volatile uint32_t seq;
    uint64_t reserved[4];
};
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
