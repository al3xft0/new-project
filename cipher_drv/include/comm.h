#pragma once
#include <ntifs.h>
#include "shared.h"

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS comm_init(void);
void     comm_shutdown(void);

typedef void (*comm_handler_fn)(const struct cipher_request* req,
                                struct cipher_response* resp,
                                void* data_buf, SIZE_T data_cap);

void comm_set_handler(comm_handler_fn handler);

#ifdef __cplusplus
}
#endif
