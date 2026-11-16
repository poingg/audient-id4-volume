#!/bin/bash
set -euo pipefail

PLIST="$HOME/Library/LaunchAgents/com.local.audient-startup-volume.plist"
APP_DIR="$HOME/Library/Application Support/AudientStartupVolume"

launchctl bootout "gui/$(id -u)" "$PLIST" >/dev/null 2>&1 || true
rm -f "$PLIST"
rm -rf "$APP_DIR"

echo "Audient Startup Volume login item removed."
echo "Homebrew's libusb package was left installed."
