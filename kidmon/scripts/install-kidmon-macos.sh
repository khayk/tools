#!/usr/bin/env bash
set -euo pipefail

# ---------------------------------------------------------------------------
# install-kidmon.sh  <binary-path>  <agent|daemon>  [--token <token>]
#
#   agent   → ~/Library/LaunchAgents/   (runs as current user, stops on logout)
#   daemon  → /Library/LaunchDaemons/   (runs as root, survives logout, needs sudo)
# ---------------------------------------------------------------------------

LABEL="com.kidmon.server"
PLIST_NAME="${LABEL}.plist"
TOKEN=""

# ---- argument parsing ------------------------------------------------------

usage() {
    echo "Usage: $0 <binary-path> <agent|daemon> [--token <value>]"
    echo ""
    echo "  binary-path   Absolute path to the kidmon-app binary"
    echo "  agent         Install as a per-user LaunchAgent"
    echo "  daemon        Install as a system LaunchDaemon (requires sudo)"
    echo "  --token       Authorization token passed to kidmon (optional)"
    exit 1
}

[[ $# -lt 2 ]] && usage

BINARY_PATH="$1"
INSTALL_TYPE="$2"
shift 2

while [[ $# -gt 0 ]]; do
    case "$1" in
        --token)
            [[ $# -lt 2 ]] && { echo "error: --token requires a value"; exit 1; }
            TOKEN="$2"
            shift 2
            ;;
        *) echo "error: unknown argument: $1"; usage ;;
    esac
done

# ---- validate --------------------------------------------------------------

if [[ ! -f "$BINARY_PATH" ]]; then
    echo "error: binary not found: $BINARY_PATH"
    exit 1
fi

if [[ "$BINARY_PATH" != /* ]]; then
    echo "error: binary-path must be absolute (got: $BINARY_PATH)"
    exit 1
fi

case "$INSTALL_TYPE" in
    agent|daemon) ;;
    *) echo "error: install type must be 'agent' or 'daemon' (got: $INSTALL_TYPE)"; usage ;;
esac

# ---- resolve paths ---------------------------------------------------------

if [[ "$INSTALL_TYPE" == "daemon" ]]; then
    PLIST_DIR="/Library/LaunchDaemons"
    LOG_DIR="/var/log/kidmon"
    LAUNCHCTL_DOMAIN="system"
else
    PLIST_DIR="$HOME/Library/LaunchAgents"
    LOG_DIR="$HOME/Library/Logs/kidmon"
    LAUNCHCTL_DOMAIN="gui/$(id -u)"
fi

PLIST_PATH="${PLIST_DIR}/${PLIST_NAME}"

# ---- build program arguments block -----------------------------------------

build_program_arguments() {
    echo "    <key>ProgramArguments</key>"
    echo "    <array>"
    echo "        <string>${BINARY_PATH}</string>"
    if [[ -n "$TOKEN" ]]; then
        echo "        <string>--token</string>"
        echo "        <string>${TOKEN}</string>"
    fi
    echo "    </array>"
}

# ---- generate plist --------------------------------------------------------

generate_plist() {
    cat <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>${LABEL}</string>

$(build_program_arguments)

    <key>KeepAlive</key>
    <true/>

    <key>RunAtLoad</key>
    <true/>

    <key>StandardOutPath</key>
    <string>${LOG_DIR}/stdout.log</string>
    <key>StandardErrorPath</key>
    <string>${LOG_DIR}/stderr.log</string>
</dict>
</plist>
EOF
}

# ---- install ---------------------------------------------------------------

echo "==> Installing kidmon as $INSTALL_TYPE"
echo "    binary : $BINARY_PATH"
echo "    plist  : $PLIST_PATH"
echo "    logs   : $LOG_DIR"

mkdir -p "$LOG_DIR"

if [[ "$INSTALL_TYPE" == "daemon" ]]; then
    TMP_PLIST="$(mktemp /tmp/${PLIST_NAME}.XXXXXX)"
    generate_plist > "$TMP_PLIST"
    sudo mkdir -p "$PLIST_DIR"
    sudo cp "$TMP_PLIST" "$PLIST_PATH"
    rm -f "$TMP_PLIST"
    sudo chown root:wheel "$PLIST_PATH"
    sudo chmod 644 "$PLIST_PATH"
    sudo launchctl load "$PLIST_PATH"
else
    mkdir -p "$PLIST_DIR"
    generate_plist > "$PLIST_PATH"
    launchctl load "$PLIST_PATH"
fi

# ---- print instructions ----------------------------------------------------

if [[ "$INSTALL_TYPE" == "daemon" ]]; then
    START="sudo launchctl start ${LABEL}"
    STOP="sudo launchctl stop  ${LABEL}"
    LIST="sudo launchctl list | grep kidmon"
    UNLOAD="sudo launchctl unload ${PLIST_PATH}"
else
    START="launchctl start ${LABEL}"
    STOP="launchctl stop  ${LABEL}"
    LIST="launchctl list | grep kidmon"
    UNLOAD="launchctl unload ${PLIST_PATH}"
fi

cat <<EOF

==> Installation complete

Manage the service:
  Start  : ${START}
  Stop   : ${STOP}
  Status : ${LIST}

Uninstall:
  $(dirname "$0")/uninstall-kidmon-macos.sh ${INSTALL_TYPE}
  -- or manually --
  ${UNLOAD}
  rm ${PLIST_PATH}

Logs:
  ${LOG_DIR}/stdout.log
  ${LOG_DIR}/stderr.log
EOF
