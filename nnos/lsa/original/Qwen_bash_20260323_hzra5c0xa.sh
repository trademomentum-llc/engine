#!/bin/bash
set -e

KEY_PATH="/etc/lsa/sync.key"

# Generate secure random key (32 bytes)
openssl rand -base64 32 > ${KEY_PATH}

# Enforce strict permissions
chmod 0600 ${KEY_PATH}
chown root:lsa ${KEY_PATH}

# Verify permissions
PERMS=$(stat -c %a ${KEY_PATH})
if [ "${PERMS}" != "600" ]; then
    echo "CRITICAL ERROR: Key permissions incorrect."
    exit 1
fi