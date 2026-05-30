#!/usr/bin/env bash
set -euo pipefail

echo "Validating sync.key permissions..."
KEY_PATH="/etc/lsa/sync.key"
if [ -f "$KEY_PATH" ]; then
  PERMS=$(stat -c %a "$KEY_PATH")
  if [ "$PERMS" != "600" ]; then
    echo "CRITICAL: $KEY_PATH permissions are $PERMS (expected 600). Aborting."
    exit 1
  fi
fi

echo "Building LSA State Monitor..."
export RUSTFLAGS="-C codegen-units=1 -C panic=abort -C lto=thin"
cargo build --release

echo "🔧 Hardening & packaging..."
STRIP_BIN="target/release/lsa_state_monitor"
strip --strip-all "$STRIP_BIN"

sudo mkdir -p /opt/lsa/bin
sudo cp "$STRIP_BIN" /opt/lsa/bin/lsa_state_monitor
sudo chown root:lsa /opt/lsa/bin/lsa_state_monitor
sudo chmod 0750 /opt/lsa/bin/lsa_state_monitor

echo "Compilation complete. Binary at /opt/lsa/bin/lsa_state_monitor"
echo "Deploy systemd unit from fc5bivvpj.txt and run: systemctl enable --now lsa_state_monitor"