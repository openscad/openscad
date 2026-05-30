# Windows Code Signing with Certum SimplySign

<!-- markdownlint-disable MD013 -->

This guide explains how to sign PythonSCAD Windows release artifacts (NSIS
installer `.exe` and MSIX package) using a **Certum Open Source Code Signing**
certificate on the SimplySign cloud HSM. The private key never leaves Certum's
infrastructure.

## Release flow overview

```text
Release Please PR merged
        │
        ▼
GitHub release created as pre-release
        │
        ▼
All platform build workflows run in parallel
(AppImage, Debian, macOS, RPM, Windows)
        │
        ▼
notify-signing.yml: waits for all builds → posts signing instructions
to the release body → GitHub emails the maintainer → workflow exits
        │
        [release sits as pre-release; no timeout]
        │
        ▼  (whenever the maintainer is ready)
Maintainer runs: .\scripts\sign-windows-release.ps1
  → downloads NSIS installer + MSIX (if present)
  → signs with signtool via SimplySign Desktop
  → re-uploads signed files
  → triggers publish-release.yml
        │
        ▼
publish-release.yml: downloads all artifacts → computes SHA256/SHA512
checksums → uploads sidecar + checksums.txt → removes pre-release flag
        │
        ▼
Release published; downstream workflows fire (PyPI, etc.)
```

Key properties of this design:

- **No timeout.** The release sits as a pre-release until you sign. Sign it a
  day later, a week later — no workflow is waiting.
- **Checksums are computed after signing.** The publish workflow runs on the
  already-signed artifacts, so all checksums are correct by construction. The
  signing script does not need to touch checksums at all.
- **MSIX is optional.** Releases before 1.0.0 do not include an MSIX; the
  script silently skips it when not present.

---

## Part 1: One-time setup

### 1.1 Purchase the certificate

