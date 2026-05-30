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