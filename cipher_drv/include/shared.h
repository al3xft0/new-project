#pragma once
/* Shared between kernel payload and usermode client.
   Pure C — keep identical on both sides. */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CIPHER_MAGIC          0x43495048u  /* 'CIPH' */
#define CIPHER_VERSION        1

/* Commands */
#define CMD_PING              1u
#define CMD_READ              2u
#define CMD_WRITE             3u
#define CMD_ALLOC             4u
#define CMD_FREE              5u
#define CMD_GET_CR3           6u
#define CMD_GET_PROCESS       7u

/* Status codes returned in cipher_response::status */
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
    uint32_t magic;       /* CIPHER_MAGIC */
    uint32_t version;     /* CIPHER_VERSION */
    uint32_t cmd;         /* CMD_* */
    uint32_t size;        /* bytes to r/w or alloc */
    uint64_t address;     /* target VA */
    uint64_t cr3;         /* target process CR3 (0 = current) */
    uint64_t pid;         /* optional process id */
    uint64_t reserved;
    /* for WRITE the payload bytes follow this header in the same buffer */
};

struct cipher_response {
    uint32_t magic;
    uint32_t version;
    int32_t  status;      /* ST_* */
    uint32_t size;        /* bytes returned / allocated */
    uint64_t address;     /* for ALLOC: kernel VA; for READ: echo */
    uint64_t cr3;         /* for GET_CR3 */
    uint64_t reserved;
    /* for READ the data follows this header */
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif
