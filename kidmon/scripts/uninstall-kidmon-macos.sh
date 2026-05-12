#!/usr/bin/env bash
set -euo pipefail

# ---------------------------------------------------------------------------
# uninstall-kidmon.sh  <agent|daemon>
# ---------------------------------------------------------------------------

LABEL="com.kidmon.server"
PLIST_NAME="${LABEL}.plist"

# ---- argument parsing ------------------------------------------------------

usage() {
    echo "Usage: $0 <agent|daemon>"
    exit 1
}

[[ $# -lt 1 ]] && usage

INSTALL_TYPE="$1"

case "$INSTALL_TYPE" in
    agent|daemon) ;;
    *) echo "error: install type must be 'agent' or 'daemon' (got: $INSTALL_TYPE)"; usage ;;
esac

# ---- resolve paths ---------------------------------------------------------

if [[ "$INSTALL_TYPE" == "daemon" ]]; then
    PLIST_PATH="/Library/LaunchDaemons/${PLIST_NAME}"
else
    PLIST_PATH="$HOME/Library/LaunchAgents/${PLIST_NAME}"
fi

# ---- check -----------------------------------------------------------------

if [[ ! -f "$PLIST_PATH" ]]; then
    echo "error: plist not found at $PLIST_PATH — is kidmon installed as $INSTALL_TYPE?"
    exit 1
fi

# ---- uninstall -------------------------------------------------------------

echo "==> Uninstalling kidmon ($INSTALL_TYPE)"
echo "    plist : $PLIST_PATH"

if [[ "$INSTALL_TYPE" == "daemon" ]]; then
    sudo launchctl unload "$PLIST_PATH"
    sudo rm -f "$PLIST_PATH"
else
    launchctl unload "$PLIST_PATH"
    rm -f "$PLIST_PATH"
fi

echo "==> Done. The log directory was left in place."
