# Building Cipher on GitHub Actions

## 1. Put the workflow in your repo
Copy `.github/workflows/build-windows.yml` from this pack into your repo root:

```
your-repo/
  .github/workflows/build-windows.yml
  Cipher.sln          (or your real name)
  ...
```

## 2. Fix the solution name
Edit the workflow line:

```yaml
msbuild Cipher.sln /p:Configuration=Release /p:Platform=x64 /m
```

Use the **exact** `.sln` filename from your repo.

## 3. Enable Actions
GitHub → repo → **Actions** tab → enable workflows if prompted.

## 4. Run
- Push to `main` / `master`, or
- Actions → **Build Cipher** → **Run workflow**

## 5. Download the exe
Open the successful run → **Artifacts** → `cipher-release-x64`.

## Notes
- `windows-latest` has VS Build Tools + MSVC.
- Private deps (ImGui, DXSDK) must be **in the repo** or restored in a step (e.g. submodule).
- Secrets (license HMAC) should stay in source you control; don’t print them in logs.
- Admin exe: add a second project to the `.sln` and it uploads with the same artifact glob.

## Submodules
```yaml
- uses: actions/checkout@v4
  with:
    submodules: recursive
```

## Signing (optional later)
Code signing needs a cert in GitHub Secrets and a `signtool` step — not required for CI smoke builds.
