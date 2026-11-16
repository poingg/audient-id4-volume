#!/bin/bash
set -euo pipefail

TARGET_DB="${1:--20}"
if ! [[ "$TARGET_DB" =~ ^-?[0-9]+$ ]] || (( TARGET_DB < -127 || TARGET_DB > 0 )); then
  echo "Usage: $0 [target-dB]"
  echo "Choose an integer from -127 to 0, e.g. $0 -20"
  exit 2
fi

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required: https://brew.sh"
  exit 1
fi

brew list libusb >/dev/null 2>&1 || brew install libusb

APP_DIR="$HOME/Library/Application Support/AudientStartupVolume"
AGENT_DIR="$HOME/Library/LaunchAgents"
BIN="$APP_DIR/audient-startup-volume"
PLIST="$AGENT_DIR/com.local.audient-startup-volume.plist"

mkdir -p "$APP_DIR" "$AGENT_DIR"

LIBUSB_PREFIX="$(brew --prefix libusb)"
clang -O2 -Wall -Wextra \
  -I"$LIBUSB_PREFIX/include" \
  "$PWD/audient-startup-volume.c" \
  -L"$LIBUSB_PREFIX/lib" -lusb-1.0 \
  -Wl,-rpath,"$LIBUSB_PREFIX/lib" \
  -o "$BIN"

chmod 755 "$BIN"

cat > "$PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.local.audient-startup-volume</string>
    <key>ProgramArguments</key>
    <array>
        <string>$BIN</string>
        <string>$TARGET_DB</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <false/>
    <key>StandardOutPath</key>
    <string>$APP_DIR/startup.log</string>
    <key>StandardErrorPath</key>
    <string>$APP_DIR/startup.log</string>
</dict>
</plist>
EOF

# Reload the agent if this installer is run more than once.
launchctl bootout "gui/$(id -u)" "$PLIST" >/dev/null 2>&1 || true
launchctl bootstrap "gui/$(id -u)" "$PLIST"
launchctl kickstart -k "gui/$(id -u)/com.local.audient-startup-volume"

echo
echo "Installed. Target volume: ${TARGET_DB} dB"
echo "LaunchAgent: $PLIST"
echo "Log: $APP_DIR/startup.log"
echo "It will run once at each login."
