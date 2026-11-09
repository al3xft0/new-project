# cipher commands v1

Section (stable) + deferred worker (2s) + CMD_PING.

## Run
1. GA build cipher_drv.sys
2. cl /EHsc /O2 cipher_ping/main.cpp /Fe:cipher_ping.exe
3. kdmapper cipher_drv.sys  → wait for success
4. Wait ~3 seconds
5. cipher_ping.exe → *** PING OK ***
