/*
 * lst.h — Lossless Semantic Tree
 *
 * Core data structures for representing project artifacts.
 * An LST captures the complete semantic state of a project:
 * source files, dependencies, license data, security posture,
 * configuration — everything needed to run recipes without
 * re-parsing the original project.
 *
 * Design principles:
 *   - Fixed-layout structs, no heap-allocated wrappers
 *   - Primitive types throughout (no boxing)
 *   - Flat arrays with counts, not linked lists
 *   - Serializable to disk as raw bytes
 *   - Extensible via node_type enum (add new types without
 *     changing existing structures)
 */

#ifndef ENGINE_LST_H
#define ENGINE_LST_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <strings.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Limits — compile-time constants, tune per deployment
 * -------------------------------------------------------------------------- */

#define LST_MAX_NAME        256
#define LST_MAX_PATH        1024
#define LST_MAX_LICENSE      64
#define LST_MAX_AUTHORS      16
#define LST_MAX_AUTHOR_NAME 128
#define LST_MAX_DEPS       4096
#define LST_MAX_FILES      8192
#define LST_MAX_NODES     16384
#define LST_MAX_RECIPES     256
#define LST_MAX_ISSUES     1024
#define LST_MAX_AGENTS       64
#define LST_MAX_TRACES     2048
#define LST_MAX_GUARDRAILS  128

FILE *lst_secure_fopen(const char *path, const char *mode);

/* --------------------------------------------------------------------------
 * Enumerations — all stored as uint8_t, no string comparisons at runtime
 * -------------------------------------------------------------------------- */

typedef enum {
    PKG_UNKNOWN   = 0,
    PKG_COMPOSER  = 1,   /* PHP     */
    PKG_NPM       = 2,   /* Node.js */
    PKG_PIP       = 3,   /* Python  */
    PKG_CARGO     = 4,   /* Rust    */
    PKG_GO        = 5,   /* Go      */
    PKG_RUBY      = 6,   /* Ruby    */
    PKG_MAVEN     = 7,   /* Java    */
    PKG_GRADLE    = 8,   /* Java    */
    PKG_NUGET     = 9,   /* .NET    */
    PKG_SWIFT     = 10,  /* Swift   */
    PKG_COCOAPODS = 11,  /* iOS     */
    PKG_COUNT            /* sentinel — always last */
} pkg_manager_t;

typedef enum {
    LIC_UNKNOWN       = 0,
    LIC_MIT           = 1,
    LIC_BSD_2         = 2,
    LIC_BSD_3         = 3,
    LIC_APACHE_2      = 4,
    LIC_GPL_2         = 5,
    LIC_GPL_3         = 6,
    LIC_LGPL_2_1      = 7,
    LIC_LGPL_3        = 8,
    LIC_ISC           = 9,
    LIC_MPL_2         = 10,
    LIC_UNLICENSE     = 11,
    LIC_CC0           = 12,
    LIC_WTFPL         = 13,
    LIC_ARTISTIC_2    = 14,
    LIC_ZLIB          = 15,
    LIC_PROPRIETARY   = 16,
    LIC_DUAL          = 17,  /* multi-license, see license_str */
    LIC_COUNT
} license_t;

typedef enum {
    SEV_NONE     = 0,
    SEV_INFO     = 1,
    SEV_WARNING  = 2,
    SEV_ERROR    = 3,
    SEV_CRITICAL = 4,
} severity_t;

typedef enum {
    NODE_PROJECT    = 0,
    NODE_FILE       = 1,
    NODE_DEPENDENCY = 2,
    NODE_LICENSE    = 3,
    NODE_SECURITY   = 4,
    NODE_CONFIG     = 5,
    NODE_RECIPE_OUT = 6,
    /* --- extend here, never reorder above --- */
    NODE_TYPE_COUNT
} node_type_t;

typedef enum {
    SEAL_NONE      = 0,
    SEAL_LOCKED    = 1,   /* chmod 444, checksum verified */
    SEAL_IMMUTABLE = 2,   /* chattr +i on Linux          */
} seal_status_t;

typedef enum {
    HEALTH_UNKNOWN   = 0,
    HEALTH_HEALTHY   = 1,
    HEALTH_DEGRADED  = 2,
    HEALTH_UNHEALTHY = 3,
} health_t;

typedef enum {
    SEC_PUBLIC   = 0,
    SEC_PRIVATE  = 1,
    SEC_ISOLATED = 2,
    SEC_SECURE   = 3,
    SEC_CRITICAL = 4,
} security_level_t;

