# cipher_drv minimal

DriverEntry only. No section, no event, no worker thread.

## Build (GitHub Actions)

Same as before: push `cipher_drv/`, run workflow, download `cipher_drv-sys` artifact.

## Map (Admin)

```bat
cd /d C:\Users\adamt\Downloads\kdmapper\x64\Release
.\kdmapper_Release.exe C:\Users\adamt\Downloads\cipher_drv.sys
```

Expect: DriverEntry 0x0, success, **no BSOD**.

Do **not** run cipher_ping against this build — there is no channel yet.
