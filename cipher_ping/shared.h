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
#define CIPHER_READY_MAGIC    0x52454459u  /* 'REDY' — set by driver when channel is live */

#define CMD_NONE              0u
#define CMD_PING              1u
#define CMD_READ              2u
#define CMD_WRITE             3u

#define ST_OK                 0
#define ST_INVALID_CMD       -1
#define ST_INVALID_PARAM     -2
#define ST_READ_FAIL         -4
#define ST_WRITE_FAIL        -5
#define ST_BUSY              -8

/* Shared section layout (64KB)
 * 0x0000  cipher_shared_header (ready + handshake)
 * 0x0100  cipher_request
 * 0x0200  cipher_response
 * 0x1000  data buffer
 */
#define SEC_OFF_HEADER        0x0000
#define SEC_OFF_REQUEST       0x0100
#define SEC_OFF_RESPONSE      0x0200
#define SEC_OFF_DATA          0x1000
#define SEC_DATA_SIZE         (64 * 1024 - 0x1000)
#define SECTION_SIZE          (64 * 1024)

#pragma pack(push, 1)
struct cipher_shared_header {
    uint32_t ready_magic;   /* CIPHER_READY_MAGIC when driver is up */
    uint32_t version;
    volatile uint32_t submit;   /* usermode sets 1 to submit; driver sets 0 when done */
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
