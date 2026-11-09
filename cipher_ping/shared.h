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

#define SECTION_SIZE          (64 * 1024)
#define SEC_OFF_HEADER        0x0000

#pragma pack(push, 1)
struct cipher_shared_header {
    uint32_t ready_magic;
    uint32_t version;
    uint32_t submit;
    uint32_t seq;
    uint64_t reserved[4];
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif
