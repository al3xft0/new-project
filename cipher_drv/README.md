# cipher_drv — Phase 2 minimal payload

Kernel driver skeleton for Cipher.  
Mapper-loadable. Commands: ping / read / write / alloc / free / get_cr3.

## Layout

```
cipher_drv/
  include/shared.h      ← ABI (pure C, shared with usermode)
  include/physmem.h
  include/comm.h
  include/log.h
  src/entry.c
  src/physmem.c
  src/comm.c
  src/commands.c
  cipher_drv.vcxproj
  .github/workflows/build-driver.yml
```

## Fix applied (build errors)

1. **C2059 `:` syntax** — `enum X : uint32_t` is C++. Headers are now pure C (`#define` constants).
2. **C1083 ntddk.h** — workflow now detects the kit version that actually contains `km\ntddk.h` and passes `WindowsTargetPlatformVersion` to MSBuild. Runner is `windows-2022`.

## Build with GitHub Actions

1. Repo root must contain the `cipher_drv/` folder (your layout is already correct).
2. The workflow file should live at **either**:
   - `cipher_drv/.github/workflows/build-driver.yml` (inside the folder), **or**
   - `.github/workflows/build-driver.yml` at the **repo root** (recommended).

   GitHub only auto-runs workflows under the repo’s top-level `.github/workflows/`.  
   If the workflow is only inside `cipher_drv/.github/`, move/copy it:

   ```
   mkdir -p .github/workflows
   cp cipher_drv/.github/workflows/build-driver.yml .github/workflows/
   ```

3. Actions → **Build cipher_drv** → Run workflow.
4. Download artifact **cipher_drv-sys**.

## Communication

- Section: `\BaseNamedObjects\CipherCommSection` (64 KB)
- Event:   `\BaseNamedObjects\CipherCommEvent`
- Offsets: request @ 0x0, response @ 0x2000, data @ 0x4000

## Next

Phase 3 — map with MSRMapper + usermode ping client.
