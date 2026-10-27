#include "../include/shared.h"
#include "../include/physmem.h"
#include "../include/log.h"
#include <ntddk.h>

void cipher_handle_request(const struct cipher_request* req,
                           struct cipher_response* resp,
                           void* data_buf, size_t data_cap)
{
    resp->magic   = CIPHER_MAGIC;
    resp->version = CIPHER_VERSION;
    resp->status  = ST_OK;
    resp->size    = 0;
    resp->address = 0;
    resp->cr3     = 0;

    switch (req->cmd) {
    case CMD_PING:
        resp->size = 0;
        cipher_log("ping");
        break;

    case CMD_READ: {
        if (req->size == 0 || req->size > data_cap || req->address == 0) {
            resp->status = ST_INVALID_PARAM;
            break;
        }
        NTSTATUS st = physmem_read_virtual(req->cr3, req->address, data_buf, req->size);
        if (!NT_SUCCESS(st)) {
            resp->status = ST_READ_FAIL;
            break;
        }
        resp->size    = req->size;
        resp->address = req->address;
        break;
    }

    case CMD_WRITE: {
        if (req->size == 0 || req->size > data_cap || req->address == 0) {
            resp->status = ST_INVALID_PARAM;
            break;
        }
        // data to write is already in data_buf (usermode wrote it after the request header)
        NTSTATUS st = physmem_write_virtual(req->cr3, req->address, data_buf, req->size);
        if (!NT_SUCCESS(st)) {
            resp->status = ST_WRITE_FAIL;
            break;
        }
        resp->size    = req->size;
        resp->address = req->address;
        break;
    }

    case CMD_ALLOC: {
        size_t alloc_size = req->size ? req->size : PAGE_SIZE;
        PVOID p = ExAllocatePoolWithTag(NonPagedPool, alloc_size, 'hpiC');
        if (!p) {
            resp->status = ST_ALLOC_FAIL;
            break;
        }
        RtlZeroMemory(p, alloc_size);
        resp->address = (uint64_t)p;
        resp->size    = (uint32_t)alloc_size;
        cipher_log("alloc %u bytes @ %p", (unsigned)alloc_size, p);
        break;
    }

    case CMD_FREE: {
        if (req->address == 0) {
            resp->status = ST_INVALID_PARAM;
            break;
        }
        ExFreePoolWithTag((PVOID)req->address, 'hpiC');
        resp->address = req->address;
        break;
    }

    case CMD_GET_CR3: {
        // Phase 2: return current process CR3 (system context when called from our thread)
        resp->cr3 = __readcr3();
        break;
    }

    default:
        resp->status = ST_INVALID_CMD;
        break;
    }
}
