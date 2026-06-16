#!/usr/bin/env bash
# Build + netload to the 3DS in one shot.
#   ./deploy.sh <3ds-ip>   first time (IP is remembered in .3ds-ip)
#   ./deploy.sh            thereafter
#
# Works under WSL2 NAT: connects to the 3DS by explicit IP (no UDP broadcast).
# On the 3DS: open the Homebrew Launcher and press Y to wait for netload.
set -euo pipefail
cd "$(dirname "$0")"

# Make sure the devkitPro environment is available even in a non-login shell.
if [ -z "${DEVKITARM:-}" ] && [ -f /etc/profile.d/devkit-env.sh ]; then
	# shellcheck disable=SC1091
	source /etc/profile.d/devkit-env.sh
fi

DKP="${DEVKITPRO:-/opt/devkitpro}"
LINK="$DKP/tools/bin/3dslink"
[ -x "$LINK" ] || LINK="$(command -v 3dslink || true)"
if [ -z "$LINK" ]; then
	echo "ERROR: 3dslink not found. Is devkitPro installed and DEVKITPRO set?" >&2
	exit 1
fi

IP_FILE=".3ds-ip"
IP="${1:-}"
[ -z "$IP" ] && [ -f "$IP_FILE" ] && IP="$(cat "$IP_FILE")"
if [ -z "$IP" ]; then
	echo "Usage: ./deploy.sh <3ds-ip>" >&2
	echo "Find it on the 3DS: Homebrew Launcher -> press Y (netload). It shows the IP it's listening on." >&2
	echo "(Or System Settings -> Internet Settings -> Connection -> your AP -> the assigned IP.)" >&2
	exit 1
fi
echo "$IP" > "$IP_FILE"

echo "==> Building..."
make -j"$(nproc)"

TARGET="claudendo"
echo "==> Netloading $TARGET.3dsx to 3DS at $IP ..."
"$LINK" -a "$IP" "$TARGET.3dsx"
echo "==> Sent. It should be running on the 3DS now."
