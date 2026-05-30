I am unable to generate direct file downloads. However, I have formatted the complete specification below within a code block. You can copy the content and save it as a markdown file.

**Instructions:**
1. Copy the text inside the code block below.
2. Paste it into a text editor.
3. Save the file as `LSA-SPEC-002-AMD-001.md`.

```markdown
# NNOS Gap Remediation Specification

**Document ID:** LSA-SPEC-002-AMD-001  
**Version:** 2.1.0 (Remediation)  
**Date:** 2026-03-20  
**Status:** APPROVED FOR IMPLEMENTATION  
**Classification:** CONFIDENTIAL  
**Parent Document:** LSA-SPEC-002 Version 2.0.0  

## 1. Purpose

This document defines mandatory remediation requirements to address security, traceability, and operational gaps identified during the Convergence Assessment of 2026-03-20. This specification supersedes conflicting instructions in `nnos-install-scripts.docx` and `68-Atomic-Primitives.md` where security or operational integrity is compromised.

Implementation of these requirements is critical for the protection of behavioral health data and system stability.

## 2. Security Remediation Requirements

### 2.1 Database Credential Management (Critical)
**Gap:** Previous implementation examples utilized blank passwords or hardcoded credentials.  
**Requirement:**  
2.1.1. All database connections SHALL authenticate using credentials stored in environment variables or a secure vault.  
2.1.2. Hardcoded passwords in source code are strictly prohibited.  
2.1.3. Applications SHALL fail securely (exit code 1) if credentials are missing.  

**Compliant Java Implementation:**
```java
package app.ihep.knockout.security;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;

public class SecureDBConnection {
    public static Connection getConnection() throws SQLException {
        String dbUrl = System.getenv("NNOS_DB_URL");
        String dbUser = System.getenv("NNOS_DB_USER");
        String dbPass = System.getenv("NNOS_DB_PASSWORD");

        if (dbUrl == null || dbUser == null || dbPass == null) {
            throw new SecurityException("Critical environment variables missing. Aborting connection.");
        }

        if (dbPass.isEmpty()) {
            throw new SecurityException("Database password cannot be empty.");
        }

        return DriverManager.getConnection(dbUrl, dbUser, dbPass);
    }
}
```

### 2.2 Encryption Key Permissions
**Gap:** Install scripts did not enforce strict permissions on synchronization keys.  
**Requirement:**  
2.2.1. The synchronization key (`/etc/lsa/sync.key`) SHALL be generated during installation.  
2.2.2. File permissions SHALL be set to `0600` (Owner Read/Write only).  
2.2.3. Ownership SHALL be set to the `lsa` service user.  

**Compliant Bash Implementation:**
```bash
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
```

## 3. Primitive Traceability Requirements

### 3.1 System-to-Atomic Mapping
**Gap:** No explicit link between System Primitives (J-Series) and Atomic Primitives (P/C/G/D-Series).  
**Requirement:**  
3.1.1. The `Jason_LSA_3Plane_STPU.xlsx` document SHALL be updated to include a column `Atomic_Primitive_Dependency`.  
3.1.2. Each J-Series Primitive SHALL map to at least one Atomic Primitive required for execution.  
3.1.3. Mapping SHALL be validated during the build process via a JSON manifest.  

**Example Mapping Manifest (`primitive_map.json`):**
```json
{
  "J-014_PROFILE_REFINEMENT": {
    "node": "M1",
    "atomic_dependencies": ["C052_RANDOM_FOREST", "P040_AGG_MEAN", "P042_AGG_SD"]
  },
  "J-019_DRIFT_DETECTION": {
    "node": "NUC",
    "atomic_dependencies": ["C061_TS_AUTOCORRELATION", "P047_AGG_QUANTILE"]
  }
}
```

## 4. Daemon Supervision Requirements

### 4.1 Systemd Unit Configuration (NUC/Orin)
**Gap:** Install scripts lacked service management files for automatic restart.  
**Requirement:**  
4.1.1. All daemons SHALL be managed by `systemd` on NUC and Orin nodes.  
4.1.2. Restart policy SHALL be `always` with a delay of 2 seconds (FR-6.11.3).  
4.1.3. Logs SHALL be captured by `journald`.  

**Compliant Systemd Unit File (`/etc/systemd/system/lsa-state-monitor.service`):**
```ini
[Unit]
Description=LSA State Monitor Daemon
After=network.target

