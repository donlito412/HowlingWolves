# Automated VST Build & Signing Instructions

The build and signing process is now fully integrated into your GitHub Actions workflow (`.github/workflows/build.yml`). You no longer need to sign files manually on a Windows machine.

## Azure resources (must match the workflow)

The Windows job uses **`azure/artifact-signing-action@v1.2.0`** with:

| Setting | Value in `build.yml` |
| :--- | :--- |
| **Endpoint** | `https://prodeastus.codesigning.azure.net` |
| **Signing account** (`signing-account-name`) | `WolfPaccSigning` |
| **Certificate profile** | `WolfPaccAudioSignature` |

If Azure names or region change, update `endpoint`, `signing-account-name`, and `certificate-profile-name` in `.github/workflows/build.yml` (and `build-promo.yml` if you use it) to match the portal **exactly**. Setup reference: [Quickstart: Set up Artifact Signing](https://learn.microsoft.com/en-us/azure/trusted-signing/quickstart).

## Apple Developer (macOS sign + notarize)

The **Sign and Notarize (macOS)** and **Notarize (macOS)** steps in `.github/workflows/build.yml` use these **repository secrets** (same GitHub **Settings → Secrets and variables → Actions** page):

| Secret | What it is |
| :--- | :--- |
| `APPLE_CERTIFICATE_BASE64` | Base64 of your **Developer ID Application** `.p12` export (see below). |
| `APPLE_CERTIFICATE_PASSWORD` | Password you set when exporting the `.p12`. |
| `APPLE_ID` | Apple ID email used for notarization. |
| `APPLE_APP_SPECIFIC_PASSWORD` | App-specific password from [appleid.apple.com](https://appleid.apple.com) (Sign-In and Security → App-Specific Passwords). |
| `APPLE_TEAM_ID` | 10-character Team ID (Membership details in [developer.apple.com](https://developer.apple.com/account)). |
| `APPLE_CODESIGN_IDENTITY` *(optional)* | Full identity string exactly as shown for your cert, e.g. `Developer ID Application: Your Name (XXXXXXXXXX)`. If omitted, the workflow falls back to `Developer ID Application: Jonathan Freeman ($APPLE_TEAM_ID)` — change the secret or the fallback in YAML if your legal name differs. |

**One-time: create and export the signing certificate**

1. [developer.apple.com](https://developer.apple.com) → **Account** → **Certificates, Identifiers & Profiles** → **Certificates** → **+** → **Developer ID Application** (not “Mac Development”).  
2. Follow the steps to create a CSR from **Keychain Access** on your Mac, upload it, download the certificate, double-click to install.  
3. Keychain Access → **My Certificates** → expand **Developer ID Application: …** → select **both** the cert and its private key → right-click → **Export 2 items…** → **Personal Information Exchange (.p12)** → choose a password (that becomes `APPLE_CERTIFICATE_PASSWORD`).  
4. On your Mac, base64 the file for the secret:  
   `base64 -i YourExport.p12 | pbcopy`  
   Paste into GitHub as `APPLE_CERTIFICATE_BASE64`.

After secrets are set, pushes to `main` (same-repo PRs with secrets) run **codesign** on `.vst3`, `.component`, and `.app`, then **notarytool** on `HowlingWolves_MacOS.zip`.

## 1. Required GitHub Secrets (Azure / Windows)
To allow GitHub to authenticate with your Azure Trusted Signing account, add the following **Repository Secrets** in GitHub (**Settings > Secrets and variables > Actions**):

| Secret Name | How to find it |
| :--- | :--- |
| `AZURE_CLIENT_ID` | The 'Application (client) ID' of your Azure Service Principal / App Registration. |
| `AZURE_TENANT_ID` | The 'Directory (tenant) ID' from your Azure Portal. |
| `AZURE_SUBSCRIPTION_ID` | Your Azure Subscription ID. |
| `AZURE_CLIENT_SECRET` | A Client Secret generated for your App Registration. |

## 2. Automated Workflow Sequence
Once the secrets are added, every push to `main` (or a version tag) will:
1. **Build** the Howling Wolves VST3 and Standalone binaries.
2. **Login** to Azure using your secrets.
3. **Sign** the raw VST3 and DLL files.
4. **Compile** the Windows Installer using Inno Setup.
5. **Sign** the final installer `.exe`.
6. **Upload** all signed artifacts to the GitHub Action run.

## 3. Creating a Release
To trigger a formal GitHub Release with these signed files:
1. Tag your commit (e.g., `v1.0.20`).
2. Push the tag: `git push origin v1.0.20`.
3. The `release` job will automatically pick up the **signed** `.zip` and `.exe` files and attach them to the new Release.

## 4. Verification
After the GitHub Action completes:
- Download the `HowlingWolves_Win_Installer.exe` from the GitHub Release.
- Right-click -> **Properties** -> **Digital Signatures**. 
- It should show "Wolf Pacc Music Group LLC" as the verified signer.