/* Vulnerability types — from security scanner */
typedef enum {
    VULN_INJECTION       = 0,   /* SQL/command injection      */
    VULN_AUTHENTICATION  = 1,
    VULN_CRYPTOGRAPHIC   = 2,   /* weak crypto (MD5/SHA1)     */
    VULN_ACCESS_CONTROL  = 3,
    VULN_CONFIGURATION   = 4,
    VULN_DATA_EXPOSURE   = 5,   /* hardcoded creds/secrets    */
    VULN_BUFFER_OVERFLOW = 6,
    VULN_RACE_CONDITION  = 7,
    VULN_NEURAL_SAFETY   = 8,   /* BCI-specific               */
    VULN_TYPE_COUNT
} vulnerability_type_t;

/* Code check types — from code_check framework */
typedef enum {
    CHK_SYNTAX         = 0,
    CHK_STATIC_ANALYSIS= 1,   /* bare except, eval/exec     */
    CHK_SECURITY       = 2,   /* overlaps with vuln scanner  */
    CHK_INTEGRITY      = 3,   /* incomplete files, hashes    */
    CHK_COMPLEXITY     = 4,   /* cyclomatic complexity       */
    CHK_STYLE          = 5,
    CHK_TYPE_COUNT
} check_type_t;

/* PQC algorithms — NIST FIPS 203/204/205 */
typedef enum {
    PQC_UNKNOWN          = 0,
    PQC_KYBER_512        = 1,   /* FIPS 203 KEM */
    PQC_KYBER_768        = 2,
    PQC_KYBER_1024       = 3,
    PQC_DILITHIUM_2      = 4,   /* FIPS 204 Signatures */
    PQC_DILITHIUM_3      = 5,
    PQC_DILITHIUM_5      = 6,
    PQC_SPHINCS_128F     = 7,   /* FIPS 205 Hash Signatures */
    PQC_SPHINCS_256F     = 8,
    PQC_ED25519          = 9,   /* classical hybrid */
    PQC_RSA_4096         = 10,
    PQC_ALG_COUNT
} pqc_algorithm_t;

/* Compliance standards */
typedef enum {
    COMP_FDA_21CFR820    = 0,   /* Quality Management  */
    COMP_HIPAA           = 1,   /* PHI Protection      */
    COMP_GDPR            = 2,   /* Data Privacy        */
    COMP_ISO14708_3      = 3,   /* Neurostimulators    */
    COMP_SOC2            = 4,   /* Service Org Controls */
    COMP_NIST_AI_RMF     = 5,   /* AI Risk Management  */
    COMP_ISO42001        = 6,   /* AI Management System */
    COMP_EU_AI_ACT       = 7,   /* EU AI Regulation    */
    COMP_COUNT
} compliance_standard_t;

/* --------------------------------------------------------------------------
 * AI Agent Lifecycle -- stages, risk dimensions, traceability, compliance
 * -------------------------------------------------------------------------- */

/* Agent lifecycle stages -- ordered state machine */
typedef enum {
    AGENT_PROPOSED     = 0,   /* Design/spec phase, not yet approved       */
    AGENT_APPROVED     = 1,   /* Approved for implementation               */
    AGENT_INITIALIZED  = 2,   /* Code exists, not yet active               */
    AGENT_ACTIVE       = 3,   /* Running in production                     */
    AGENT_SUSPENDED    = 4,   /* Temporarily halted (manual or automatic)  */
    AGENT_TERMINATED   = 5,   /* Permanently stopped, awaiting archive     */
    AGENT_ARCHIVED     = 6,   /* Immutable record retained for audit       */
    AGENT_STAGE_COUNT
} agent_stage_t;

/* Risk dimensions -- NIST AI RMF aligned */
typedef enum {
    RISK_CAPABILITY    = 0,   /* Tool/API access scope               */
    RISK_DATA_ACCESS   = 1,   /* Sensitivity of accessible data      */
    RISK_AUTONOMY      = 2,   /* Unsupervised operation level        */
    RISK_IMPACT        = 3,   /* Blast radius of actions             */
    RISK_TEMPORAL      = 4,   /* Duration/persistence of effects     */
    RISK_DIM_COUNT
} risk_dimension_t;

/* Risk tiers -- maps to HITL requirements */
typedef enum {
    RISK_TIER_MINIMAL  = 0,   /* No special controls                 */
    RISK_TIER_LOW      = 1,   /* Logging sufficient                  */
    RISK_TIER_MODERATE = 2,   /* Periodic review required            */
    RISK_TIER_HIGH     = 3,   /* HITL approval for critical actions  */
    RISK_TIER_CRITICAL = 4,   /* Mandatory human oversight always    */
    RISK_TIER_COUNT
} risk_tier_t;

