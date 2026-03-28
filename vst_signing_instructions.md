# Automated VST Build & Signing Instructions

The build and signing process is now fully integrated into your GitHub Actions workflow (`.github/workflows/build.yml`). You no longer need to sign files manually on a Windows machine.

## Azure resources (must match the workflow)

The Windows job uses **`azure/trusted-signing-action@v0.5.0`** with:

| Setting | Value in `build.yml` |
| :--- | :--- |
| **Endpoint (East US)** | `https://eus.codesigning.azure.net/` |
| **Trusted Signing account** | `WolfPaccSigning` |
| **Certificate profile** | `WolfPaccCodeSigning` |

Create the **certificate profile** in the Azure portal if it does not exist yet: open your **Trusted Signing** (Artifact Signing) account **`WolfPaccSigning`** in region **East US**, add a certificate profile named exactly **`WolfPaccCodeSigning`**, and complete identity validation if prompted. Step-by-step: [Quickstart: Set up Artifact Signing](https://learn.microsoft.com/en-us/azure/trusted-signing/quickstart) (use the **East US** endpoint above for that region).

If you use a different profile name in Azure, change `certificate-profile-name` in `.github/workflows/build.yml` to match **exactly**.

## 1. Required GitHub Secrets
To allow GitHub to authenticate with your Azure Trusted Signing account, you must add the following **Repository Secrets** in GitHub (**Settings > Secrets and variables > Actions**):

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
