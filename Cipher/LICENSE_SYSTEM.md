# Cipher license system

## Two executables
| EXE | Role |
|---|---|
| **Cipher** (loader) | Intro → license field → attach UI / cheat |
| **CipherAdmin** | Password gate → generate / revoke / list keys |

## Key format
`CIPHER-XXXXX-XXXXX-...` base32 blob = payload (id, created, days) + HMAC-SHA256 truncated.

## Status UI
| State | Color | Label |
|---|---|---|
| Valid | green | Valid |
| Revoked | red | Revoked |
| Expired | orange | Expired |
| Bad / invalid | gray | Invalid / Bad format |

Also shows **Active N days · expires YYYY-MM-DD**.

## Admin
1. Set `kAdminPasswordDefault` and `kLicenseSecret` in `license_crypto.h` (same secret in client).
2. Run admin exe → unlock with password.
3. Generate key (days + optional note) → stored in `keys.db` next to admin exe.
4. Revoke marks status in `keys.db`. Client should load revoked ids from the same db or a published revoke list.

## Loader UI flow
1. Full-window black chrome (no gray title strip).
2. Intro glow logo (existing).
3. Logo **eases** to top half + shrinks (not teleport).
4. Centered row: `[ Enter license key ............ ] [ → ]`
5. On submit: show Valid/Revoked/… under the field.
6. Only if Valid → show attach panel.

## Wire into Cipher
- Add `licenseOk`, `licenseBuf`, `logoAnim`, … to `AppState` (see `license_gate_ui.cpp` comments).
- After `introDone`, if `!licenseOk` call `UI_RenderLicenseGate` and return.
- Paint title bar region with same `IM_COL32(10,10,12)` as body.