/* Trace event types -- audit trail categories */
typedef enum {
    TRACE_ACTION       = 0,   /* Agent performed an action           */
    TRACE_DECISION     = 1,   /* Agent made an autonomous decision   */
    TRACE_DELEGATION   = 2,   /* Agent delegated to sub-agent        */
    TRACE_ESCALATION   = 3,   /* Agent escalated to human            */
    TRACE_OVERRIDE     = 4,   /* Human overrode agent decision       */
    TRACE_GUARDRAIL    = 5,   /* Guardrail triggered                 */
    TRACE_LIFECYCLE    = 6,   /* Lifecycle state transition           */
    TRACE_EVENT_COUNT
} trace_event_type_t;

/* Human-in-the-loop requirements */
typedef enum {
    HITL_NONE          = 0,   /* Fully autonomous                    */
    HITL_ADVISORY      = 1,   /* Human notified, no approval needed  */
    HITL_APPROVAL      = 2,   /* Human approval before execution     */
    HITL_MANDATORY     = 3,   /* Human must execute directly         */
    HITL_COUNT
} hitl_requirement_t;

/* Guardrail types */
typedef enum {
    GUARD_RATE_LIMIT    = 0,   /* Rate limiting on actions            */
    GUARD_SCOPE_BOUND   = 1,   /* Scope constraint enforcement        */
    GUARD_DATA_HANDLING = 2,   /* PII/PHI data rules                  */
    GUARD_HITL_GATE     = 3,   /* Human-in-the-loop gate              */
    GUARD_KILL_SWITCH   = 4,   /* Emergency termination               */
    GUARD_BUDGET_CAP    = 5,   /* Resource/cost budget limits         */
    GUARD_TYPE_COUNT
} guardrail_type_t;

/* Component lifecycle states — from morphogenetic/types.py */
typedef enum {
    COMP_STATE_DISCONNECTED  = 0,
    COMP_STATE_CONNECTING    = 1,
    COMP_STATE_CONNECTED     = 2,
    COMP_STATE_DISCONNECTING = 3,
    COMP_STATE_FAILED        = 4,
    COMP_STATE_COUNT
} component_state_t;

/* Morphogenetic repair modes — neuroanatomical glial cell mapping */
typedef enum {
    REPAIR_HEALING      = 0,   /* Hours-days: microglial phagocytosis      */
    REPAIR_REPAIR       = 1,   /* Days-weeks: astrocyte remyelination      */
    REPAIR_OPTIMIZATION = 2,   /* Weeks-months: activity-dependent myelin  */
    REPAIR_ADAPTATION   = 3,   /* Minutes-hours: synaptic plasticity       */
    REPAIR_MODE_COUNT
} repair_mode_t;

/* Component issue types — from morphogenetic/types.py */
typedef enum {
    ISSUE_DEGRADED_PERFORMANCE = 0,
    ISSUE_CONNECTIVITY_LOSS    = 1,
    ISSUE_MEMORY_LEAK          = 2,
    ISSUE_DEADLOCK             = 3,
    ISSUE_CORRUPTION           = 4,
    ISSUE_RESOURCE_EXHAUSTION  = 5,
    ISSUE_LATENCY_VIOLATION    = 6,
    ISSUE_TYPE_COUNT
} component_issue_type_t;

/* Source file language */
typedef enum {
    LANG_UNKNOWN   = 0,
    LANG_C         = 1,
    LANG_CPP       = 2,
    LANG_PYTHON    = 3,
    LANG_JAVASCRIPT= 4,
    LANG_TYPESCRIPT= 5,
    LANG_PHP       = 6,
    LANG_RUST      = 7,
    LANG_GO        = 8,
    LANG_JAVA      = 9,
    LANG_RUBY      = 10,
    LANG_SWIFT     = 11,
    LANG_KOTLIN    = 12,
    LANG_SHELL     = 13,
    LANG_SQL       = 14,
    LANG_HTML      = 15,
    LANG_CSS       = 16,
    LANG_JSON      = 17,
    LANG_YAML      = 18,
    LANG_MARKDOWN  = 19,
    LANG_COUNT
} language_t;

/* --------------------------------------------------------------------------
 * Primitive structs — fixed layout, no pointers to heap
 * -------------------------------------------------------------------------- */

