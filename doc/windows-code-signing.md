# Windows Code Signing with Certum SimplySign

<!-- markdownlint-disable MD013 -->

This guide explains how to sign PythonSCAD Windows release artifacts (NSIS
installer `.exe` and MSIX package) using a **Certum Open Source Code Signing**
certificate on the SimplySign cloud HSM. The private key never leaves Certum's
infrastructure.

## How it fits into the release flow

1. A Release Please PR is merged → a GitHub release is created as a
   **pre-release**.
2. All platform build workflows run automatically and upload their artifacts to
   the release.
3. The `publish-release.yml` workflow waits for all builds. When they all
   succeed it appends signing instructions to the release body and **stops**,
   leaving the release as a pre-release.
4. The maintainer (that's you) receives a GitHub notification, runs the local
   signing script (see Part 3), and triggers the Publish Release workflow
   manually.
5. The release is published and downstream workflows (PyPI, etc.) run.

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
  MSIX with a matching publisher identity (currently the MSIX uses an unsigned
  placeholder).

### 1.4 Install prerequisites

- **Windows SDK** (comes with Visual Studio, or download the standalone
  [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/))
  — provides `signtool.exe`.
- **GitHub CLI** (`gh`) — used by the signing script to download artifacts and
  re-upload signed files. Install from <https://cli.github.com/> and run
  `gh auth login`.

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
5. The tray icon should briefly show a notification confirming connection and
   listing your card(s) and certificate(s).

Once connected, your code-signing certificate is visible in the Windows
certificate store (`Cert:\CurrentUser\My`) and `signtool` can use it.

**Pinless cards:** The Certum Open Source Code Signing certificate is issued
without a PIN. Signing proceeds immediately — no PIN prompt will appear.

---

## Part 3: Signing a release (the normal release workflow)

After receiving the GitHub notification that builds are complete:

```powershell
# 1. Make sure SimplySign Desktop is connected (Part 2 above)

# 2. Pull latest (the script lives in scripts/)
git pull

# 3. Run the signing script — it downloads, signs, verifies, and re-uploads
.\scripts\sign-windows-release.ps1
# Or for a specific tag:
.\scripts\sign-windows-release.ps1 -Tag v0.20.0
```

The script will:

1. Confirm SimplySign is connected (certificate found in the Windows store).
2. Download the NSIS installer (`.exe`) and MSIX package from the GitHub
   release.
3. Sign both files with `signtool` using the Certum certificate.
4. Verify each signature (`Get-AuthenticodeSignature` → `Status: Valid`).
5. Re-upload the signed files to the release (overwriting the unsigned originals).

**No push notification is needed** for this certificate (pinless). The signing
happens immediately once SimplySign Desktop is connected.

After the script succeeds, trigger the Publish Release workflow manually:

```text
GitHub → Actions → Publish Release → Run workflow (leave tag empty)
```

---

## Part 4: What gets signed and why

| Artifact | Signed | Reason |
| --- | --- | --- |
| NSIS installer `.exe` | **Yes** | Windows SmartScreen flags unsigned installers as from "Unknown Publisher" |
| MSIX package `.msix` | **Yes** | Windows requires a valid Authenticode signature to install an MSIX |
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
interactive session (username + TOTP token from the mobile app) that is
difficult to provide on ephemeral GitHub-hosted runners.

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

1. Receive GitHub notification: "Awaiting Windows code signing".
2. Connect SimplySign Desktop (tray icon → Connect to SimplySign, username + mobile token).
3. Run `.\scripts\sign-windows-release.ps1` in PowerShell.
4. Trigger Publish Release workflow manually on GitHub Actions.

Keep your SimplySign account credentials and mobile app safe. There is no
private key to protect — it never leaves Certum's HSM.

<!-- markdownlint-enable MD013 -->
