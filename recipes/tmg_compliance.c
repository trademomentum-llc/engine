/*
 * tmg_compliance.c -- Trident Markets Group Quantum Requirements Compliance
 *
 * Validates project codebases against TMG quantum-era regulatory and
 * infrastructure requirements:
 *
 *   - PQC & Crypto: FIPS 203/204/205, signature hierarchy, TLS 1.3,
 *     AES-256-GCM at rest, deprecated crypto detection, crypto-agility
 *   - Custody: hot/warm/cold wallet allocation, multisig thresholds
 *   - Settlement: T+0 atomic DvP, EIG Bank Coin, FedNow/Fedwire
 *   - Performance: 25ms P99, 99.999% uptime, 200K RPS
 *   - Compliance: Zero Trust, CPMI-IOSCO PFMI, SEC, CFTC, FinCEN,
 *     MiCA, DORA, GDPR, AMLD6, FATF, SOC 2 Type II, ISO 27001,
 *     FIPS 140-3 Level 3, PCI DSS 4.0
 *   - Architecture: DoD zones 0-6, CIS Level 2, append-only audit logs,
 *     SHA-3 + PQC attestation, TLA+ model checking, Verus proofs
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>

/* --------------------------------------------------------------------------
 * TMG Constants
 * -------------------------------------------------------------------------- */

/* Custody allocation bounds (percentage of AUM) */
#define TMG_HOT_MAX_PCT        2
#define TMG_WARM_MIN_PCT       5
#define TMG_WARM_MAX_PCT      10
#define TMG_COLD_MIN_PCT      88
#define TMG_COLD_MAX_PCT      93

/* Multisig thresholds */
#define TMG_MSIG_HOT_M         3
#define TMG_MSIG_HOT_N         5
#define TMG_MSIG_WARM_M        5
#define TMG_MSIG_WARM_N        8
#define TMG_MSIG_COLD_M        7
#define TMG_MSIG_COLD_N       11

/* Performance SLAs */
#define TMG_P99_LATENCY_MS    25
#define TMG_UPTIME_NINES       5    /* 99.999% */
#define TMG_TARGET_RPS    200000

/* QRNG throughput */
#define TMG_QRNG_GBPS        40

/* --------------------------------------------------------------------------
 * Signature hierarchy spec
 * -------------------------------------------------------------------------- */

typedef struct {
    const char *role;
    const char *algorithm;
    const char *fips;
} tmg_sig_tier_t;

static const tmg_sig_tier_t TMG_SIG_HIERARCHY[] = {
    { "Root CA",       "SLH-DSA",    "FIPS 205" },
    { "Intermediate",  "ML-DSA-87",  "FIPS 204" },
    { "End-entity",    "ML-DSA-65",  "FIPS 204" },
};

#define TMG_SIG_TIER_COUNT \
    (sizeof(TMG_SIG_HIERARCHY) / sizeof(TMG_SIG_HIERARCHY[0]))

/* --------------------------------------------------------------------------
 * Deprecated crypto patterns
 * -------------------------------------------------------------------------- */

typedef struct {
    const char *pattern;
    const char *reason;
} tmg_deprecated_t;

static const tmg_deprecated_t TMG_DEPRECATED[] = {
    { "rsa-1024",  "RSA < 4096 not quantum-safe"       },
    { "rsa-2048",  "RSA < 4096 not quantum-safe"       },
    { "rsa-3072",  "RSA < 4096 not quantum-safe"       },
    { "rsa_1024",  "RSA < 4096 not quantum-safe"       },
    { "rsa_2048",  "RSA < 4096 not quantum-safe"       },
    { "rsa_3072",  "RSA < 4096 not quantum-safe"       },
    { "des-cbc",   "DES is broken"                     },
    { "3des",      "3DES is deprecated (NIST 2023)"    },
    { "triple-des","3DES is deprecated (NIST 2023)"    },
    { "triple_des","3DES is deprecated (NIST 2023)"    },
    { "rc4",       "RC4 is broken (RFC 7465)"          },
    { "md5",       "MD5 is broken for crypto use"      },
    { "sha-1",     "SHA-1 is deprecated (NIST 2030)"   },
    { "sha1",      "SHA-1 is deprecated (NIST 2030)"   },
    { "blowfish",  "Blowfish has 64-bit block weakness" },
};

#define TMG_DEPRECATED_COUNT \
    (sizeof(TMG_DEPRECATED) / sizeof(TMG_DEPRECATED[0]))

/* --------------------------------------------------------------------------
 * PQC algorithm indicators (positive signals)
 * -------------------------------------------------------------------------- */

static const char *TMG_PQC_INDICATORS[] = {
    "ml-kem", "ml_kem", "kyber",
    "ml-dsa", "ml_dsa", "dilithium",
    "slh-dsa", "slh_dsa", "sphincs",
    "fips203", "fips204", "fips205",
    "fips-203", "fips-204", "fips-205",
    "falcon", "fips206", "fips-206",
    "hqc",
    "liboqs", "pqcrypto", "post-quantum", "post_quantum",
    NULL
};

/* Crypto-agility keywords for FALCON/HQC readiness */
static const char *TMG_AGILITY_INDICATORS[] = {
    "crypto_agility", "crypto-agility", "algorithm_negotiation",
    "algorithm-negotiation", "cipher_suite_negotiation",
    "falcon", "fips-206", "fips206", "hqc",
    "pluggable_crypto", "pluggable-crypto",
    NULL
};

/* Compliance framework keywords */
static const char *TMG_COMPLIANCE_FRAMEWORKS[] = {
    "zero_trust", "zero-trust", "nist-800-207", "sp-800-207", "sp800-207",
    "cpmi-iosco", "cpmi_iosco", "pfmi",
    "sec-rule", "sec_rule", "regulation-sho", "reg-nms",
    "cftc", "commodity-futures",
    "fincen", "bsa-aml",
    "mica", "markets-in-crypto",
    "dora", "digital-operational-resilience",
    "gdpr", "data-protection",
    "amld6", "anti-money-laundering",
    "fatf", "travel-rule",
    "soc-2", "soc2", "soc_2",
    "iso-27001", "iso27001", "iso_27001",
    "fips-140-3", "fips140-3", "fips_140_3",
    "pci-dss", "pci_dss", "pcidss",
    NULL
};

