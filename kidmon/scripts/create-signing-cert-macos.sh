#!/usr/bin/env bash
set -euo pipefail

# ---------------------------------------------------------------------------
# create-signing-cert-macos.sh  [identity-name]
#
# Creates a self-signed code-signing certificate in the login keychain so the
# kidmon bundle can be signed with a STABLE identity. A stable identity is what
# lets macOS keep the Screen Recording grant across rebuilds: TCC keys its
# database on the code-signing designated requirement, not on the binary hash.
#
# Default identity name: "Kidmon Self-Signed" (must match KIDMON_CODESIGN_IDENTITY
# in CMake).
#
# If this CLI flow fails on your machine, create the certificate manually:
#   Keychain Access -> Certificate Assistant -> Create a Certificate...
#     Name:              Kidmon Self-Signed
#     Identity Type:     Self Signed Root
#     Certificate Type:  Code Signing
# ---------------------------------------------------------------------------

IDENTITY="${1:-Kidmon Self-Signed}"
KEYCHAIN="${HOME}/Library/Keychains/login.keychain-db"

# codesign looks up the signing identity by name in the keychain SEARCH LIST,
# not via --keychain. On some machines that list is empty, so codesign reports
# "no identity found" even though the cert exists. Make sure the login keychain
# is in the search list (preserving any existing entries) and is the default.
ensure_keychain_in_search_list() {
    local existing
    existing=$(security list-keychains -d user | sed -e 's/^[[:space:]]*//' -e 's/"//g')
    if ! grep -qF "$KEYCHAIN" <<<"$existing"; then
        echo "==> Adding login keychain to the search list"
        # shellcheck disable=SC2086
        security list-keychains -d user -s "$KEYCHAIN" $existing
    fi
    security default-keychain -d user >/dev/null 2>&1 || \
        security default-keychain -d user -s "$KEYCHAIN" >/dev/null 2>&1 || true
}

ensure_keychain_in_search_list

# Note: no -v. A self-signed cert is reported untrusted (CSSMERR_TP_NOT_TRUSTED)
# and hidden by -v, but codesign signs with it fine -- trust is only needed for
# verification, not signing. Reference the keychain explicitly: the default
# search list can be empty in some sessions.
if security find-identity -p codesigning "$KEYCHAIN" | grep -qF "$IDENTITY"; then
    echo "==> Code-signing identity '$IDENTITY' already exists. Nothing to do."
    exit 0
fi

echo "==> Creating self-signed code-signing certificate '$IDENTITY'"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# Self-signed cert with the codeSigning extended key usage.
openssl req -x509 -newkey rsa:2048 -nodes -days 3650 \
    -keyout "$TMP/key.pem" -out "$TMP/cert.pem" \
    -subj "/CN=${IDENTITY}" \
    -addext "keyUsage=critical,digitalSignature" \
    -addext "extendedKeyUsage=critical,codeSigning"

# OpenSSL 3 defaults to PKCS12 encryption/MAC algorithms that Apple's
# `security import` cannot read ("MAC verification failed"). -legacy restores
# the older 3DES/RC2 + SHA1 scheme that macOS expects. LibreSSL (system
# /usr/bin/openssl) already uses those and does not know the flag.
LEGACY=()
if openssl pkcs12 -help 2>&1 | grep -q -- '-legacy'; then
    LEGACY=(-legacy)
fi

# A non-empty transport password is required: macOS rejects empty-password
# PKCS12 from OpenSSL 3 with "MAC verification failed". The .p12 is a temporary
# transport container (deleted on exit), so the value itself does not matter.
P12_PASS="kidmon"
openssl pkcs12 -export "${LEGACY[@]}" -inkey "$TMP/key.pem" -in "$TMP/cert.pem" \
    -out "$TMP/identity.p12" -passout "pass:${P12_PASS}"

# Import the identity, allowing codesign to use the private key.
security import "$TMP/identity.p12" -k "$KEYCHAIN" -P "$P12_PASS" \
    -T /usr/bin/codesign -T /usr/bin/security

# Pre-authorize codesign so it can use the key without a GUI prompt on every
# build. This needs the login-keychain password. Prompt for it; if skipped,
# macOS will instead show a one-time "Always Allow" dialog on the first build.
echo "==> Authorizing codesign to use the key."
printf "    Enter your macOS login password (or press Enter to skip): "
read -r -s LOGIN_PW
echo
if [[ -n "$LOGIN_PW" ]]; then
    security set-key-partition-list -S apple-tool:,apple:,codesign: -s \
        -k "$LOGIN_PW" "$KEYCHAIN" >/dev/null
    unset LOGIN_PW
else
    echo "    Skipped. Click \"Always Allow\" when macOS prompts during the first build."
fi

if security find-identity -p codesigning "$KEYCHAIN" | grep -qF "$IDENTITY"; then
    echo "==> Done. '$IDENTITY' is ready for code signing."
    echo "    Rebuild kidmon and the bundle will be signed automatically."
else
    echo "warning: identity not visible to codesign yet. If signing fails, create"
    echo "         the certificate via Keychain Access (see header of this script)."
fi
