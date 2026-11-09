# cipher channel v2

No event object. Shared section + poll worker + submit flag.

## Build driver
GitHub Actions on `cipher_drv/` → artifact `cipher_drv.sys`

## Build ping
```bat
cd cipher_ping
cl /EHsc /O2 main.cpp /Fe:cipher_ping.exe
```

## Run (Admin)
```bat
cd /d C:\Users\adamt\Downloads\kdmapper\x64\Release
.\kdmapper_Release.exe C:\Users\adamt\Downloads\cipher_drv.sys
cipher_ping.exe
```

Want: DriverEntry 0x0, then *** PING OK ***