Buy the **Open Source Code Signing on SimplySign** product from
[certum.store](https://certum.store/open-source-code-signing-on-simplysign.html).

During onboarding Certum will verify your identity and project, issue the
certificate tied to your SimplySign account, and guide you through installing
the **SimplySign** app on your Android or iOS device.

### 1.2 Install desktop software

Install both of these on your Windows signing machine:

- **SimplySign Desktop** — emulates a virtual smart card so `signtool.exe` can
  see your cloud certificate in the Windows certificate store.
- **proCertum CardManager** — optional; useful for certificate management and
  initialising PIN codes if ever needed.

Both are available from
[support.certum.eu](https://support.certum.eu/en/cert-offer-software-and-libraries/).

### 1.3 Note your certificate thumbprint and Subject DN

After connecting SimplySign Desktop for the first time (see Part 2), run this
in PowerShell to find your certificate's SHA1 thumbprint and exact Subject DN:

```powershell
Get-ChildItem Cert:\CurrentUser\My |
    Where-Object { $_.EnhancedKeyUsageList.ObjectId -contains '1.3.6.1.5.5.7.3.3' } |
    Select-Object Thumbprint, Subject | Format-List
```

- **Thumbprint** — used by `signtool` to select the right certificate. Update
  the `-Thumbprint` default in `scripts/sign-windows-release.ps1` if it
  changes (e.g. after certificate renewal).
- **Subject DN** — the full string (e.g.
  `CN=Open Source Developer Jane Smith, O=Open Source Developer, L=Vienna, S=Wien, C=AT`)
  is needed as the MSIX `publisher-cn` if you ever want to produce a signed
  MSIX with a matching publisher identity.

### 1.4 Install prerequisites

- **Windows SDK** (comes with Visual Studio, or download the standalone
  [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/))
  — provides `signtool.exe`.
- **GitHub CLI** (`gh`) — used by the signing script to download artifacts,
  re-upload signed files, and trigger the publish workflow. Install from
  <https://cli.github.com/> and run `gh auth login`.

---

## Part 2: Connecting SimplySign Desktop before signing

SimplySign Desktop must be **running and connected** whenever you want to sign.
The connection is valid for approximately 2 hours; after that you need to
reconnect.

1. Right-click the **SimplySign Desktop tray icon** (near the system clock).
2. Choose **Connect to SimplySign**.
3. Open the **SimplySign mobile app** on your phone and generate a **token**.
4. In the desktop dialog enter your **username** (your Certum/SimplySign email)
   and the **token**, then click **OK**.
5. The tray notification confirms connection and lists your card(s) and
   certificate(s).

Once connected, your code-signing certificate is visible in the Windows
certificate store (`Cert:\CurrentUser\My`) and `signtool` can use it.

**Pinless cards:** The Certum Open Source Code Signing certificate is issued
without a PIN. Signing proceeds immediately — no PIN prompt will appear.

---

## Part 3: Signing a release

After receiving the GitHub notification email that builds are complete:

```powershell
# 1. Make sure SimplySign Desktop is connected (Part 2 above)

# 2. Pull latest (the script lives in scripts/)
git pull

# 3. Run the signing script
.\scripts\sign-windows-release.ps1
# Or for a specific tag:
.\scripts\sign-windows-release.ps1 -Tag v1.0.0
# Or against a fork:
.\scripts\sign-windows-release.ps1 -Tag v1.0.0 -Repo myfork/pythonscad
```

The script will:

1. Confirm SimplySign is connected (certificate found in the Windows store).
2. Download the NSIS installer (`.exe`) and, if present, the MSIX package from
   the pre-release.
3. Sign each file with `signtool` using the Certum certificate.
4. Verify each signature (`Get-AuthenticodeSignature` → `Status: Valid`).
5. Re-upload the signed files to the release (overwriting the unsigned originals).
6. Trigger the `publish-release.yml` workflow via `gh workflow run`, which
   computes all checksums and makes the release public.

That's it — no further manual steps needed.

---

## Part 4: What gets signed and why

| Artifact | Signed | Reason |
| --- | --- | --- |
| NSIS installer `.exe` | **Yes** | Windows SmartScreen flags unsigned installers as from "Unknown Publisher" |
| MSIX package `.msix` | **Yes** (≥ 1.0.0) | Windows requires a valid Authenticode signature to install an MSIX |
| ZIP distribution `.zip` | No | Archives are not Authenticode-signable in a meaningful way |
| `pythonscad.exe` inside the ZIP | No | Optional; the installer signature is what end-users encounter |

---

## Part 5: Verifying signatures on release artifacts

```powershell
# Verify NSIS installer
Get-AuthenticodeSignature .\PythonSCAD-*-Installer.exe | Format-List Status, SignerCertificate

# Verify MSIX
Get-AuthenticodeSignature .\PythonSCAD-*.msix | Format-List Status, SignerCertificate
```

Both should show `Status: Valid` and a `SignerCertificate` issued by
`Certum Code Signing 2021 CA`.

On Linux with osslsigncode:

```bash
osslsigncode verify -in PythonSCAD-*-Installer.exe
```

---

## Part 6: Automation (future)

The current approach is intentionally manual — SimplySign Desktop requires an
interactive session (username + TOTP token from the mobile app, valid ~2 hours)
that is difficult to provide on ephemeral GitHub-hosted runners.

Two automation paths have been investigated for future reference:

**TOTP automation on a self-hosted runner:** The SimplySign QR code contains a
standard `otpauth://` URI, which means the TOTP token can be generated
programmatically from the stored secret. Combined with a self-hosted Windows
runner that has SimplySign Desktop installed and a script that types the
generated token into the desktop app, fully automated signing is possible. See
<https://www.devas.life/how-to-automate-signing-your-windows-app-with-certum/>
for a worked example. The session expires after ~2 hours.

**Linux container approach:** Run SimplySign Desktop inside an Xvnc container,
expose a p11-kit socket, and use `osslsigncode` against it. See
<https://github.com/hpvb/certum-container>. Requires one interactive VNC
login per container restart.

---

## Summary checklist (per release)

1. Receive GitHub notification email: release body contains "Awaiting Windows code signing".
2. Connect SimplySign Desktop (tray icon → Connect to SimplySign, username + mobile token).
3. Run `.\scripts\sign-windows-release.ps1` in PowerShell.
4. Done — the script triggers the publish workflow automatically.

Keep your SimplySign account credentials and mobile app safe. There is no
private key to protect — it never leaves Certum's HSM.

<!-- markdownlint-enable MD013 -->
