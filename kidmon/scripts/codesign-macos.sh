#!/usr/bin/env bash
set -euo pipefail

# ---------------------------------------------------------------------------
# codesign-macos.sh  <identity-name>  <bundle-path>
#
# Build-time helper invoked by CMake to sign the kidmon .app bundle. If the
# requested identity is not present in the keychain it warns and exits 0, so a
# fresh checkout still builds without a certificate. Create the identity with
# scripts/create-signing-cert-macos.sh.
# ---------------------------------------------------------------------------

IDENTITY="$1"
BUNDLE="$2"
BUNDLE_ID="${3:-com.kidmon.app}"

# Reference the login keychain explicitly. Relying on the default search list is
# fragile -- it can be empty in some sessions, in which case find-identity and
# codesign see no identities at all.
KEYCHAIN="${HOME}/Library/Keychains/login.keychain-db"

# No -v: a self-signed identity is reported untrusted and hidden by -v, but
# codesign can still sign with it.
if ! security find-identity -p codesigning "$KEYCHAIN" | grep -qF "$IDENTITY"; then
    echo "warning: code-signing identity '$IDENTITY' not found; skipping signing."
    echo "         kidmon will not appear under its own name in Screen Recording"
    echo "         until it is signed. Create the identity with:"
    echo "             kidmon/scripts/create-signing-cert-macos.sh \"$IDENTITY\""
    exit 0
fi

echo "==> Code-signing $BUNDLE with '$IDENTITY'"
codesign --force --deep --keychain "$KEYCHAIN" \
    --identifier "$BUNDLE_ID" --sign "$IDENTITY" "$BUNDLE"
codesign --verify --verbose "$BUNDLE" || true