/* Dependency record — one per third-party package */
typedef struct {
    char            name[LST_MAX_NAME];
    char            version[64];
    uint8_t         license;          /* license_t */
    char            license_str[LST_MAX_LICENSE]; /* raw string if LIC_DUAL */
    uint8_t         pkg_manager;      /* pkg_manager_t */
    char            authors[LST_MAX_AUTHORS][LST_MAX_AUTHOR_NAME];
    uint8_t         author_count;
    char            copyright[LST_MAX_NAME];
    char            source_path[LST_MAX_PATH];
} lst_dep_t;

/* Source file record */
typedef struct {
    char            path[LST_MAX_PATH];
    uint32_t        size_bytes;
    uint32_t        line_count;
    uint8_t         language;         /* future: language_t enum */
    uint8_t         hash[32];         /* SHA-256 */
} lst_file_t;

/* Security issue */
typedef struct {
    char            id[64];
    uint8_t         severity;         /* severity_t */
    char            title[LST_MAX_NAME];
    char            description[LST_MAX_NAME];
    char            file_path[LST_MAX_PATH];
    uint32_t        line_number;
    char            cwe[16];          /* e.g. "CWE-89" */
    char            remediation[LST_MAX_NAME];
} lst_issue_t;

/* --------------------------------------------------------------------------
 * Agent Lifecycle Record -- tracks agent state through its lifecycle
 * -------------------------------------------------------------------------- */

typedef struct {
    char            agent_id[LST_MAX_NAME];
    char            agent_name[LST_MAX_NAME];
    uint8_t         current_stage;        /* agent_stage_t */
    uint8_t         risk_tier;            /* risk_tier_t */
    uint8_t         hitl_requirement;     /* hitl_requirement_t */
    float           risk_scores[RISK_DIM_COUNT]; /* per-dimension 0.0-1.0 */
    float           aggregate_risk;       /* weighted composite 0.0-100.0 */
    time_t          created_at;
    time_t          last_transition;
    char            owner[LST_MAX_NAME];  /* responsible human */
    char            approver[LST_MAX_NAME]; /* approval authority */
    uint32_t        transition_count;
    uint8_t         has_kill_switch;       /* 0 or 1 */
    uint8_t         has_audit_trail;       /* 0 or 1 */
    uint8_t         has_scope_bounds;      /* 0 or 1 */
} lst_agent_record_t;

/* --------------------------------------------------------------------------
 * Trace Record -- immutable audit trail entry
 * -------------------------------------------------------------------------- */

typedef struct {
    char            trace_id[64];
    char            parent_trace_id[64];  /* for delegation chains */
    char            agent_id[LST_MAX_NAME];
    uint8_t         event_type;           /* trace_event_type_t */
    uint8_t         severity;             /* severity_t */
    time_t          timestamp;
    char            action[LST_MAX_NAME];
    char            reasoning[LST_MAX_NAME]; /* decision rationale */
    char            outcome[LST_MAX_NAME];
    uint8_t         human_involved;       /* 0 or 1 */
    uint8_t         guardrail_triggered;  /* 0 or 1 */
} lst_trace_record_t;

/* --------------------------------------------------------------------------
 * Guardrail Rule -- compliance constraint definition
 * -------------------------------------------------------------------------- */

typedef struct {
    char            rule_id[64];
    char            description[LST_MAX_NAME];
    uint8_t         guardrail_type;       /* guardrail_type_t */
    uint8_t         compliance_standard;  /* compliance_standard_t */
    uint8_t         severity_on_breach;   /* severity_t */
    uint8_t         hitl_requirement;     /* hitl_requirement_t */
    uint8_t         enabled;              /* 0 or 1 */
    char            pattern[LST_MAX_NAME]; /* detection pattern */
    uint32_t        trigger_count;        /* times triggered */
    uint32_t        bypass_count;         /* authorized bypasses */
} lst_guardrail_t;

/* --------------------------------------------------------------------------
 * LST Node — tagged union, the fundamental unit of the tree
 * -------------------------------------------------------------------------- */

typedef struct {
    uint8_t         type;             /* node_type_t */
    uint32_t        id;               /* unique within tree */
    uint32_t        parent_id;        /* 0 = root */
    uint32_t        child_ids[64];
    uint16_t        child_count;
    /* payload index into the appropriate array in lst_artifact_t */
    uint32_t        payload_index;
} lst_node_t;

/* --------------------------------------------------------------------------
 * LST Artifact — the complete serializable project representation
 *
 * This is the unit of storage. Built once per project, written to
 * store/, read by any recipe. All arrays are inline (no pointers).
 * -------------------------------------------------------------------------- */