[Service]
Type=simple
User=lsa
Group=lsa
ExecStart=/opt/lsa/bin/lsa_state_monitor
Restart=always
RestartSec=2s
LimitNOFILE=65535
Environment="NNOS_NODE_ROLE=EPN"
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

### 4.2 Launchd Configuration (M1)
**Requirement:**  
4.2.1. All daemons SHALL be managed by `launchd` on M1 nodes.  
4.2.2. `KeepAlive` policy SHALL be set to `true`.  

**Compliant Launchd Plist (`~/Library/LaunchAgents/app.ihep.knockout.convergence.plist`):**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>app.ihep.knockout.convergence</string>
    <key>ProgramArguments</key>
    <array>
        <string>/opt/lsa/bin/lsa_convergence_bond</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
    <key>StandardOutPath</key>
    <string>/var/log/lsa/convergence.log</string>
    <key>StandardErrorPath</key>
    <string>/var/log/lsa/convergence.err</string>
</dict>
</plist>
```

## 5. Vector Database Formalization

### 5.1 Specification Update
**Gap:** `LSA-Spec.pdf` Section 6.1 did not formally include Vector Database requirements.  
**Requirement:**  
5.1.1. LSA-SPEC-002 Section 6.1 (Origin Vault) SHALL be amended to include `pgvector` and `pgvectorscale`.  
5.1.2. Vector dimensions SHALL be fixed at 384 (MiniLM-L6-v2).  
5.1.3. Indexing SHALL use HNSW for cosine similarity.  

### 5.2 Schema Enforcement
**Requirement:**  
5.2.1. The `nnos_schema.sql` SHALL be version-controlled and applied via migration scripts.  
5.2.2. Direct schema modification by daemons is prohibited.  

## 6. Credential and Secret Distribution

### 6.1 Secret Localization
**Requirement:**  
6.1.1. Exchange API secrets and private keys SHALL reside ONLY on the NUC (DCN).  
6.1.2. Orin and M1 nodes SHALL NOT possess credentials capable of executing trades or accessing external financial APIs.  
6.1.3. Inter-node communication regarding secrets SHALL use ephemeral tokens valid for less than 5 minutes.  

### 6.2 Environment Variable Injection
**Requirement:**  
6.2.1. Secrets SHALL be injected into daemon environments via `systemd` `EnvironmentFile` or `launchd` `EnvironmentVariables`.  
6.2.2. Secrets SHALL NOT be passed as command-line arguments.  

**Compliant Systemd Environment File (`/etc/lsa/lsa.env`):**
```bash
NNOS_DB_URL="jdbc:postgresql://localhost:5432/nnos"
NNOS_DB_USER="lsa_service"
# Password loaded via secure vault mechanism or restricted file read
NNOS_DB_PASSWORD_FILE="/etc/lsa/secrets/db_pass"
```

## 7. Verification and Acceptance

### 7.1 Security Audit
7.1.1. A static code analysis scan SHALL be run prior to deployment.  
7.1.2. Any finding related to hardcoded secrets SHALL block deployment.  
7.1.3. File permission audits SHALL be run weekly via cron.  

### 7.2 Traceability Audit
7.2.1. The Primitive Mapping Manifest SHALL be validated during the Maven build phase.  
7.2.2. Build SHALL fail if any J-Series primitive lacks atomic dependencies.  

### 7.3 Supervision Audit
7.3.1. Daemon crash tests SHALL be performed monthly.  
7.3.2. Recovery time SHALL be measured and must be < 2.5 seconds.  

## 8. Implementation Timeline

| Phase | Task | Owner | Deadline |
| :--- | :--- | :--- | :--- |
| 1 | Update Install Scripts with Security Fixes | DevOps | 2026-03-25 |
| 2 | Generate Primitive Mapping Manifest | Architecture | 2026-03-27 |
| 3 | Deploy Systemd/Launchd Units | DevOps | 2026-03-30 |
| 4 | Formalize Vector DB Spec in LSA-SPEC-002 | Architecture | 2026-04-01 |
| 5 | Security Audit & Penetration Test | Security | 2026-04-05 |

## 9. Approval

**Approved By:** System Architecture Board  
**Date:** 2026-03-20  
**Signature:** [Digital Signature Required]  

---
*End of Document*
```