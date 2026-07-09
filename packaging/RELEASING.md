# Releasing Gesture Synth

## How a release works

1. Bump the version in the `VERSION` file (single source of truth — CMake, the
   installers, and the in-app update checker all read from it). **No trailing
   newline** in that file.
2. Commit, then tag and push:

   ```sh
   git tag v0.1.1        # must match the VERSION file contents
   git push origin main v0.1.1
   ```

3. CI builds all platforms and creates a **draft-quality GitHub Release**
   (marked *pre-release*) containing:
   - `Gesture Synth-<version>-macOS.pkg` — the macOS installer (Standalone app,
     VST3, AU, CLAP; signed + notarized when the secrets below are configured)
   - `Gesture Synth-<version>-Windows.exe` — the Windows installer
   - platform zips of the raw binaries
4. **Edit the GitHub Release and uncheck "pre-release" to publish it.** This is
   deliberate: the in-app update checker uses GitHub's `releases/latest`
   endpoint, which only returns full (non-prerelease, non-draft) releases. So
   users are notified only after you flip that switch.

## The in-app update checker

`source/Utility/UpdateChecker.h` checks
`api.github.com/repos/TTeuber/GestureSynth/releases/latest` on a background
thread when the editor opens, at most once per 24 hours (cached in a
properties file). If the release tag is newer than the built-in `VERSION`, an
"Update to vX.Y.Z" button appears next to the menu button; clicking it opens
the release page. All failures (offline, rate limit, sandboxed host) are
silent.

## macOS signing & notarization (one-time setup)

Without these secrets CI still produces a working **unsigned** installer, but
Gatekeeper makes users right-click → Open to run it. For a friction-free
install, join the [Apple Developer Program](https://developer.apple.com/programs/)
($99/year) and then:

1. **Create two certificates** at developer.apple.com → Certificates (or via
   Xcode → Settings → Accounts → Manage Certificates):
   - *Developer ID Application* (signs the app/plugin bundles)
   - *Developer ID Installer* (signs the .pkg)
2. **Export each as a `.p12`** from Keychain Access (select the certificate,
   File → Export Items), choosing an export password.
3. **Base64-encode them** for GitHub:

   ```sh
   base64 -i DeveloperIDApplication.p12 | pbcopy
   ```

4. **Create an app-specific password** for notarization at
   [account.apple.com](https://account.apple.com) → Sign-In and Security →
   App-Specific Passwords.
5. **Find your Team ID** at developer.apple.com → Membership details.
6. **Add the repository secrets** (Settings → Secrets and variables → Actions,
   or `gh secret set NAME`):

   | Secret | Value |
   | --- | --- |
   | `DEV_ID_APP_CERT` | base64 of the Developer ID **Application** .p12 |
   | `DEV_ID_APP_PASSWORD` | export password for that .p12 |
   | `DEV_ID_INSTALLER_CERT` | base64 of the Developer ID **Installer** .p12 |
   | `DEV_ID_INSTALLER_PASSWORD` | export password for that .p12 |
   | `NOTARIZATION_APPLE_ID` | your Apple ID email |
   | `NOTARIZATION_PASSWORD` | the app-specific password |
   | `APPLE_TEAM_ID` | your 10-character Team ID |

The next tagged build will sign, notarize, and staple the installer
automatically — no workflow changes needed.

## Windows & Linux signing

**Linux — nothing to sign.** Linux has no Gatekeeper/notarization equivalent;
running the binary raises no OS trust prompt. "Signing" would only mean
GPG-signing packages for an apt/rpm repo (irrelevant for a portfolio zip) or
shipping a checksum for integrity. If desired, publish a `sha256sum` of the zip
alongside the release — no certificate, cost, or CI credentials required.

**Windows — currently unsigned (deliberate).** The `.exe` installer ships
unsigned, so SmartScreen shows *"Windows protected your PC" → More info → Run
anyway* on first launch. Note this workaround in the release notes.

To remove that prompt, use **Azure Trusted Signing** (~$10/month) — the modern,
CI-friendly path. Traditional OV/EV code-signing certs are a poor fit: since
June 2023 their private keys must live on a FIPS hardware token or cloud HSM, so
they can't be handed to GitHub Actions as a file the way the Apple `.p12`s are.

Azure Trusted Signing eligibility note: the individual/public offering requires
the signing identity to be ~3 years old (Microsoft anti-fraud rule), so a brand
new Azure account may not qualify yet. When ready, the setup is:

1. In Azure, create a **Trusted Signing account** and a **certificate profile**,
   and an **app registration** (service principal) with the *Trusted Signing
   Certificate Profile Signer* role.
2. Add the repository secrets: `AZURE_TENANT_ID`, `AZURE_CLIENT_ID`,
   `AZURE_CLIENT_SECRET` (plus the endpoint / account name / profile name as
   workflow inputs).
3. Add an `azure/trusted-signing-action` step to the Windows leg of
   `build_and_test.yml`, after *Generate Installer*, pointing at the built
   `.exe`. (The project's original template scaffolded this step; it was removed
   pending an Apple/Azure signing decision and can be restored from git history.)

## Testing the installer locally

```sh
cmake --build cmake-build-relwithdebinfo --target GestureSynth_Standalone GestureSynth_VST3 GestureSynth_AU GestureSynth_CLAP -j12

ARTIFACTS_PATH=cmake-build-relwithdebinfo/GestureSynth_artefacts/RelWithDebInfo \
PRODUCT_NAME="Gesture Synth" VERSION=$(cat VERSION) \
BUNDLE_ID=com.tylerteuber.gesturesynth OUTPUT_DIR=/tmp/pkgtest \
./packaging/package-macos.sh
```

If Developer ID identities are in your login keychain the script signs with
them; otherwise it prints a note and produces an unsigned pkg.
