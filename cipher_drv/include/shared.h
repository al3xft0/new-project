#pragma once
#if defined(CIPHER_KERNEL_BUILD)
  typedef unsigned int       uint32_t;
  typedef int                int32_t;
  typedef unsigned long long uint64_t;
#else
  #include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CIPHER_MAGIC          0x43495048u
#define CIPHER_VERSION        1
#define CIPHER_READY_MAGIC    0x52454459u

#define CMD_NONE              0u
#define CMD_PING              1u

#define ST_OK                 0
#define ST_INVALID_CMD       -1
#define ST_INVALID_PARAM     -2

#define SECTION_SIZE          (64 * 1024)
#define SEC_OFF_HEADER        0x0000
#define SEC_OFF_REQUEST       0x0100
#define SEC_OFF_RESPONSE      0x0200
#define SEC_OFF_DATA          0x1000

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

#ifdef __cplusplus
}
#endif
