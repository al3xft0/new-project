# cipher_drv — Phase 2 minimal payload

Kernel driver skeleton for Cipher.  
Mapper-loadable. Four commands: ping / read / write / alloc (+ free, get_cr3).

## Layout

```
cipher_drv/
  include/
    shared.h      ← ABI shared with usermode (keep identical)
    physmem.h
    comm.h
    log.h
  src/
    entry.c       ← DriverEntry
    physmem.c     ← VA↔PA + physical R/W (Phase 2 simple path)
    comm.c        ← section + event channel
    commands.c    ← command handlers
  cipher_drv.vcxproj
  .github/workflows/build-driver.yml
```

## Communication (Phase 2)

- Section: `\BaseNamedObjects\CipherCommSection` (64 KB)
- Event:   `\BaseNamedObjects\CipherCommEvent`
- Layout inside section:
  - `0x0000`  `cipher_request`
  - `0x2000`  `cipher_response`
  - `0x4000`  data buffer (read results / write payload)

Usermode flow:
1. Open section + event
2. Write `cipher_request` (and data for WRITE)
3. Signal event
4. Wait / poll for response magic + status
5. Read `cipher_response` (+ data for READ)

## Build with GitHub Actions

1. Put this folder at the root of your repo as `cipher_drv/` (or adjust the workflow paths).
2. Push to GitHub.
3. Actions tab → “Build cipher_drv” → run workflow (or it runs on push).
4. When green, download the artifact `cipher_drv-sys` — it contains `cipher_drv.sys`.

### First-time WDK on the runner

`windows-latest` has Visual Studio and the Windows SDK. The workflow installs the WDK via chocolatey.  
If the WDK step fails on a future runner image, switch to the **EWDK** approach:

- Download the Enterprise WDK ISO from Microsoft in a previous step
- Mount it and call `LaunchBuildEnv.cmd` then `msbuild`

Most of the time the chocolatey WDK package is enough for a simple WDM driver.

## Local build (optional)

If you have VS 2022 + WDK 10 installed:

```
msbuild cipher_drv.vcxproj /p:Configuration=Release /p:Platform=x64
```

Output lands under `x64\Release\cipher_drv.sys` (or the path MSBuild reports).

## Next (Phase 3)

- Map `cipher_drv.sys` with MSRMapper
- Small usermode test client that opens the section/event and sends CMD_PING
- Confirm round-trip before adding Fortnite logic

## Notes

- Phase 2 physmem uses `MmGetPhysicalAddress` / `MmMapIoSpace` — enough to prove the channel and basic R/W in the same context. Full CR3 page-table walk lands in Phase 4.
- Communication is intentionally simple (section + event) so we can replace it later with the Fuse-style win32k hook without changing `shared.h`.