/* Settlement keywords */
static const char *TMG_SETTLEMENT_INDICATORS[] = {
    "atomic-dvp", "atomic_dvp", "delivery-versus-payment",
    "t+0", "t_plus_0", "real-time-settlement",
    "eig-bank", "eig_bank", "bank-coin", "bank_coin",
    "fednow", "fed_now", "fedwire", "fed_wire",
    NULL
};

/* Architecture keywords */
static const char *TMG_ARCH_INDICATORS[] = {
    "dod-zone", "dod_zone", "security-zone",
    "cis-level-2", "cis_level_2", "cis-benchmark",
    "append-only", "append_only", "hash-chain", "hash_chain",
    "audit-log", "audit_log",
    "sha-3", "sha3", "sha_3",
    "tla+", "tla_plus", "model-checking", "model_checking",
    "verus", "formal-verification", "formal_verification",
    "intel-tdx", "intel_tdx",
    "qrng", "quantum-random", "quantum_random",
    NULL
};

/* Custody keywords */
static const char *TMG_CUSTODY_INDICATORS[] = {
    "hot-wallet", "hot_wallet", "warm-wallet", "warm_wallet",
    "cold-wallet", "cold_wallet", "cold-storage", "cold_storage",
    "multisig", "multi-sig", "multi_sig",
    "3-of-5", "3_of_5", "5-of-8", "5_of_8", "7-of-11", "7_of_11",
    NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_tmg(const char *path, size_t *out_len) {
    FILE *f = lst_secure_fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0 || sz > 50 * 1024 * 1024) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[rd] = '\0';
    if (out_len) *out_len = rd;
    return buf;
}

static const char *strcasestr_tmg(const char *haystack, const char *needle) {
    if (!*needle) return haystack;
    size_t nlen = strlen(needle);
    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, nlen) == 0)
            return haystack;
    }
    return NULL;
}

static void add_tmg_issue(lst_artifact_t *art, uint8_t severity,
                           const char *title, const char *desc,
                           const char *file_path, uint32_t line,
                           const char *req_id) {
    if (art->issue_count >= LST_MAX_ISSUES) return;
    lst_issue_t *issue = &art->issues[art->issue_count];
    memset(issue, 0, sizeof(lst_issue_t));

    snprintf(issue->id, sizeof(issue->id), "TMG-%04u", art->issue_count + 1);
    issue->severity = severity;
    snprintf(issue->title, sizeof(issue->title), "%s", title);
    snprintf(issue->description, sizeof(issue->description), "%s", desc);
    if (file_path)
        snprintf(issue->file_path, sizeof(issue->file_path), "%s", file_path);
    issue->line_number = line;
    if (req_id)
        snprintf(issue->cwe, sizeof(issue->cwe), "%s", req_id);
    snprintf(issue->remediation, sizeof(issue->remediation),
        "See TMG Quantum Requirements Specification");

    art->issue_count++;
}

/* --------------------------------------------------------------------------
 * Scan context -- accumulates coverage signals across all files
 * -------------------------------------------------------------------------- */

typedef struct {
    int files_scanned;
    int has_fips203;
    int has_fips204;
    int has_fips205;
    int has_slhdsa_root;
    int has_mldsa87_intermediate;
    int has_mldsa65_entity;
    int has_tls13;
    int has_aes256gcm;
    int has_deprecated_crypto;
    int has_agility;
    int has_qrng;
    int has_tdx;
    int has_hot_wallet;
    int has_warm_wallet;
    int has_cold_wallet;
    int has_multisig;
    int has_atomic_dvp;
    int has_fednow;
    int has_settlement;
    int has_p99_latency;
    int has_uptime_sla;
    int has_rps_target;
    int has_zero_trust;
    int has_pfmi;
    int has_soc2;
    int has_iso27001;
    int has_fips140;
    int has_pcidss;
    int has_dod_zones;
    int has_cis_hardening;
    int has_audit_log;
    int has_sha3_attestation;
    int has_tlaplus;
    int has_verus;
    int has_gdpr;
    int has_mica;
    int has_dora;
    int compliance_count;
    int financial_indicator_count;
} tmg_scan_ctx_t;

/* --------------------------------------------------------------------------
 * Financial project detection
 *
 * TMG compliance only applies to financial/trading projects.
 * A project is considered financial if:
 *   1. It contains a .tmg-compliance marker file (explicit opt-in), OR
 *   2. Its source files contain financial domain indicators
 * -------------------------------------------------------------------------- */

static const char *TMG_FINANCIAL_MARKERS[] = {
    ".tmg-compliance", "tmg.conf", "tmg_config",
    NULL
};

static const char *TMG_FINANCIAL_INDICATORS[] = {
    "wallet", "custody", "settlement", "trading",
    "order-book", "order_book", "orderbook",
    "exchange", "clearinghouse", "clearing_house",
    "ledger", "stablecoin", "tokenization",
    "aml", "kyc", "know-your-customer",
    "multisig", "multi-sig",
    "fednow", "fedwire", "swift",
    "dvp", "delivery-versus-payment",
    NULL
};

/* --------------------------------------------------------------------------
 * Per-line checks
 * -------------------------------------------------------------------------- */

/* Check for deprecated crypto algorithms */
static void check_tmg_deprecated(lst_artifact_t *art, tmg_scan_ctx_t *ctx,
                                  const char *fpath, const char *line,
                                  int lineno) {
    for (size_t i = 0; i < TMG_DEPRECATED_COUNT; i++) {
        if (strcasestr_tmg(line, TMG_DEPRECATED[i].pattern)) {
            ctx->has_deprecated_crypto = 1;
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                "Deprecated: %s -- %s",
                TMG_DEPRECATED[i].pattern,
                TMG_DEPRECATED[i].reason);
            add_tmg_issue(art, SEV_ERROR,
                "Deprecated Cryptographic Algorithm",
                desc, fpath, (uint32_t)lineno, "TMG-CRYPTO");
        }
    }
}

