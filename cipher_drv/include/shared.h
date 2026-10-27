#pragma once
/* Shared ABI — pure C.
   When building the driver (_KERNEL_MODE), use local typedefs.
   Usermode clients include this without _KERNEL_MODE and get <stdint.h>. */

#if defined(_KERNEL_MODE) || defined(_NTDDK_) || defined(_NTHAL_) || defined(_NTIFS_)
  #ifndef _CIPHER_UINT_DEFINED
  #define _CIPHER_UINT_DEFINED
  typedef unsigned int       uint32_t;
  typedef int                int32_t;
  typedef unsigned long long uint64_t;
  #endif
#else
  #include <stdint.h>
#endif

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
#define CMD_GET_PROCESS       7u

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
