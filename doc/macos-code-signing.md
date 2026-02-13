# macOS Code Signing and Notarization (Without a Mac)

<!-- markdownlint-disable MD013 -->

This guide explains how to sign and optionally notarize PythonSCAD macOS builds in GitHub Actions when you do **not** have access to a Mac.
You only need an Apple Developer account and a Linux (or Windows) machine to create the certificate once.

## Overview

- **Code signing** uses an Apple **Developer ID Application** certificate so users can open the app without "unidentified developer" / Gatekeeper block.
- **Notarization** tells Apple's servers that the app was checked; macOS Gatekeeper then allows it with less friction.
  Both the `.app` and the `.dmg` must be signed; you submit the `.dmg` for notarization.

The GitHub workflow uses macOS runners to build and sign.
You only need to create the certificate and secrets once from a non-Mac machine.

## Part 1: Create the Developer ID Application Certificate (on Linux)

You need a **Developer ID Application** certificate (for apps distributed outside the Mac App Store).
Apple does not require a Mac: generate a private key and CSR with OpenSSL, then use the Apple Developer portal.

### 1.1 Generate private key and CSR (OpenSSL)

On your Linux machine (or WSL):

```bash
# Create a directory and keep it private; the key must stay secret
mkdir -p ~/apple-signing && chmod 700 ~/apple-signing && cd ~/apple-signing

# RSA 2048 private key (Apple does not support 4096 for this)
openssl genrsa -out developer_id_application.key 2048

# Certificate Signing Request – replace email, CN, and C with your details
openssl req -new -key developer_id_application.key \
  -out CertificateSigningRequest.certSigningRequest \
  -subj "/emailAddress=your@email.com/CN=Your Name/C=US"
```

Use your Apple Developer account email for `emailAddress`, your name for `CN`, and your country code for `C`.

### 1.2 Request the certificate from Apple

1. Open [Certificates, Identifiers & Profiles → Certificates](https://developer.apple.com/account/resources/certificates/list).
2. Click **+** to add a certificate.
3. Under **Software**, select **Developer ID Application** → **Continue**.
4. When asked for a CSR, choose **Choose File** and upload `CertificateSigningRequest.certSigningRequest`.
5. Continue and **Download** the issued certificate (e.g. `developerID_application.cer`).
   Do **not** lose `developer_id_application.key`; you need it to build the `.p12`.

### 1.3 Download Apple's intermediate certificate

Apple's code signing certificates are chained to an Apple CA. You need the right intermediate:

- **Apple Worldwide Developer Relations – G4 (Recommended):**
  [Apple WWDRCAG4.cer](https://www.apple.com/certificateauthority/AppleWWDRCAG4.cer)

Download it into the same directory (e.g. `~/apple-signing`).

### 1.4 Convert to PEM and create the .p12 (on Linux)

Run (adjust filenames if yours differ):

```bash
cd ~/apple-signing

# Convert Apple's .cer to PEM
openssl x509 -inform der -in AppleWWDRCAG4.cer -out AppleWWDRCAG4.pem

# Convert your Developer ID certificate to PEM (replace with your .cer filename)
openssl x509 -inform der -in developerID_application.cer -out developer_id_application.pem

# Create PKCS#12 (.p12) – you will be asked for an export password (remember it for GitHub)
openssl pkcs12 -export -legacy \
  -out developer_id_application.p12 \
  -inkey developer_id_application.key \
  -in developer_id_application.pem \
  -certfile AppleWWDRCAG4.pem
```

On OpenSSL 3.x, if `-legacy` is not supported, try without it first.
If you see legacy provider errors, add `-legacy` and ensure OpenSSL is built with legacy support.

### 1.5 Encode the .p12 for GitHub

GitHub secrets cannot hold binary files; store the certificate as base64:

```bash
base64 -w 0 developer_id_application.p12
```

Copy the entire output. You will paste it into a GitHub repository secret (e.g. `BUILD_CERTIFICATE_BASE64`).

---

## Part 2: GitHub Secrets for Code Signing

In your repo: **Settings → Secrets and variables → Actions**, add:

|Secret name|Description|
|-----------|-----------|
|`BUILD_CERTIFICATE_BASE64`|Full base64 output of your `.p12` file (from step 1.5).|
|`P12_PASSWORD`|The password you set when creating the `.p12` in step 1.4.|
|`KEYCHAIN_PASSWORD`|Any random string; the workflow creates a temporary keychain.|
|`SIGNING_IDENTITY`|`Developer ID Application: Your Name (TEAM_ID)`. Team ID from Apple Developer Membership. On Mac: `security find-identity -v -p codesigning` after import.|

The workflow uses these only when `BUILD_CERTIFICATE_BASE64` is set.
If it is not set, the build still runs with ad-hoc signing (no proper Developer ID).

---

## Part 3: (Optional) Notarization

Notarization reduces Gatekeeper warnings for users.
It uses an **App Store Connect API Key**, not your Apple ID password.

### 3.1 Create an App Store Connect API key

1. Go to [App Store Connect](https://appstoreconnect.apple.com/) → **Users and Access** → **Integrations** → **App Store Connect API** (sign in as needed).
2. Create a new API key; give it a role that includes **App Manager** or **Admin** (or at least notarization).
3. **Download the `.p8` file once** – it cannot be downloaded again.
4. Note:
   - **Key ID** (e.g. 10-character string),
   - **Issuer ID** (UUID, shown on the same page).

### 3.2 GitHub secrets for notarization

Add these secrets if you want the workflow to notarize the DMG:

|Secret name|Description|
|-----------|-----------|
|`APPLE_API_KEY_P8_BASE64`|Base64 of the `.p8` file: `base64 -w 0 AuthKey_XXXXXXXX.p8`|
|`APPLE_API_KEY_ID`|The Key ID from App Store Connect.|
|`APPLE_ISSUER_ID`|The Issuer ID (UUID) from App Store Connect.|

The workflow will use `xcrun notarytool` with these to submit the signed DMG and then staple the notarization ticket.

---

## Part 4: How the workflow uses this

- **Build and upload macOS build** (`.github/workflows/build-macos-release.yml`):
  - If `BUILD_CERTIFICATE_BASE64` is set, the job decodes the `.p12`, creates a temporary keychain, imports the certificate, and signs the merged `.app` and `.dmg` with `SIGNING_IDENTITY`.
  - If `APPLE_API_KEY_P8_BASE64` (and related secrets) are set, after signing it runs `notarytool submit` on the DMG and staples the result.
- If you do not set these secrets, the workflow still runs and produces an ad-hoc signed build (testing only; not for public distribution).

---

## Summary checklist (without a Mac)

1. On Linux: generate key + CSR with OpenSSL.
2. In Apple Developer: create **Developer ID Application** certificate using that CSR; download `.cer`.
3. On Linux: download Apple WWDR G4 `.cer`, convert to PEM; convert your `.cer` to PEM; create `.p12` with OpenSSL (see 1.4).
4. Base64 the `.p12` and store it plus the `.p12` password, keychain password, and signing identity in GitHub secrets (Part 2).
5. (Optional) Create App Store Connect API key, base64 the `.p8`, add notarization secrets (Part 3).
6. Trigger the macOS build workflow; it will sign (and optionally notarize) when secrets are present.

Keep `developer_id_application.key` and the `.p12` in a safe place; never commit them or expose them in logs.

<!-- markdownlint-enable MD013 -->