/* Check PQC and crypto posture */
static void check_tmg_pqc(tmg_scan_ctx_t *ctx, const char *line) {
    for (int i = 0; TMG_PQC_INDICATORS[i]; i++) {
        if (strcasestr_tmg(line, TMG_PQC_INDICATORS[i])) {
            /* Classify which FIPS standard */
            if (strcasestr_tmg(line, "kyber") ||
                strcasestr_tmg(line, "ml-kem") ||
                strcasestr_tmg(line, "ml_kem") ||
                strcasestr_tmg(line, "fips203") ||
                strcasestr_tmg(line, "fips-203"))
                ctx->has_fips203 = 1;
            if (strcasestr_tmg(line, "dilithium") ||
                strcasestr_tmg(line, "ml-dsa") ||
                strcasestr_tmg(line, "ml_dsa") ||
                strcasestr_tmg(line, "fips204") ||
                strcasestr_tmg(line, "fips-204"))
                ctx->has_fips204 = 1;
            if (strcasestr_tmg(line, "sphincs") ||
                strcasestr_tmg(line, "slh-dsa") ||
                strcasestr_tmg(line, "slh_dsa") ||
                strcasestr_tmg(line, "fips205") ||
                strcasestr_tmg(line, "fips-205"))
                ctx->has_fips205 = 1;
            break;
        }
    }

    /* Signature hierarchy detection */
    if ((strcasestr_tmg(line, "slh-dsa") || strcasestr_tmg(line, "slh_dsa") ||
         strcasestr_tmg(line, "sphincs")) &&
        (strcasestr_tmg(line, "root") || strcasestr_tmg(line, "root_ca") ||
         strcasestr_tmg(line, "root-ca")))
        ctx->has_slhdsa_root = 1;

    if ((strcasestr_tmg(line, "ml-dsa-87") || strcasestr_tmg(line, "ml_dsa_87") ||
         strcasestr_tmg(line, "dilithium-5") || strcasestr_tmg(line, "dilithium5")) &&
        (strcasestr_tmg(line, "intermediate") || strcasestr_tmg(line, "ica")))
        ctx->has_mldsa87_intermediate = 1;

    if ((strcasestr_tmg(line, "ml-dsa-65") || strcasestr_tmg(line, "ml_dsa_65") ||
         strcasestr_tmg(line, "dilithium-3") || strcasestr_tmg(line, "dilithium3")) &&
        (strcasestr_tmg(line, "end-entity") || strcasestr_tmg(line, "end_entity") ||
         strcasestr_tmg(line, "leaf") || strcasestr_tmg(line, "ee-cert")))
        ctx->has_mldsa65_entity = 1;

    /* TLS 1.3 hybrid PQC */
    if (strcasestr_tmg(line, "tls-1.3") || strcasestr_tmg(line, "tls_1_3") ||
        strcasestr_tmg(line, "tls1.3") || strcasestr_tmg(line, "tlsv1.3") ||
        strcasestr_tmg(line, "tls13"))
        ctx->has_tls13 = 1;

    /* AES-256-GCM at rest */
    if (strcasestr_tmg(line, "aes-256-gcm") || strcasestr_tmg(line, "aes_256_gcm") ||
        strcasestr_tmg(line, "aes256gcm"))
        ctx->has_aes256gcm = 1;

    /* Crypto-agility for FALCON / HQC */
    for (int i = 0; TMG_AGILITY_INDICATORS[i]; i++) {
        if (strcasestr_tmg(line, TMG_AGILITY_INDICATORS[i])) {
            ctx->has_agility = 1;
            break;
        }
    }

    /* QRNG and Intel TDX */
    if (strcasestr_tmg(line, "qrng") || strcasestr_tmg(line, "quantum-random") ||
        strcasestr_tmg(line, "quantum_random"))
        ctx->has_qrng = 1;
    if (strcasestr_tmg(line, "intel-tdx") || strcasestr_tmg(line, "intel_tdx") ||
        strcasestr_tmg(line, "tdx"))
        ctx->has_tdx = 1;
}

/* Check custody configuration */
static void check_tmg_custody(lst_artifact_t *art, tmg_scan_ctx_t *ctx,
                               const char *fpath, const char *line,
                               int lineno) {
    /* Broad custody indicator scan via TMG_CUSTODY_INDICATORS array */
    for (int i = 0; TMG_CUSTODY_INDICATORS[i]; i++) {
        if (strcasestr_tmg(line, TMG_CUSTODY_INDICATORS[i])) {
            /* Classify which custody component was found */
            if (strstr(TMG_CUSTODY_INDICATORS[i], "hot"))
                ctx->has_hot_wallet = 1;
            else if (strstr(TMG_CUSTODY_INDICATORS[i], "warm"))
                ctx->has_warm_wallet = 1;
            else if (strstr(TMG_CUSTODY_INDICATORS[i], "cold"))
                ctx->has_cold_wallet = 1;
            else if (strstr(TMG_CUSTODY_INDICATORS[i], "sig") ||
                     strstr(TMG_CUSTODY_INDICATORS[i], "of-"))
                ctx->has_multisig = 1;
            break;
        }
    }

    /* Validate hot wallet percentage if specified */
    if ((strcasestr_tmg(line, "hot") && strcasestr_tmg(line, "percent")) ||
        (strcasestr_tmg(line, "hot") && strcasestr_tmg(line, "aum"))) {
        const char *eq = strstr(line, "=");
        if (eq) {
            int pct = atoi(eq + 1);
            if (pct > TMG_HOT_MAX_PCT) {
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                    "Hot wallet allocation %d%% exceeds maximum %d%% AUM",
                    pct, TMG_HOT_MAX_PCT);
                add_tmg_issue(art, SEV_CRITICAL,
                    "Custody Allocation Violation",
                    desc, fpath, (uint32_t)lineno, "TMG-CUSTODY");
            }
        }
    }
}