typedef struct {
    /* Header */
    uint32_t        magic;            /* 0x4C535400 "LST\0" */
    uint32_t        version;          /* artifact format version */
    char            project_name[LST_MAX_NAME];
    char            project_path[LST_MAX_PATH];
    time_t          built_at;
    uint8_t         checksum[32];     /* SHA-256 of payload */
    uint8_t         seal_status;      /* seal_status_t */

    /* Package manager presence flags */
    uint8_t         has_manager[PKG_COUNT];

    /* Dependency array */
    lst_dep_t       deps[LST_MAX_DEPS];
    uint32_t        dep_count;

    /* Source file array */
    lst_file_t      files[LST_MAX_FILES];
    uint32_t        file_count;

    /* Security issues */
    lst_issue_t     issues[LST_MAX_ISSUES];
    uint32_t        issue_count;

    /* Node tree */
    lst_node_t      nodes[LST_MAX_NODES];
    uint32_t        node_count;

    /* Agent lifecycle records */
    lst_agent_record_t agents[LST_MAX_AGENTS];
    uint32_t        agent_count;

    /* Trace records (audit trail) */
    lst_trace_record_t traces[LST_MAX_TRACES];
    uint32_t        trace_count;

    /* Compliance guardrails */
    lst_guardrail_t guardrails[LST_MAX_GUARDRAILS];
    uint32_t        guardrail_count;

    /* Aggregate stats */
    uint8_t         health;           /* health_t */
    uint8_t         security_level;   /* security_level_t */
    float           risk_score;       /* 0.0 - 100.0 */
} lst_artifact_t;

/* --------------------------------------------------------------------------
 * LST Builder API
 * -------------------------------------------------------------------------- */

/* Allocate and initialize a new artifact for the given project path. */
lst_artifact_t *lst_create(const char *project_path);

/* Scan and populate: dependencies, files, issues. */
int lst_build(lst_artifact_t *art);

/* Individual scanners — called by lst_build, exposed for recipes. */
int lst_scan_deps_composer(lst_artifact_t *art);
int lst_scan_deps_npm(lst_artifact_t *art);
int lst_scan_deps_pip(lst_artifact_t *art);
int lst_scan_deps_cargo(lst_artifact_t *art);
int lst_scan_deps_go(lst_artifact_t *art);
int lst_scan_files(lst_artifact_t *art);

/* Compute checksum over payload. */
void lst_checksum(lst_artifact_t *art);

/* Free artifact memory. */
void lst_destroy(lst_artifact_t *art);

/* --------------------------------------------------------------------------
 * LST Store API
 * -------------------------------------------------------------------------- */

/* Serialize artifact to file. Returns 0 on success. */
int lst_store_write(const lst_artifact_t *art, const char *store_dir);

/* Deserialize artifact from file. Caller must lst_destroy(). */
lst_artifact_t *lst_store_read(const char *store_dir, const char *project_name);

/* Check if stored artifact exists and matches current project state. */
int lst_store_is_current(const char *store_dir, const char *project_name);

/* List all stored artifacts. Returns count, fills names array. */
int lst_store_list(const char *store_dir, char names[][LST_MAX_NAME], int max);

/* --------------------------------------------------------------------------
 * Recipe API
 * -------------------------------------------------------------------------- */

/* Recipe function signature: takes artifact, returns 0 on success.
 * Recipes NEVER modify the source project — they produce output
 * (files, reports, diffs) or modify the artifact's output nodes. */
typedef int (*recipe_fn_t)(lst_artifact_t *art, const char *output_dir);

typedef struct {
    char            name[LST_MAX_NAME];
    char            description[LST_MAX_NAME];
    recipe_fn_t     execute;
    uint32_t        version;
} lst_recipe_t;

/* Register a recipe with the engine. */
int lst_recipe_register(const lst_recipe_t *recipe);

/* Look up recipe by name. Returns NULL if not found. */
const lst_recipe_t *lst_recipe_find(const char *name);

/* Run a named recipe against a stored artifact. */
int lst_recipe_run(const char *recipe_name, const char *project_name,
                   const char *store_dir, const char *output_dir);

/* List all registered recipes. Returns count, fills array. */
int lst_recipe_list(lst_recipe_t *out, int max);

/* --------------------------------------------------------------------------
 * Seal API — production immutability
 * -------------------------------------------------------------------------- */

/* Seal an output file (chmod 444, write checksum marker). */
int lst_seal(const char *file_path);

/* Verify seal integrity (checksum match). */
int lst_seal_verify(const char *file_path);

/* Amend a sealed file (append only, re-seal). */
int lst_seal_amend(const char *file_path, const char *amendment);

#ifdef __cplusplus
}
#endif

#endif /* ENGINE_LST_H */
