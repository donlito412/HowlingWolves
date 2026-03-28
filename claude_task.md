# Claude/Cursor Task: Fix Azure OIDC Authentication for GitHub Actions

## Problem
The GitHub Actions build fails at the **Azure Login** step with:
```
AADSTS70025: The client 'wolfpacc-github-signing' has no configured federated identity credentials.
```

The `azure/login@v2` action uses OIDC (OpenID Connect) to authenticate. The Azure App Registration (`wolfpacc-github-signing`) needs a **Federated Identity Credential** that trusts GitHub Actions tokens from this specific repo.

## Fix Option A: Add Federated Credential in Azure Portal
1. Go to **Azure Portal** → **App registrations** → find `wolfpacc-github-signing`
2. Click **Certificates & secrets** → **Federated credentials** → **Add credential**
3. Select **GitHub Actions deploying Azure resources**
4. Fill in:
   - **Organization**: `donlito412`
   - **Repository**: `HowlingWolves`
   - **Entity type**: `Branch`
   - **Branch**: `main`
   - **Name**: `github-actions-main`
5. Click **Add**
6. Repeat step 2-5 but with **Entity type**: `Tag` and **Tag**: `*` (name it `github-actions-tags`) so version tag pushes also work.

## Fix Option B: Switch to Client Secret Auth (simpler)
If OIDC is too complex, change the Azure Login step in `.github/workflows/build.yml` to use a client secret instead:

Replace the current Azure Login step (around line 62):
```yaml
    - name: Azure Login
      if: runner.os == 'Windows'
      uses: azure/login@v2
      with:
        client-id: ${{ secrets.AZURE_CLIENT_ID }}
        tenant-id: ${{ secrets.AZURE_TENANT_ID }}
        subscription-id: ${{ secrets.AZURE_SUBSCRIPTION_ID }}
```

With:
```yaml
    - name: Azure Login
      if: runner.os == 'Windows'
      uses: azure/login@v2
      with:
        creds: '{"clientId":"${{ secrets.AZURE_CLIENT_ID }}","clientSecret":"${{ secrets.AZURE_CLIENT_SECRET }}","subscriptionId":"${{ secrets.AZURE_SUBSCRIPTION_ID }}","tenantId":"${{ secrets.AZURE_TENANT_ID }}"}'
```

And remove `id-token: write` from the permissions block (line 14) since it's only needed for OIDC.

**Important**: The user must have `AZURE_CLIENT_SECRET` set as a GitHub repo secret. This is the client secret from the `wolfpacc-github-signing` App Registration in Azure.

## After Fixing
Push the change. The build should get past Azure Login and proceed to sign the VST3 binaries and installer.

## Additional Context
- **Azure Trusted Signing Account**: `WolfPaccSigning`
- **Certificate Profile**: `WolfPaccAudioSignature`
- **Endpoint**: `https://prodeastus.codesigning.azure.net`
- The macOS build passed fine. Only Windows fails because signing is Windows-only.