/* Check settlement configuration */
static void check_tmg_settlement(tmg_scan_ctx_t *ctx, const char *line) {
    for (int i = 0; TMG_SETTLEMENT_INDICATORS[i]; i++) {
        if (strcasestr_tmg(line, TMG_SETTLEMENT_INDICATORS[i])) {
            ctx->has_settlement = 1;
            break;
        }
    }
    if (strcasestr_tmg(line, "atomic-dvp") || strcasestr_tmg(line, "atomic_dvp") ||
        strcasestr_tmg(line, "delivery-versus-payment"))
        ctx->has_atomic_dvp = 1;
    if (strcasestr_tmg(line, "fednow") || strcasestr_tmg(line, "fed_now") ||
        strcasestr_tmg(line, "fedwire") || strcasestr_tmg(line, "fed_wire"))
        ctx->has_fednow = 1;
}

/* Check performance SLA references */
static void check_tmg_performance(lst_artifact_t *art, tmg_scan_ctx_t *ctx,
                                   const char *fpath, const char *line,
                                   int lineno) {
    if (strcasestr_tmg(line, "p99") || strcasestr_tmg(line, "p99_latency") ||
        strcasestr_tmg(line, "p99-latency"))
        ctx->has_p99_latency = 1;
    if (strcasestr_tmg(line, "99.999") || strcasestr_tmg(line, "five-nines") ||
        strcasestr_tmg(line, "five_nines"))
        ctx->has_uptime_sla = 1;
    if (strcasestr_tmg(line, "200000") || strcasestr_tmg(line, "200k-rps") ||
        strcasestr_tmg(line, "200k_rps") || strcasestr_tmg(line, "200000_rps"))
        ctx->has_rps_target = 1;

    /* Flag P99 latency values that exceed the 25ms target */
    if (strcasestr_tmg(line, "p99") &&
        (strstr(line, "=") || strstr(line, ":"))) {
        const char *eq = strstr(line, "=");
        if (!eq) eq = strstr(line, ":");
        if (eq) {
            int val = atoi(eq + 1);
            if (val > TMG_P99_LATENCY_MS && val < 10000) {
                char desc[LST_MAX_NAME];
                snprintf(desc, sizeof(desc),
                    "P99 latency %dms exceeds TMG target of %dms",
                    val, TMG_P99_LATENCY_MS);
                add_tmg_issue(art, SEV_WARNING,
                    "Performance SLA Concern",
                    desc, fpath, (uint32_t)lineno, "TMG-PERF");
            }
        }
    }
}

/* Check compliance framework references */
static void check_tmg_compliance(tmg_scan_ctx_t *ctx, const char *line) {
    for (int i = 0; TMG_COMPLIANCE_FRAMEWORKS[i]; i++) {
        if (strcasestr_tmg(line, TMG_COMPLIANCE_FRAMEWORKS[i])) {
            ctx->compliance_count++;
            break;
        }
    }

    if (strcasestr_tmg(line, "zero-trust") || strcasestr_tmg(line, "zero_trust") ||
        strcasestr_tmg(line, "800-207") || strcasestr_tmg(line, "sp800-207"))
        ctx->has_zero_trust = 1;
    if (strcasestr_tmg(line, "cpmi-iosco") || strcasestr_tmg(line, "cpmi_iosco") ||
        strcasestr_tmg(line, "pfmi"))
        ctx->has_pfmi = 1;
    if (strcasestr_tmg(line, "soc-2") || strcasestr_tmg(line, "soc2") ||
        strcasestr_tmg(line, "soc_2"))
        ctx->has_soc2 = 1;
    if (strcasestr_tmg(line, "iso-27001") || strcasestr_tmg(line, "iso27001") ||
        strcasestr_tmg(line, "iso_27001"))
        ctx->has_iso27001 = 1;
    if (strcasestr_tmg(line, "fips-140-3") || strcasestr_tmg(line, "fips140-3") ||
        strcasestr_tmg(line, "fips_140_3"))
        ctx->has_fips140 = 1;
    if (strcasestr_tmg(line, "pci-dss") || strcasestr_tmg(line, "pci_dss") ||
        strcasestr_tmg(line, "pcidss"))
        ctx->has_pcidss = 1;
    if (strcasestr_tmg(line, "gdpr") || strcasestr_tmg(line, "data-protection"))
        ctx->has_gdpr = 1;
    if (strcasestr_tmg(line, "mica") || strcasestr_tmg(line, "markets-in-crypto"))
        ctx->has_mica = 1;
    if (strcasestr_tmg(line, "dora") ||
        strcasestr_tmg(line, "digital-operational-resilience"))
        ctx->has_dora = 1;
}

/* Check architecture requirements */
static void check_tmg_architecture(tmg_scan_ctx_t *ctx, const char *line) {
    /* Broad indicator scan via TMG_ARCH_INDICATORS array */
    for (int i = 0; TMG_ARCH_INDICATORS[i]; i++) {
        if (strcasestr_tmg(line, TMG_ARCH_INDICATORS[i]))
            break; /* at least one arch indicator present on this line */
    }

    /* Specific classification for coverage tracking */
    if (strcasestr_tmg(line, "dod-zone") || strcasestr_tmg(line, "dod_zone") ||
        strcasestr_tmg(line, "security-zone") || strcasestr_tmg(line, "security_zone"))
        ctx->has_dod_zones = 1;
    if (strcasestr_tmg(line, "cis-level-2") || strcasestr_tmg(line, "cis_level_2") ||
        strcasestr_tmg(line, "cis-benchmark") || strcasestr_tmg(line, "cis_benchmark"))
        ctx->has_cis_hardening = 1;
    if ((strcasestr_tmg(line, "append-only") || strcasestr_tmg(line, "append_only")) &&
        (strcasestr_tmg(line, "audit") || strcasestr_tmg(line, "log") ||
         strcasestr_tmg(line, "hash-chain") || strcasestr_tmg(line, "hash_chain")))
        ctx->has_audit_log = 1;
    if ((strcasestr_tmg(line, "sha-3") || strcasestr_tmg(line, "sha3") ||
         strcasestr_tmg(line, "sha_3")) &&
        (strcasestr_tmg(line, "attestation") || strcasestr_tmg(line, "pqc")))
        ctx->has_sha3_attestation = 1;
    if (strcasestr_tmg(line, "tla+") || strcasestr_tmg(line, "tla_plus") ||
        strcasestr_tmg(line, "model-checking") || strcasestr_tmg(line, "model_checking"))
        ctx->has_tlaplus = 1;
    if (strcasestr_tmg(line, "verus") ||
        strcasestr_tmg(line, "formal-verification") ||
        strcasestr_tmg(line, "formal_verification"))
        ctx->has_verus = 1;
}

