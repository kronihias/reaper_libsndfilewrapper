# GitHub Actions Code Signing Setup

This document explains how to set up code signing for the automated macOS and
Windows installer builds (`.github/workflows/release.yml`). Code signing is
**optional** — if the secrets below are not configured, the workflow falls back
to unsigned installers. The Linux job produces an unsigned `.tar.gz` and needs
no secrets.

## GitHub Secrets Setup

### Repository Secrets vs Environment Secrets

For code signing certificates, use **Repository secrets** (not Environment secrets) because:

- **Repository secrets**: Available to all workflows in the repo, suitable for release builds
- **Environment secrets**: Scoped to specific environments (production/staging), require manual approval

Since the workflow triggers on release publication (a production event), **Repository secrets** are appropriate and simpler to manage.

### Adding Repository Secrets

Go to: **Settings → Secrets and variables → Actions → Repository secrets**

Add each secret listed below.

### macOS Certificate Secrets
- `CODESIGN_CERTIFICATE_P12`: Your Developer ID certificate exported as base64-encoded PKCS#12 (.p12) file
- `CODESIGN_CERTIFICATE_PASSWORD`: Password for the PKCS#12 certificate file
- `KEYCHAIN_PASSWORD`: Password for the temporary keychain (can be any secure password)

### macOS Certificate identity secrets
- `CODESIGN_APP`: Your **Developer ID Application** certificate name (signs the `.dylib`)
- `CODESIGN_INSTALLER`: Your **Developer ID Installer** certificate name (signs the `.pkg`)

> The imported `.p12` must contain both identities (Developer ID Application and
> Developer ID Installer), or at least the private keys for both certificates.

### macOS Notarization Secrets
- `NOTARIZE_APPLE_ID`: Your Apple ID email address
- `NOTARIZE_PASSWORD`: App-specific password for notarytool (generate at https://appleid.apple.com)
- `NOTARIZE_TEAM_ID`: Your Apple Developer Team ID

### Windows Certificate Secrets
- `WINDOWS_CODESIGN_SUBJECT`: Certificate subject name (e.g., "Matthias Kronlachner")
- `WINDOWS_CODESIGN_PFX`: Base64-encoded `.pfx` certificate file (optional; leave empty to use the certificate store)
- `WINDOWS_CODESIGN_PFX_PASSWORD`: Password for the `.pfx` file (if using PFX)

## How to Export Your macOS Certificate

```bash
# Find your certificate identity
security find-identity -v -p codesigning

# Export from login keychain (Keychain Access → My Certificates → Export → .p12 also works)
security export -k ~/Library/Keychains/login.keychain-db -t identities -f pkcs12 -o certificate.p12

# Base64 encode for the GitHub secret
base64 -i certificate.p12 | pbcopy
```

Developer ID certificates must include the private key to be usable for signing.
If export is unavailable, re-download the certificate from the
[Apple Developer Certificates page](https://developer.apple.com/account/resources/certificates/list)
and double-click the `.cer` to reinstall it with its private key.

## How to Export Your Windows Certificate

1. Open Certificate Manager (`certmgr.msc`)
2. Personal → Certificates → your code signing certificate
3. Right-click → All Tasks → Export → "Yes, export the private key" → PKCS #12 (.pfx)
4. `base64 -i certificate.pfx` and copy the output into `WINDOWS_CODESIGN_PFX`

## Workflow Behavior

- **macOS**: certificate secrets configured → signed + notarized `.pkg`; otherwise → unsigned `.pkg`
- **Windows**: certificate secrets configured → signed `.exe`; otherwise → unsigned `.exe`
- **Linux**: always an unsigned `.tar.gz` (no signing infrastructure for REAPER plugins on Linux)

## How to Publish a Release

1. **Update the `VERSION` file** (e.g. `1.0.0`)
2. **Commit and push** the VERSION change
3. **Create a GitHub release** with tag `v1.0.0` (must match VERSION, with a `v` prefix) and **Publish** it
4. GitHub Actions builds the Windows `.exe`, macOS `.pkg` and Linux `.tar.gz` and uploads them as release assets

You can also trigger the workflow manually via **Actions → Release → Run workflow**
(`workflow_dispatch`); that uploads the installers as build artifacts instead of
attaching them to a release.

### Example certificate identities
- `CODESIGN_APP`: `Developer ID Application: Matthias Kronlachner (TEAMID)`
- `CODESIGN_INSTALLER`: `Developer ID Installer: Matthias Kronlachner (TEAMID)`

## Security Notes

- Never commit certificate files or passwords to the repository (`scripts/codesign.env` is gitignored)
- Rotate app-specific passwords regularly
- The temporary keychain is deleted after each macOS build
