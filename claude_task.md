# Claude Task: GitHub Pages Deployment

The following frontend updates have been made to the local codebase:
1. GA4 Tracking code (`G-HM1J0JWXL9`) injected into the `<head>` of all 16 HTML pages.
2. The MailerLite global footer embedded form ID was updated to `mQWonf` to decouple generic newsletter signups from the free teaser pack funnels.
3. The "Coming Soon" placeholder buttons on `docs/bundles/index.html` were commented out.

**Your Objective:**
1. The global `git push` to `origin HEAD` is hanging due to the massive size of the repository's audio assets and VST source files (4,250+ objects). Please isolate the modified files inside the `docs/` deployment directory, commit them properly, and circumvent the hanging push issue to get the website updates live on GitHub Pages immediately.
2. **Review Signing Instructions**: Azure Trusted Signing is now fully configured and automated via GitHub Actions. Detailed instructions for repository secrets and version tagging are found in `vst_signing_instructions.md` in the project root.