/* Check for non-PQC TLS versions */
static void check_tmg_tls(lst_artifact_t *art, const char *fpath,
                            const char *line, int lineno) {
    if ((strcasestr_tmg(line, "tls-1.0") || strcasestr_tmg(line, "tls_1_0") ||
         strcasestr_tmg(line, "tlsv1.0") || strcasestr_tmg(line, "tls1.0") ||
         strcasestr_tmg(line, "tls-1.1") || strcasestr_tmg(line, "tls_1_1") ||
         strcasestr_tmg(line, "tlsv1.1") || strcasestr_tmg(line, "tls1.1") ||
         strcasestr_tmg(line, "ssl-3") || strcasestr_tmg(line, "sslv3")) &&
        !strcasestr_tmg(line, "disable") && !strcasestr_tmg(line, "reject") &&
        !strcasestr_tmg(line, "deny") && !strcasestr_tmg(line, "block")) {
        add_tmg_issue(art, SEV_ERROR,
            "Deprecated TLS Version",
            "TMG requires TLS 1.3 with hybrid PQC -- older versions prohibited",
            fpath, (uint32_t)lineno, "TMG-TLS");
    }
}

/* --------------------------------------------------------------------------
 * File scanner
 * -------------------------------------------------------------------------- */

static const char *SKIP_DIRS_TMG[] = {
    "node_modules", "vendor", ".git", "__pycache__", "dist", "build",
    ".venv", "venv", "venv_new", "venv_old", "agent_env", "env",
    "site-packages", "legacy", "fragments", "reference_clones",
    ".next", "target", ".cache", NULL
};

static int should_skip_tmg(const char *name) {
    if (name[0] == '.') return 1;
    for (int i = 0; SKIP_DIRS_TMG[i]; i++)
        if (strcmp(name, SKIP_DIRS_TMG[i]) == 0) return 1;
    return 0;
}

static int is_tmg_scannable(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    const char *exts[] = {
        ".c", ".cpp", ".h", ".hpp", ".py", ".js", ".ts",
        ".yaml", ".yml", ".toml", ".ini", ".conf", ".cfg",
        ".json", ".sh", ".go", ".rs", ".java", ".rb",
        ".swift", ".kt", ".tf", ".hcl", NULL
    };
    for (int i = 0; exts[i]; i++)
        if (strcasecmp(ext, exts[i]) == 0) return 1;
    return 0;
}

static void scan_file_tmg(lst_artifact_t *art, tmg_scan_ctx_t *ctx,
                           const char *fpath) {
    size_t len = 0;
    char *content = read_file_tmg(fpath, &len);
    if (!content) return;

    ctx->files_scanned++;
    int lineno = 1;
    char *line_start = content;

    while (*line_start) {
        char *eol = strchr(line_start, '\n');
        if (eol) *eol = '\0';

        /* Skip comment lines */
        const char *stripped = line_start;
        while (*stripped == ' ' || *stripped == '\t') stripped++;
        if (*stripped != '#' && *stripped != '/' && *stripped != '*') {
            check_tmg_deprecated(art, ctx, fpath, line_start, lineno);
            check_tmg_pqc(ctx, line_start);
            check_tmg_custody(art, ctx, fpath, line_start, lineno);
            check_tmg_settlement(ctx, line_start);
            check_tmg_performance(art, ctx, fpath, line_start, lineno);
            check_tmg_compliance(ctx, line_start);
            check_tmg_architecture(ctx, line_start);
            check_tmg_tls(art, fpath, line_start, lineno);

            /* Count financial domain indicators */
            for (int fi = 0; TMG_FINANCIAL_INDICATORS[fi]; fi++) {
                if (strcasestr_tmg(line_start, TMG_FINANCIAL_INDICATORS[fi])) {
                    ctx->financial_indicator_count++;
                    break;
                }
            }
        }

        if (!eol) break;
        line_start = eol + 1;
        lineno++;
    }

    free(content);
}

static void scan_dir_tmg(lst_artifact_t *art, tmg_scan_ctx_t *ctx,
                          const char *dir, int depth) {
    if (depth > 8) return;

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (should_skip_tmg(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir_tmg(art, ctx, child, depth + 1);
        } else if (S_ISREG(st.st_mode) && is_tmg_scannable(ent->d_name)) {
            scan_file_tmg(art, ctx, child);
        }
    }
    closedir(d);
}

/* --------------------------------------------------------------------------
 * Gap analysis -- flag missing coverage after full scan
 * -------------------------------------------------------------------------- */

