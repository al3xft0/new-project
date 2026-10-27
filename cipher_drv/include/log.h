#pragma once
#include <ntifs.h>

#define CIPHER_LOG_PREFIX "[cipher] "

#if DBG
#define cipher_log(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, CIPHER_LOG_PREFIX fmt "\n", ##__VA_ARGS__)
#else
#define cipher_log(fmt, ...) ((void)0)
#endif

#define cipher_err(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, CIPHER_LOG_PREFIX fmt "\n", ##__VA_ARGS__)
