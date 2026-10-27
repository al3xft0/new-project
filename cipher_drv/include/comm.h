#pragma once
#include <ntddk.h>
#include "shared.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize stealth communication channel.
// For Phase 2 we use a simple shared section + event pair that usermode opens.
// Later we can swap this for the Fuse-style NtUserGetCPD hook without changing the command ABI.
NTSTATUS comm_init(void);
void     comm_shutdown(void);

// Called from the worker thread when a request arrives
typedef void (*comm_handler_fn)(const struct cipher_request* req, struct cipher_response* resp, void* data_buf, size_t data_cap);

void comm_set_handler(comm_handler_fn handler);

#ifdef __cplusplus
}
#endif