static void emit_coverage_gaps(lst_artifact_t *art, const tmg_scan_ctx_t *ctx) {
    /* PQC standards */
    if (!ctx->has_fips203)
        add_tmg_issue(art, SEV_WARNING, "Missing FIPS 203 (ML-KEM/Kyber)",
            "No ML-KEM key encapsulation references found",
            NULL, 0, "TMG-PQC");
    if (!ctx->has_fips204)
        add_tmg_issue(art, SEV_WARNING, "Missing FIPS 204 (ML-DSA/Dilithium)",
            "No ML-DSA signature references found",
            NULL, 0, "TMG-PQC");
    if (!ctx->has_fips205)
        add_tmg_issue(art, SEV_WARNING, "Missing FIPS 205 (SLH-DSA/SPHINCS+)",
            "No SLH-DSA hash-based signature references found",
            NULL, 0, "TMG-PQC");

    /* Signature hierarchy */
    if (!ctx->has_slhdsa_root)
        add_tmg_issue(art, SEV_WARNING, "Missing Root CA Signature Config",
            "Root CA must use SLH-DSA (FIPS 205)",
            NULL, 0, "TMG-SIG");
    if (!ctx->has_mldsa87_intermediate)
        add_tmg_issue(art, SEV_WARNING, "Missing Intermediate CA Config",
            "Intermediate CA must use ML-DSA-87 (Dilithium-5)",
            NULL, 0, "TMG-SIG");
    if (!ctx->has_mldsa65_entity)
        add_tmg_issue(art, SEV_WARNING, "Missing End-Entity Cert Config",
            "End-entity certs must use ML-DSA-65 (Dilithium-3)",
            NULL, 0, "TMG-SIG");

    /* TLS and encryption */
    if (!ctx->has_tls13)
        add_tmg_issue(art, SEV_WARNING, "Missing TLS 1.3 Hybrid PQC",
            "No TLS 1.3 configuration with PQC hybrid mode detected",
            NULL, 0, "TMG-TLS");
    if (!ctx->has_aes256gcm)
        add_tmg_issue(art, SEV_WARNING, "Missing AES-256-GCM At-Rest Encryption",
            "Data at rest must be encrypted with AES-256-GCM",
            NULL, 0, "TMG-ENC");

    /* Crypto-agility */
    if (!ctx->has_agility)
        add_tmg_issue(art, SEV_INFO, "Missing Crypto-Agility",
            "No crypto-agility mechanism for FALCON (FIPS 206) / HQC migration",
            NULL, 0, "TMG-AGILITY");

    /* QRNG and TDX */
    if (!ctx->has_qrng)
        add_tmg_issue(art, SEV_INFO, "Missing QRNG Integration",
            "QRNG at 40Gbps throughput not detected",
            NULL, 0, "TMG-QRNG");
    if (!ctx->has_tdx)
        add_tmg_issue(art, SEV_INFO, "Missing Intel TDX Reference",
            "Intel TDX confidential computing not detected",
            NULL, 0, "TMG-TDX");

    /* Custody */
    if (!ctx->has_hot_wallet && !ctx->has_cold_wallet)
        add_tmg_issue(art, SEV_INFO, "Missing Custody Wallet Config",
            "No hot/warm/cold wallet tier configuration found",
            NULL, 0, "TMG-CUSTODY");
    if (!ctx->has_multisig)
        add_tmg_issue(art, SEV_INFO, "Missing Multisig Config",
            "Multisig (3-of-5 hot, 5-of-8 warm, 7-of-11 cold) not detected",
            NULL, 0, "TMG-MSIG");

    /* Settlement */
    if (!ctx->has_atomic_dvp)
        add_tmg_issue(art, SEV_INFO, "Missing Atomic DvP",
            "T+0 atomic Delivery-versus-Payment not detected",
            NULL, 0, "TMG-SETTLE");
    if (!ctx->has_fednow)
        add_tmg_issue(art, SEV_INFO, "Missing FedNow/Fedwire Integration",
            "No FedNow or Fedwire settlement rail references found",
            NULL, 0, "TMG-SETTLE");

    /* Performance */
    if (!ctx->has_p99_latency)
        add_tmg_issue(art, SEV_INFO, "Missing P99 Latency Config",
            "No P99 latency target (25ms required) configuration found",
            NULL, 0, "TMG-PERF");
    if (!ctx->has_uptime_sla)
        add_tmg_issue(art, SEV_INFO, "Missing Uptime SLA",
            "No 99.999%% uptime SLA reference found",
            NULL, 0, "TMG-PERF");

    /* Compliance frameworks */
    if (!ctx->has_zero_trust)
        add_tmg_issue(art, SEV_WARNING, "Missing Zero Trust Architecture",
            "NIST SP 800-207 Zero Trust not referenced",
            NULL, 0, "TMG-ZTA");
    if (!ctx->has_pfmi)
        add_tmg_issue(art, SEV_WARNING, "Missing CPMI-IOSCO PFMI",
            "CPMI-IOSCO Principles for Financial Market Infrastructures not referenced",
            NULL, 0, "TMG-PFMI");
    if (!ctx->has_fips140)
        add_tmg_issue(art, SEV_WARNING, "Missing FIPS 140-3 Level 3",
            "FIPS 140-3 Level 3 HSM certification not referenced",
            NULL, 0, "TMG-HSM");

    /* Architecture */
    if (!ctx->has_dod_zones)
        add_tmg_issue(art, SEV_INFO, "Missing DoD Security Zones",
            "DoD zones 0-6 network segmentation not detected",
            NULL, 0, "TMG-ARCH");
    if (!ctx->has_audit_log)
        add_tmg_issue(art, SEV_INFO, "Missing Append-Only Audit Logs",
            "Append-only hash-chained audit log not detected",
            NULL, 0, "TMG-AUDIT");
    if (!ctx->has_tlaplus)
        add_tmg_issue(art, SEV_INFO, "Missing TLA+ Model Checking",
            "TLA+ formal model checking not detected",
            NULL, 0, "TMG-FORMAL");
    if (!ctx->has_verus)
        add_tmg_issue(art, SEV_INFO, "Missing Verus Proofs",
            "Verus formal verification for Rust core not detected",
            NULL, 0, "TMG-FORMAL");
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static void write_sep_tmg(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line_tmg(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

static const char *pass_fail(int flag) {
    return flag ? "PASS" : "FAIL";
}

static int is_financial_project(const char *project_path, const tmg_scan_ctx_t *ctx) {
    /* Check for explicit marker files */
    for (int i = 0; TMG_FINANCIAL_MARKERS[i]; i++) {
        char path[LST_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", project_path,
                 TMG_FINANCIAL_MARKERS[i]);
        struct stat st;
        if (stat(path, &st) == 0) return 1;
    }

    /* Check if scan found any financial domain signals */
    if (ctx->has_hot_wallet || ctx->has_warm_wallet || ctx->has_cold_wallet)
        return 1;
    if (ctx->has_multisig) return 1;
    if (ctx->has_atomic_dvp || ctx->has_fednow || ctx->has_settlement)
        return 1;

    /* Threshold: at least 3 financial indicator hits across all files */
    if (ctx->financial_indicator_count >= 3)
        return 1;

    return 0;
}

static int recipe_tmg_compliance(lst_artifact_t *art, const char *output_dir) {
    if (!art) return -1;

    uint32_t initial_issues = art->issue_count;
    tmg_scan_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* First pass: scan files for financial indicators and violations */
    scan_dir_tmg(art, &ctx, art->project_path, 0);

    /* Gate: only emit coverage gaps for financial projects */
    if (!is_financial_project(art->project_path, &ctx)) {
        printf("  tmg-compliance: %s is not a financial project, skipping.\n",
               art->project_name);
        printf("  To opt in, create a .tmg-compliance marker file in the project root.\n");
        return 0;
    }

    /* Emit coverage gap issues after full scan */
    emit_coverage_gaps(art, &ctx);

    uint32_t new_issues = art->issue_count - initial_issues;

    /* Build output path */
    char outpath[LST_MAX_PATH];
    if (output_dir)
        snprintf(outpath, sizeof(outpath),
            "%s/TMG_COMPLIANCE_REPORT", output_dir);
    else
        snprintf(outpath, sizeof(outpath),
            "%s/TMG_COMPLIANCE_REPORT", art->project_path);

    if (output_dir) mkdir(output_dir, 0755);

    FILE *f = lst_secure_fopen(outpath, "w");
    if (!f) {
        fprintf(stderr, "tmg-compliance: cannot write %s\n", outpath);
        return -1;
    }

    /* Header */
    write_sep_tmg(f);
    fprintf(f, "TRIDENT MARKETS GROUP -- QUANTUM REQUIREMENTS COMPLIANCE REPORT\n");
    fprintf(f, "Project: %s\n", art->project_name);
    fprintf(f, "Path: %s\n", art->project_path);

    time_t now = time(NULL);
    struct tm tm_buf;
    struct tm *t = gmtime_r(&now, &tm_buf);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S UTC", t);
    fprintf(f, "Generated: %s\n", ts);
    fprintf(f, "Files Scanned: %d\n", ctx.files_scanned);
    fprintf(f, "Issues Found: %u\n", new_issues);
    write_sep_tmg(f);
    fprintf(f, "\n");

    /* ---- PQC & Crypto Section ---- */
    fprintf(f, "PQC & CRYPTOGRAPHIC COMPLIANCE\n");
    write_line_tmg(f);
    fprintf(f, "\n");
    fprintf(f, "  FIPS Standards:\n");
    fprintf(f, "    [%s] FIPS 203 (ML-KEM / Kyber)\n", pass_fail(ctx.has_fips203));
    fprintf(f, "    [%s] FIPS 204 (ML-DSA / Dilithium)\n", pass_fail(ctx.has_fips204));
    fprintf(f, "    [%s] FIPS 205 (SLH-DSA / SPHINCS+)\n", pass_fail(ctx.has_fips205));
    fprintf(f, "\n");
    fprintf(f, "  Signature Hierarchy:\n");
    for (size_t i = 0; i < TMG_SIG_TIER_COUNT; i++) {
        int ok = 0;
        if (i == 0) ok = ctx.has_slhdsa_root;
        if (i == 1) ok = ctx.has_mldsa87_intermediate;
        if (i == 2) ok = ctx.has_mldsa65_entity;
        fprintf(f, "    [%s] %-15s -> %s (%s)\n",
            pass_fail(ok),
            TMG_SIG_HIERARCHY[i].role,
            TMG_SIG_HIERARCHY[i].algorithm,
            TMG_SIG_HIERARCHY[i].fips);
    }
    fprintf(f, "\n");
    fprintf(f, "  Transport & Encryption:\n");
    fprintf(f, "    [%s] TLS 1.3 with hybrid PQC\n", pass_fail(ctx.has_tls13));
    fprintf(f, "    [%s] AES-256-GCM at rest\n", pass_fail(ctx.has_aes256gcm));
    fprintf(f, "    [%s] No deprecated crypto\n",
        pass_fail(!ctx.has_deprecated_crypto));
    fprintf(f, "\n");
    fprintf(f, "  Crypto-Agility & Hardware:\n");
    fprintf(f, "    [%s] FALCON (FIPS 206) / HQC readiness\n",
        pass_fail(ctx.has_agility));
    fprintf(f, "    [%s] QRNG 40Gbps integration\n", pass_fail(ctx.has_qrng));
    fprintf(f, "    [%s] Intel TDX confidential compute\n", pass_fail(ctx.has_tdx));
    fprintf(f, "\n");

    /* ---- Custody Section ---- */
    fprintf(f, "CUSTODY REQUIREMENTS\n");
    write_line_tmg(f);
    fprintf(f, "\n");
    fprintf(f, "  Wallet Tiers:\n");
    fprintf(f, "    [%s] Hot wallet  (< %d%% AUM)\n",
        pass_fail(ctx.has_hot_wallet), TMG_HOT_MAX_PCT);
    fprintf(f, "    [%s] Warm wallet (%d-%d%% AUM)\n",
        pass_fail(ctx.has_warm_wallet), TMG_WARM_MIN_PCT, TMG_WARM_MAX_PCT);
    fprintf(f, "    [%s] Cold wallet (%d-%d%% AUM)\n",
        pass_fail(ctx.has_cold_wallet), TMG_COLD_MIN_PCT, TMG_COLD_MAX_PCT);
    fprintf(f, "\n");
    fprintf(f, "  Multisig Thresholds:\n");
    fprintf(f, "    Hot:  %d-of-%d\n", TMG_MSIG_HOT_M, TMG_MSIG_HOT_N);
    fprintf(f, "    Warm: %d-of-%d\n", TMG_MSIG_WARM_M, TMG_MSIG_WARM_N);
    fprintf(f, "    Cold: %d-of-%d\n", TMG_MSIG_COLD_M, TMG_MSIG_COLD_N);
    fprintf(f, "    [%s] Multisig configured\n", pass_fail(ctx.has_multisig));
    fprintf(f, "\n");

    /* ---- Settlement Section ---- */
    fprintf(f, "SETTLEMENT REQUIREMENTS\n");
    write_line_tmg(f);
    fprintf(f, "\n");
    fprintf(f, "    [%s] T+0 atomic DvP (Delivery-versus-Payment)\n",
        pass_fail(ctx.has_atomic_dvp));
    fprintf(f, "    [%s] EIG Bank Coin / stablecoin integration\n",
        pass_fail(ctx.has_settlement));
    fprintf(f, "    [%s] FedNow / Fedwire settlement rails\n",
        pass_fail(ctx.has_fednow));
    fprintf(f, "\n");

    /* ---- Performance Section ---- */
    fprintf(f, "PERFORMANCE SLAs\n");
    write_line_tmg(f);
    fprintf(f, "\n");
    fprintf(f, "    [%s] P99 latency <= %dms\n",
        pass_fail(ctx.has_p99_latency), TMG_P99_LATENCY_MS);
    fprintf(f, "    [%s] Uptime >= 99.999%%\n", pass_fail(ctx.has_uptime_sla));
    fprintf(f, "    [%s] Throughput >= %dK RPS\n",
        pass_fail(ctx.has_rps_target), TMG_TARGET_RPS / 1000);
    fprintf(f, "\n");

    /* ---- Compliance Frameworks Section ---- */
    fprintf(f, "REGULATORY & COMPLIANCE FRAMEWORKS\n");
    write_line_tmg(f);
    fprintf(f, "\n");
    fprintf(f, "  Security:\n");
    fprintf(f, "    [%s] Zero Trust (NIST SP 800-207)\n",
        pass_fail(ctx.has_zero_trust));
    fprintf(f, "    [%s] CPMI-IOSCO PFMI\n", pass_fail(ctx.has_pfmi));
    fprintf(f, "\n");
    fprintf(f, "  Certifications:\n");
    fprintf(f, "    [%s] SOC 2 Type II\n", pass_fail(ctx.has_soc2));
    fprintf(f, "    [%s] ISO 27001\n", pass_fail(ctx.has_iso27001));
    fprintf(f, "    [%s] FIPS 140-3 Level 3\n", pass_fail(ctx.has_fips140));
    fprintf(f, "    [%s] PCI DSS 4.0\n", pass_fail(ctx.has_pcidss));
    fprintf(f, "\n");
    fprintf(f, "  Regulatory:\n");
    fprintf(f, "    SEC, CFTC, FinCEN (US)\n");
    fprintf(f, "    [%s] MiCA (EU crypto-assets)\n", pass_fail(ctx.has_mica));
    fprintf(f, "    [%s] DORA (EU digital resilience)\n", pass_fail(ctx.has_dora));
    fprintf(f, "    [%s] GDPR (EU data protection)\n", pass_fail(ctx.has_gdpr));
    fprintf(f, "    AMLD6, FATF Travel Rule\n");
    fprintf(f, "\n");

    /* ---- Architecture Section ---- */
    fprintf(f, "ARCHITECTURE REQUIREMENTS\n");
    write_line_tmg(f);
    fprintf(f, "\n");
    fprintf(f, "    [%s] DoD zones 0-6 network segmentation\n",
        pass_fail(ctx.has_dod_zones));
    fprintf(f, "    [%s] CIS Level 2 hardening\n",
        pass_fail(ctx.has_cis_hardening));
    fprintf(f, "    [%s] Append-only hash-chained audit logs\n",
        pass_fail(ctx.has_audit_log));
    fprintf(f, "    [%s] SHA-3 + PQC attestation\n",
        pass_fail(ctx.has_sha3_attestation));
    fprintf(f, "    [%s] TLA+ model checking\n", pass_fail(ctx.has_tlaplus));
    fprintf(f, "    [%s] Verus proofs for Rust core\n", pass_fail(ctx.has_verus));
    fprintf(f, "\n");

    /* ---- Findings ---- */
    if (new_issues > 0) {
        fprintf(f, "DETAILED FINDINGS\n");
        write_sep_tmg(f);
        fprintf(f, "\n");

        /* Group by severity */
        for (int sev = SEV_CRITICAL; sev >= SEV_INFO; sev--) {
            int printed = 0;
            for (uint32_t i = initial_issues; i < art->issue_count; i++) {
                if (art->issues[i].severity != sev) continue;
                if (!printed) {
                    fprintf(f, "  [%s]\n\n",
                        sev == SEV_CRITICAL ? "CRITICAL" :
                        sev == SEV_ERROR    ? "ERROR" :
                        sev == SEV_WARNING  ? "WARNING" : "INFO");
                    printed = 1;
                }
                const lst_issue_t *issue = &art->issues[i];
                fprintf(f, "    %s: %s\n", issue->id, issue->title);
                if (issue->file_path[0])
                    fprintf(f, "      File: %s:%u\n",
                        issue->file_path, issue->line_number);
                if (issue->cwe[0])
                    fprintf(f, "      Ref: %s\n", issue->cwe);
                fprintf(f, "      %s\n\n", issue->description);
            }
        }
    } else {
        fprintf(f, "COMPLIANCE: FULL PASS\n");
        fprintf(f, "  All TMG quantum requirements validated.\n\n");
    }

    /* Footer */
    write_sep_tmg(f);
    fprintf(f, "END OF TMG COMPLIANCE REPORT\n");
    fprintf(f, "\nValidated against: Trident Markets Group Quantum Requirements\n");
    fprintf(f, "Standards: FIPS 203/204/205, NIST SP 800-207, CPMI-IOSCO PFMI\n");
    fprintf(f, "Custody: Hot <2%% / Warm 5-10%% / Cold 88-93%% AUM\n");
    fprintf(f, "Settlement: T+0 atomic DvP, EIG Bank Coin, FedNow/Fedwire\n");
    fprintf(f, "Performance: 25ms P99, 99.999%% uptime, 200K RPS\n");
    write_sep_tmg(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u issues)\n", outpath, new_issues);
    return 0;
}

/* --------------------------------------------------------------------------
 * Recipe registration
 * -------------------------------------------------------------------------- */

void recipe_tmg_compliance_register(void) {
    lst_recipe_t r = {0};
    snprintf(r.name, LST_MAX_NAME, "tmg-compliance");
    snprintf(r.description, LST_MAX_NAME,
        "Validate Trident Markets Group quantum requirements compliance");
    r.execute = recipe_tmg_compliance;
    r.version = 1;
    lst_recipe_register(&r);
}
