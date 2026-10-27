#include <ntifs.h>
#include "../include/shared.h"
#include "../include/physmem.h"
#include "../include/log.h"

void cipher_handle_request(const struct cipher_request* req,
                           struct cipher_response* resp,
                           void* data_buf, SIZE_T data_cap)
{
    resp->magic   = CIPHER_MAGIC;
    resp->version = CIPHER_VERSION;
    resp->status  = ST_OK;
    resp->size    = 0;
    resp->address = 0;
    resp->cr3     = 0;

    switch (req->cmd) {
    case CMD_PING:
        cipher_log("ping");
        break;

    case CMD_READ: {
        NTSTATUS st;
        if (req->size == 0 || req->size > data_cap || req->address == 0) {
            resp->status = ST_INVALID_PARAM;
            break;
        }
        st = physmem_read_virtual(req->cr3, req->address, data_buf, req->size);
        if (!NT_SUCCESS(st)) {
            resp->status = ST_READ_FAIL;
            break;
        }
        resp->size    = req->size;
        resp->address = req->address;
        break;
    }

    case CMD_WRITE: {
        NTSTATUS st;
        if (req->size == 0 || req->size > data_cap || req->address == 0) {
            resp->status = ST_INVALID_PARAM;
            break;
        }
        st = physmem_write_virtual(req->cr3, req->address, data_buf, req->size);
        if (!NT_SUCCESS(st)) {
            resp->status = ST_WRITE_FAIL;
            break;
        }
        resp->size    = req->size;
        resp->address = req->address;
        break;
    }

    case CMD_ALLOC: {
        SIZE_T alloc_size = req->size ? (SIZE_T)req->size : PAGE_SIZE;
        PVOID p = ExAllocatePoolWithTag(NonPagedPool, alloc_size, 'hpiC');
        if (!p) {
            resp->status = ST_ALLOC_FAIL;
            break;
        }
        RtlZeroMemory(p, alloc_size);
        resp->address = (UINT64)(ULONG_PTR)p;
        resp->size    = (UINT32)alloc_size;
        cipher_log("alloc %lu bytes @ %p", (unsigned long)alloc_size, p);
        break;
    }

    case CMD_FREE: {
        if (req->address == 0) {
            resp->status = ST_INVALID_PARAM;
            break;
        }
        ExFreePoolWithTag((PVOID)(ULONG_PTR)req->address, 'hpiC');
        resp->address = req->address;
        break;
    }

    case CMD_GET_CR3:
        resp->cr3 = __readcr3();
        break;

    default:
        resp->status = ST_INVALID_CMD;
        break;
    }
}
