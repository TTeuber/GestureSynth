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
