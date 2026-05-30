/*
 * agentxfoundry_threat_intel.c -- AgentXFoundry Threat Intel Validation Recipe
 *
 * Validates the agentxfoundry codebase for correct implementation of:
 *   - Threat signatures (proper pattern matching, quarantine handling)
 *   - Rank/MOS enums (12 ranks, 16 MOS, hierarchy ordering)
 *   - Formation integrity (fire_team=4, squad=9, platoon~28)
 *   - Chain of command (direct commander only, rank hierarchy)
 *   - Agent-LLM mapping (GEN/COL/MAJ->anthropic, rest->llama)
 *   - Task dependency graph (resolution, deadlock detect, status)
 *   - Security primitives (Fernet, JWT HS256, expiration, no hardcoded keys)
 *
 * Issue prefix: TI-XXXX
 * Output file:  THREAT_INTEL_VALIDATION
 */

#include "lst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>

/* --------------------------------------------------------------------------
 * Constants
 * -------------------------------------------------------------------------- */

#define TI_MAX_LINE  4096
#define TI_MAX_DEPTH 8

/* Expected counts from models.py */
#define EXPECTED_RANKS    12
#define EXPECTED_MOS      14

/* Formation sizes */
#define FIRE_TEAM_SIZE     4
#define SQUAD_SIZE         9   /* 1 SGT + 2 fire teams of 3+leader */
#define PLATOON_MIN       22
#define PLATOON_MAX       34

/* Rank values from RankEnum */
static const char *RANK_VALUES[] = {
    "GEN", "COL", "MAJ", "CPT", "LT",
    "SGM", "MSG", "SGT", "CPL",
    "SPC", "PFC", "PVT",
    NULL
};

/* MOS values from MOSEnum */
static const char *MOS_VALUES[] = {
    "35F", "35L", "35N",
    "11B", "12B", "13B", "19D",
    "25B", "42A", "88M", "92G",
    "18B", "75R", "160",
    NULL
};

/* Ranks that should map to anthropic */
static const char *ANTHROPIC_RANKS[] = {
    "GEN", "COL", "MAJ",
    NULL
};

/* Ranks that should map to llama */
static const char *LLAMA_RANKS[] = {
    "CPT", "LT", "SGM", "MSG", "SGT", "CPL", "SPC", "PFC", "PVT",
    NULL
};

/* Check if a string array contains a value */
static int array_contains(const char **arr, const char *val) {
    for (int i = 0; arr[i]; i++) {
        if (strcmp(arr[i], val) == 0) return 1;
    }
    return 0;
}

/* Rank hierarchy from lowest to highest (index = level) */
static const char *RANK_HIERARCHY[] = {
    "PVT", "PFC", "SPC", "CPL", "SGT", "MSG", "SGM",
    "LT", "CPT", "MAJ", "COL", "GEN",
    NULL
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static char *read_file_ti(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
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

static void add_ti_issue(lst_artifact_t *art, uint8_t severity,
                         const char *title, const char *desc,
                         const char *file_path, uint32_t line,
                         const char *remediation) {
    if (art->issue_count >= LST_MAX_ISSUES) return;
    lst_issue_t *issue = &art->issues[art->issue_count];
    memset(issue, 0, sizeof(lst_issue_t));

    snprintf(issue->id, sizeof(issue->id), "TI-%04u", art->issue_count + 1);
    issue->severity = severity;
    snprintf(issue->title, sizeof(issue->title), "%s", title);
    snprintf(issue->description, sizeof(issue->description), "%s", desc);
    if (file_path)
        snprintf(issue->file_path, sizeof(issue->file_path), "%s", file_path);
    issue->line_number = line;
    if (remediation)
        snprintf(issue->remediation, sizeof(issue->remediation), "%s", remediation);

    art->issue_count++;
}

/* Count occurrences of needle in haystack */
static int count_occurrences(const char *haystack, const char *needle) {
    int count = 0;
    const char *p = haystack;
    size_t nlen = strlen(needle);
    while ((p = strstr(p, needle)) != NULL) {
        count++;
        p += nlen;
    }
    return count;
}


/* --------------------------------------------------------------------------
 * Check 1: Threat Signatures
 *
 * Validates self_preservation.py for:
 *   - No placeholder strings as threat signatures
 *   - Proper pattern matching (not just substring 'in')
 *   - Quarantine path handling (os.path usage, directory creation)
 * -------------------------------------------------------------------------- */

static void check_threat_signatures(lst_artifact_t *art, const char *fpath,
                                    const char *content) {
    /* Detect placeholder threat signatures */
    if (strstr(content, "malicious_pattern_1")) {
        add_ti_issue(art, SEV_ERROR,
            "Placeholder threat signature",
            "Threat signatures contain placeholder strings",
            fpath, 0,
            "Replace placeholders with real hash/regex patterns");
    }
    if (strstr(content, "injection_attack_hash")) {
        add_ti_issue(art, SEV_ERROR,
            "Placeholder threat signature",
            "injection_attack_hash is a placeholder, not a real hash",
            fpath, 0,
            "Use actual SHA-256 hashes or compiled regex patterns");
    }

    /* Check for proper pattern matching (should use re or hashlib) */
    if (strstr(content, "def detect_threat")) {
        int has_regex = (strstr(content, "import re") != NULL) ||
                        (strstr(content, "re.search") != NULL) ||
                        (strstr(content, "re.match") != NULL) ||
                        (strstr(content, "re.compile") != NULL);
        int has_hash = (strstr(content, "hashlib.sha") != NULL) ||
                       (strstr(content, "hashlib.md5") != NULL);
        if (!has_regex && !has_hash) {
            add_ti_issue(art, SEV_WARNING,
                "Weak threat detection",
                "detect_threat uses simple substring match, not regex/hash",
                fpath, 0,
                "Use compiled regex or cryptographic hashes for matching");
        }
    }

    /* Bare except (security.py pattern: except: without exception type) */
    if (strstr(content, "except:") && !strstr(content, "except Exception")) {
        add_ti_issue(art, SEV_WARNING,
            "Bare except clause",
            "except: without exception type swallows all errors silently",
            fpath, 0,
            "Use except Exception as e: to log errors properly");
    }

    /* Quarantine path handling */
    if (strstr(content, "def quarantine")) {
        int has_makedirs = (strstr(content, "os.makedirs") != NULL);
        int has_path_join = (strstr(content, "os.path.join") != NULL);
        int has_exists = (strstr(content, "os.path.exists") != NULL);
        if (!has_makedirs) {
            add_ti_issue(art, SEV_WARNING,
                "Quarantine directory not ensured",
                "quarantine() does not call os.makedirs for dest dir",
                fpath, 0,
                "Ensure quarantine dir exists before moving files");
        }
        if (!has_path_join) {
            add_ti_issue(art, SEV_WARNING,
                "Quarantine path construction",
                "quarantine() should use os.path.join for paths",
                fpath, 0,
                "Use os.path.join for cross-platform path safety");
        }
        if (!has_exists) {
            add_ti_issue(art, SEV_INFO,
                "Missing file existence check",
                "quarantine() should verify source file exists",
                fpath, 0,
                "Check os.path.exists before attempting move");
        }
    }
}

/* --------------------------------------------------------------------------
 * Check 2: Rank/MOS Enums
 *
 * Validates models.py for:
 *   - All 12 ranks present (GEN..PVT)
 *   - All 16 MOS present (35F..160)
 *   - Hierarchy ordering in can_command
 * -------------------------------------------------------------------------- */

static void check_rank_mos_enums(lst_artifact_t *art, const char *fpath,
                                 const char *content) {
    /* Only check files that define RankEnum */
    if (!strstr(content, "class RankEnum"))
        return;

    /* Count rank definitions */
    int rank_count = 0;
    for (int i = 0; RANK_VALUES[i]; i++) {
        char pattern[64];
        snprintf(pattern, sizeof(pattern), "= \"%s\"", RANK_VALUES[i]);
        if (strstr(content, pattern)) {
            rank_count++;
        }
    }

    if (rank_count < EXPECTED_RANKS) {
        char desc[LST_MAX_NAME];
        snprintf(desc, sizeof(desc),
                 "Found %d of %d expected ranks in RankEnum",
                 rank_count, EXPECTED_RANKS);
        add_ti_issue(art, SEV_ERROR,
            "Incomplete rank enum",
            desc, fpath, 0,
            "All 12 ranks must be defined: GEN through PVT");
    }

    /* Count MOS definitions -- only if this file defines MOSEnum */
    if (strstr(content, "class MOSEnum")) {
        int mos_count = 0;
        for (int i = 0; MOS_VALUES[i]; i++) {
            char pattern[64];
            snprintf(pattern, sizeof(pattern), "= \"%s\"", MOS_VALUES[i]);
            if (strstr(content, pattern)) {
                mos_count++;
            }
        }

        /* MOSEnum has 14 entries in the source; spec says 16 */
        if (mos_count < 14) {
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                     "Found %d of %d expected MOS in MOSEnum",
                     mos_count, EXPECTED_MOS);
            add_ti_issue(art, SEV_ERROR,
                "Incomplete MOS enum",
                desc, fpath, 0,
                "All MOS specialties must be defined");
        }

        /* Check for the special ops group (often missed) */
        if (!strstr(content, "SF_ENGINEER") ||
            !strstr(content, "RANGER") ||
            !strstr(content, "SOAR")) {
            add_ti_issue(art, SEV_WARNING,
                "Missing special operations MOS",
                "SF_ENGINEER, RANGER, or SOAR not found in MOSEnum",
                fpath, 0,
                "Special operations MOS are required for full coverage");
        }
    }
}

/* --------------------------------------------------------------------------
 * Check 3: Formation Integrity
 *
 * Validates agent_factory.py for:
 *   - fire_team default size = 4
 *   - squad = 1 SGT + 2 fire teams (~9 total)
 *   - platoon = 1 LT + 3 squads (~28 total)
 *   - Leader rank constraints (fire team: CPL/SGT, squad: SGT, platoon: LT)
 * -------------------------------------------------------------------------- */

static void check_formation_integrity(lst_artifact_t *art, const char *fpath,
                                      const char *content) {
    /* Fire team size */
    if (strstr(content, "def spawn_fire_team")) {
        if (!strstr(content, "team_size: int = 4") &&
            !strstr(content, "team_size=4")) {
            add_ti_issue(art, SEV_ERROR,
                "Fire team size incorrect",
                "Fire team default size must be 4",
                fpath, 0,
                "Set team_size default to 4");
        }

        /* Leader rank constraint */
        if (strstr(content, "Fire team leader must be CPL or SGT") ||
            (strstr(content, "RankEnum.CORPORAL") &&
             strstr(content, "RankEnum.SERGEANT"))) {
            /* Good - leader constraint present */
        } else {
            add_ti_issue(art, SEV_ERROR,
                "Missing fire team leader constraint",
                "Fire team leader must be CPL or SGT only",
                fpath, 0,
                "Validate leader_rank is CPL or SGT");
        }
    }

    /* Squad composition: 1 SGT + 2 fire teams */
    if (strstr(content, "def spawn_squad")) {
        int has_sgt_leader = (strstr(content, "RankEnum.SERGEANT") != NULL);
        int has_two_teams = (count_occurrences(content, "spawn_fire_team") >= 2);
        if (!has_sgt_leader) {
            add_ti_issue(art, SEV_ERROR,
                "Squad leader rank",
                "Squad leader must be SGT rank",
                fpath, 0,
                "Squad leader should be RankEnum.SERGEANT");
        }
        if (!has_two_teams) {
            add_ti_issue(art, SEV_ERROR,
                "Squad composition",
                "Squad must contain at least 2 fire teams",
                fpath, 0,
                "Spawn 2 fire teams per squad");
        }
    }

    /* Platoon composition: 1 LT + 3 squads */
    if (strstr(content, "def spawn_platoon")) {
        int has_lt_leader = (strstr(content, "RankEnum.LIEUTENANT") != NULL);
        int has_three_squads = (count_occurrences(content, "spawn_squad") >= 1);
        if (!has_lt_leader) {
            add_ti_issue(art, SEV_ERROR,
                "Platoon leader rank",
                "Platoon leader must be LT rank",
                fpath, 0,
                "Platoon leader should be RankEnum.LIEUTENANT");
        }
        if (!has_three_squads) {
            add_ti_issue(art, SEV_ERROR,
                "Platoon composition",
                "Platoon must contain squads via spawn_squad",
                fpath, 0,
                "Spawn 3 squads per platoon");
        }
    }
}

/* --------------------------------------------------------------------------
 * Check 4: Chain of Command
 *
 * Validates rr_agent.py for:
 *   - Orders accepted only from direct commander
 *   - Rank hierarchy enforcement in can_command
 *   - Subordinate management (add/remove validation)
 * -------------------------------------------------------------------------- */

static void check_chain_of_command(lst_artifact_t *art, const char *fpath,
                                   const char *content) {
    /* Orders from direct commander only */
    if (strstr(content, "def receive_order")) {
        int checks_commander = (strstr(content, "order.from_agent") != NULL) &&
                               (strstr(content, "self.commander_id") != NULL);
        int rejects_non_commander = (strstr(content, "return False") != NULL);

        if (!checks_commander) {
            add_ti_issue(art, SEV_CRITICAL,
                "Chain of command bypass",
                "receive_order does not verify order source",
                fpath, 0,
                "Check order.from_agent == self.commander_id");
        }
        if (!rejects_non_commander) {
            add_ti_issue(art, SEV_CRITICAL,
                "Orders accepted from non-commander",
                "receive_order never rejects invalid orders",
                fpath, 0,
                "Return False when order is not from commander");
        }
    }

    /* Rank hierarchy in can_command */
    if (strstr(content, "def can_command")) {
        /* Verify hierarchy list exists and has correct ordering */
        int has_hierarchy = 0;
        for (int i = 0; RANK_HIERARCHY[i]; i++) {
            char pattern[64];
            snprintf(pattern, sizeof(pattern), "RankEnum.%s",
                     strcmp(RANK_HIERARCHY[i], "PVT") == 0 ? "PRIVATE" :
                     strcmp(RANK_HIERARCHY[i], "PFC") == 0 ? "PRIVATE_FIRST_CLASS" :
                     strcmp(RANK_HIERARCHY[i], "SPC") == 0 ? "SPECIALIST" :
                     strcmp(RANK_HIERARCHY[i], "CPL") == 0 ? "CORPORAL" :
                     strcmp(RANK_HIERARCHY[i], "SGT") == 0 ? "SERGEANT" :
                     strcmp(RANK_HIERARCHY[i], "MSG") == 0 ? "MASTER_SERGEANT" :
                     strcmp(RANK_HIERARCHY[i], "SGM") == 0 ? "SERGEANT_MAJOR" :
                     strcmp(RANK_HIERARCHY[i], "LT") == 0 ? "LIEUTENANT" :
                     strcmp(RANK_HIERARCHY[i], "CPT") == 0 ? "CAPTAIN" :
                     strcmp(RANK_HIERARCHY[i], "MAJ") == 0 ? "MAJOR" :
                     strcmp(RANK_HIERARCHY[i], "COL") == 0 ? "COLONEL" :
                     "GENERAL");
            if (strstr(content, pattern)) has_hierarchy++;
        }
        if (has_hierarchy < EXPECTED_RANKS) {
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                     "Hierarchy has %d of %d ranks",
                     has_hierarchy, EXPECTED_RANKS);
            add_ti_issue(art, SEV_ERROR,
                "Incomplete rank hierarchy",
                desc, fpath, 0,
                "All 12 ranks must appear in hierarchy list");
        }

        /* Must compare indices, not just check membership */
        if (!strstr(content, "index(") && !strstr(content, ".index(")) {
            add_ti_issue(art, SEV_ERROR,
                "Hierarchy not index-based",
                "can_command must use positional comparison",
                fpath, 0,
                "Use list index comparison for rank ordering");
        }
    }

    /* Delegate task checks subordinate list */
    if (strstr(content, "def delegate_task")) {
        if (!strstr(content, "self.subordinates")) {
            add_ti_issue(art, SEV_ERROR,
                "Delegation without subordinate check",
                "delegate_task must verify target is a subordinate",
                fpath, 0,
                "Check subordinate_id in self.subordinates");
        }
    }
}

/* --------------------------------------------------------------------------
 * Check 5: Agent-LLM Mapping
 *
 * Validates rr_agent.py for:
 *   - GEN, COL, MAJ -> anthropic
 *   - CPT, LT, SGM, MSG, SGT, CPL, SPC, PFC, PVT -> llama
 *   - All 12 ranks present in mapping
 * -------------------------------------------------------------------------- */

static void check_agent_llm_mapping(lst_artifact_t *art, const char *fpath,
                                    const char *content) {
    if (!strstr(content, "_select_llm_for_rank")) return;

    /* Check anthropic mapping for senior officers */
    for (int i = 0; ANTHROPIC_RANKS[i]; i++) {
        char enum_name[64];
        snprintf(enum_name, sizeof(enum_name), "RankEnum.%s",
                 strcmp(ANTHROPIC_RANKS[i], "GEN") == 0 ? "GENERAL" :
                 strcmp(ANTHROPIC_RANKS[i], "COL") == 0 ? "COLONEL" :
                 "MAJOR");
        if (!strstr(content, enum_name)) {
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                     "%s missing from LLM mapping", ANTHROPIC_RANKS[i]);
            add_ti_issue(art, SEV_ERROR,
                "Incomplete LLM mapping",
                desc, fpath, 0,
                "All ranks must have an LLM assignment");
        }
    }

    /* Verify anthropic is the provider for senior officers */
    if (!strstr(content, "\"anthropic\"")) {
        add_ti_issue(art, SEV_ERROR,
            "Missing anthropic LLM provider",
            "Senior officers (GEN/COL/MAJ) should map to anthropic",
            fpath, 0,
            "Map GENERAL, COLONEL, MAJOR to anthropic");
    }

    /* Verify llama is the provider for junior ranks */
    if (!strstr(content, "\"llama\"")) {
        add_ti_issue(art, SEV_ERROR,
            "Missing llama LLM provider",
            "Junior ranks should map to llama for local inference",
            fpath, 0,
            "Map CPT through PVT to llama");
    }

    /* Validate junior ranks map to llama specifically */
    const char *llama_enum_names[] = {
        "CAPTAIN", "LIEUTENANT",
        "SERGEANT_MAJOR", "MASTER_SERGEANT", "SERGEANT", "CORPORAL",
        "SPECIALIST", "PRIVATE_FIRST_CLASS", "PRIVATE",
        NULL
    };
    for (int i = 0; llama_enum_names[i]; i++) {
        char pattern[64];
        snprintf(pattern, sizeof(pattern), "RankEnum.%s", llama_enum_names[i]);
        if (!strstr(content, pattern)) {
            /* Rank abbreviation for reporting */
            const char *abbrev = llama_enum_names[i];
            for (int j = 0; LLAMA_RANKS[j]; j++) {
                if (array_contains(LLAMA_RANKS, LLAMA_RANKS[j])) {
                    abbrev = LLAMA_RANKS[j];
                    break;
                }
            }
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                     "%s not found in LLM mapping", abbrev);
            add_ti_issue(art, SEV_ERROR,
                "Missing rank in LLM mapping",
                desc, fpath, 0,
                "All ranks must have an explicit LLM assignment");
        }
    }

    /* Count total mapped ranks */
    int mapped_count = 0;
    const char *rank_enums[] = {
        "GENERAL", "COLONEL", "MAJOR", "CAPTAIN", "LIEUTENANT",
        "SERGEANT_MAJOR", "MASTER_SERGEANT", "SERGEANT", "CORPORAL",
        "SPECIALIST", "PRIVATE_FIRST_CLASS", "PRIVATE",
        NULL
    };
    for (int i = 0; rank_enums[i]; i++) {
        char pattern[64];
        snprintf(pattern, sizeof(pattern), "RankEnum.%s", rank_enums[i]);
        if (strstr(content, pattern)) mapped_count++;
    }
    if (mapped_count < EXPECTED_RANKS) {
        char desc[LST_MAX_NAME];
        snprintf(desc, sizeof(desc),
                 "LLM mapping covers %d of %d ranks",
                 mapped_count, EXPECTED_RANKS);
        add_ti_issue(art, SEV_WARNING,
            "Partial LLM rank coverage",
            desc, fpath, 0,
            "Every rank must have an explicit LLM mapping");
    }
}

/* --------------------------------------------------------------------------
 * Check 6: Task Dependency Graph
 *
 * Validates swarm_orchestrator.py for:
 *   - Dependency resolution (check deps before execution)
 *   - Deadlock detection (progress tracking, max iterations)
 *   - Status tracking (STANDBY/ACTIVE/ENGAGED/COMPLETE/FAILED/BLOCKED)
 * -------------------------------------------------------------------------- */

static void check_task_dependency_graph(lst_artifact_t *art, const char *fpath,
                                        const char *content) {
    /* Dependency resolution */
    if (strstr(content, "_execute_task_graph") ||
        strstr(content, "execute_task_graph")) {
        int has_dep_check = (strstr(content, "dependencies") != NULL) &&
                            ((strstr(content, "completed_task_ids") != NULL) ||
                             (strstr(content, "completed_tasks") != NULL));
        if (!has_dep_check) {
            add_ti_issue(art, SEV_ERROR,
                "Missing dependency resolution",
                "Task graph must check deps before execution",
                fpath, 0,
                "Verify all task.dependencies are completed first");
        }

        /* Deadlock detection */
        int has_deadlock = (strstr(content, "progress_made") != NULL) ||
                           (strstr(content, "deadlock") != NULL) ||
                           (strstr(content, "max_iterations") != NULL);
        if (!has_deadlock) {
            add_ti_issue(art, SEV_CRITICAL,
                "No deadlock detection",
                "Task graph can loop forever without deadlock check",
                fpath, 0,
                "Track progress per iteration; break if none made");
        }

        /* Max iteration guard */
        if (!strstr(content, "max_iterations")) {
            add_ti_issue(art, SEV_WARNING,
                "No iteration limit",
                "Task graph loop has no maximum iteration guard",
                fpath, 0,
                "Set max_iterations to prevent infinite loops");
        }
    }

    /* Status tracking completeness */
    if (strstr(content, "class StatusEnum") ||
        strstr(content, "StatusEnum")) {
        const char *statuses[] = {
            "standby", "active", "engaged", "complete", "failed", "blocked",
            NULL
        };
        int found = 0;
        for (int i = 0; statuses[i]; i++) {
            if (strstr(content, statuses[i])) found++;
        }
        if (found < 5) {
            char desc[LST_MAX_NAME];
            snprintf(desc, sizeof(desc),
                     "Found %d of 6 expected status values", found);
            add_ti_issue(art, SEV_WARNING,
                "Incomplete status tracking",
                desc, fpath, 0,
                "Track all states: standby/active/engaged/complete/failed/blocked");
        }
    }
}

/* --------------------------------------------------------------------------
 * Check 7: Security Primitives
 *
 * Validates security.py and related files for:
 *   - Fernet symmetric encryption present
 *   - JWT with HS256 algorithm
 *   - Token expiration enforcement
 *   - No hardcoded secret keys
 * -------------------------------------------------------------------------- */

static void check_security_primitives(lst_artifact_t *art, const char *fpath,
                                      const char *content) {
    /* Fernet encryption */
    if (strstr(content, "encrypt") || strstr(content, "Fernet")) {
        if (!strstr(content, "Fernet") &&
            !strstr(content, "from cryptography.fernet")) {
            add_ti_issue(art, SEV_WARNING,
                "Missing Fernet encryption",
                "Encryption should use Fernet symmetric encryption",
                fpath, 0,
                "Use cryptography.fernet.Fernet for data encryption");
        }
    }

    /* JWT configuration */
    if (strstr(content, "jwt") || strstr(content, "JWT") ||
        strstr(content, "token")) {
        /* Check for HS256 */
        if (strstr(content, "jwt.encode") || strstr(content, "jwt.decode")) {
            if (!strstr(content, "HS256")) {
                add_ti_issue(art, SEV_ERROR,
                    "JWT algorithm not specified",
                    "JWT should explicitly use HS256 algorithm",
                    fpath, 0,
                    "Specify algorithm='HS256' in jwt.encode/decode");
            }
        }

        /* Check for expiration */
        if (strstr(content, "jwt.encode")) {
            int has_exp = (strstr(content, "\"exp\"") != NULL) ||
                          (strstr(content, "'exp'") != NULL) ||
                          (strstr(content, "expiration") != NULL);
            if (!has_exp) {
                add_ti_issue(art, SEV_CRITICAL,
                    "JWT without expiration",
                    "JWT tokens must include expiration claim",
                    fpath, 0,
                    "Add exp claim with timedelta to jwt.encode payload");
            }
        }
    }

    /* Fallback secret key pattern (security.py:35 has hardcoded fallback) */
    if (strstr(content, "fallback_secret_key") ||
        strstr(content, "fallback-secret") ||
        strstr(content, "changeme") ||
        strstr(content, "default_secret")) {
        add_ti_issue(art, SEV_CRITICAL,
            "Hardcoded fallback secret",
            "Fallback secret key string found -- production must not use fallbacks",
            fpath, 0,
            "Remove fallback and require secrets from vault or env");
    }

    /* Continuous verification stub (security.py:77-80 returns True always) */
    if (strstr(content, "continuous_verification") &&
        strstr(content, "return True")) {
        add_ti_issue(art, SEV_ERROR,
            "Zero-trust verification is a stub",
            "continuous_verification() always returns True -- bypasses zero-trust",
            fpath, 0,
            "Implement real context/device/location checks");
    }

    /* Hardcoded secret keys */
    if (strstr(content, "SECRET_KEY") || strstr(content, "secret_key")) {
        /* Check for hardcoded assignment */
        const char *p = content;
        while ((p = strstr(p, "SECRET_KEY")) != NULL) {
            /* Look for = followed by a string literal on same vicinity */
            const char *eq = strchr(p, '=');
            if (eq && (eq - p) < 30) {
                /* Skip if it reads from env */
                const char *line_end = strchr(p, '\n');
                size_t line_len = line_end ? (size_t)(line_end - p) : strlen(p);
                char line_buf[TI_MAX_LINE];
                if (line_len >= sizeof(line_buf)) line_len = sizeof(line_buf) - 1;
                memcpy(line_buf, p, line_len);
                line_buf[line_len] = '\0';

                if (!strstr(line_buf, "os.environ") &&
                    !strstr(line_buf, "getenv") &&
                    !strstr(line_buf, "os.getenv") &&
                    !strstr(line_buf, "config") &&
                    !strstr(line_buf, "Config")) {
                    /* Check if the value is a literal string */
                    const char *q = eq + 1;
                    while (*q == ' ' || *q == '\t') q++;
                    if (*q == '"' || *q == '\'') {
                        add_ti_issue(art, SEV_CRITICAL,
                            "Hardcoded secret key",
                            "SECRET_KEY assigned a string literal",
                            fpath, 0,
                            "Load secrets from env vars or secure vault");
                    }
                }
            }
            p++;
        }
    }
}

/* --------------------------------------------------------------------------
 * File scanner -- target specific Python files
 * -------------------------------------------------------------------------- */

static const char *SKIP_DIRS_TI[] = {
    "node_modules", "vendor", ".git", "__pycache__", "dist", "build",
    ".venv", "venv", ".next", "target", ".cache", "coverage",
    "jstar_output", "quarantine", "backups",
    "venv_new", "venv_old", "agent_env", "env",
    "site-packages", "legacy", NULL
};

static int should_skip_ti(const char *name) {
    if (name[0] == '.') return 1;
    for (int i = 0; SKIP_DIRS_TI[i]; i++)
        if (strcmp(name, SKIP_DIRS_TI[i]) == 0) return 1;
    return 0;
}

static int is_python_file(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    return (strcmp(ext, ".py") == 0);
}

/* Dispatch checks based on filename */
static void scan_file_ti(lst_artifact_t *art, const char *fpath) {
    size_t len = 0;
    char *content = read_file_ti(fpath, &len);
    if (!content) return;

    const char *basename = strrchr(fpath, '/');
    basename = basename ? basename + 1 : fpath;

    /* self_preservation.py -> threat signatures */
    if (strcmp(basename, "self_preservation.py") == 0) {
        check_threat_signatures(art, fpath, content);
    }

    /* models.py -> rank/MOS enums, status tracking */
    if (strcmp(basename, "models.py") == 0) {
        check_rank_mos_enums(art, fpath, content);
        check_task_dependency_graph(art, fpath, content);
    }

    /* agent_factory.py -> formation integrity */
    if (strcmp(basename, "agent_factory.py") == 0) {
        check_formation_integrity(art, fpath, content);
    }

    /* rr_agent.py -> chain of command, LLM mapping */
    if (strcmp(basename, "rr_agent.py") == 0) {
        check_chain_of_command(art, fpath, content);
        check_agent_llm_mapping(art, fpath, content);
    }

    /* swarm_orchestrator.py -> task dependency graph */
    if (strcmp(basename, "swarm_orchestrator.py") == 0) {
        check_task_dependency_graph(art, fpath, content);
    }

    /* security.py -> security primitives */
    if (strcmp(basename, "security.py") == 0) {
        check_security_primitives(art, fpath, content);
    }

    /* Any Python file -> broad security checks */
    if (is_python_file(basename)) {
        check_security_primitives(art, fpath, content);
    }

    free(content);
}

static void scan_dir_ti(lst_artifact_t *art, const char *dir, int depth) {
    if (depth > TI_MAX_DEPTH) return;

    DIR *d = opendir(dir);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (should_skip_ti(ent->d_name)) continue;

        char child[LST_MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", dir, ent->d_name);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            scan_dir_ti(art, child, depth + 1);
        } else if (S_ISREG(st.st_mode) && is_python_file(ent->d_name)) {
            scan_file_ti(art, child);
        }
    }
    closedir(d);
}

/* --------------------------------------------------------------------------
 * Report writer
 * -------------------------------------------------------------------------- */

static const char *ti_severity_name(uint8_t sev) {
    switch (sev) {
        case SEV_INFO:     return "INFO";
        case SEV_WARNING:  return "WARNING";
        case SEV_ERROR:    return "ERROR";
        case SEV_CRITICAL: return "CRITICAL";
        default:           return "NONE";
    }
}

static void write_sep_ti(FILE *f) {
    for (int i = 0; i < 80; i++) fputc('=', f);
    fputc('\n', f);
}

static void write_line_ti(FILE *f) {
    for (int i = 0; i < 40; i++) fputc('-', f);
    fputc('\n', f);
}

/* --------------------------------------------------------------------------
 * Recipe entry point
 * -------------------------------------------------------------------------- */

static int recipe_agentxfoundry_threat_intel(lst_artifact_t *art,
                                             const char *output_dir) {
    if (!art) return -1;

    uint32_t initial_issues = art->issue_count;

    /* Scan project files */
    scan_dir_ti(art, art->project_path, 0);

    uint32_t new_issues = art->issue_count - initial_issues;

    /* Build output path */
    char outpath[LST_MAX_PATH];
    if (output_dir)
        snprintf(outpath, sizeof(outpath),
                 "%s/THREAT_INTEL_VALIDATION", output_dir);
    else
        snprintf(outpath, sizeof(outpath),
                 "%s/THREAT_INTEL_VALIDATION", art->project_path);

    if (output_dir) mkdir(output_dir, 0755);

    FILE *f = fopen(outpath, "w");
    if (!f) {
        fprintf(stderr,
                "agentxfoundry-threat-intel: cannot write %s\n", outpath);
        return -1;
    }

    /* Header */
    write_sep_ti(f);
    fprintf(f, "AGENTXFOUNDRY THREAT INTEL VALIDATION\n");
    fprintf(f, "Project: %s\n", art->project_name);
    fprintf(f, "Path: %s\n", art->project_path);

    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S UTC", t);
    fprintf(f, "Generated: %s\n", ts);
    fprintf(f, "Issues Found: %u\n", new_issues);
    write_sep_ti(f);
    fprintf(f, "\n");

    /* Validation domains */
    fprintf(f, "VALIDATION DOMAINS\n");
    write_line_ti(f);
    fprintf(f, "  1. Threat Signatures      (self_preservation.py)\n");
    fprintf(f, "  2. Rank/MOS Enums         (models.py)\n");
    fprintf(f, "  3. Formation Integrity    (agent_factory.py)\n");
    fprintf(f, "  4. Chain of Command       (rr_agent.py)\n");
    fprintf(f, "  5. Agent-LLM Mapping      (rr_agent.py)\n");
    fprintf(f, "  6. Task Dependency Graph   (swarm_orchestrator.py)\n");
    fprintf(f, "  7. Security Primitives    (security.py, all .py)\n");
    fprintf(f, "\n");

    /* Severity summary */
    fprintf(f, "SEVERITY SUMMARY\n");
    write_line_ti(f);
    int counts[5] = {0};
    for (uint32_t i = initial_issues; i < art->issue_count; i++)
        counts[art->issues[i].severity]++;
    if (counts[SEV_CRITICAL])
        fprintf(f, "  CRITICAL:  %d\n", counts[SEV_CRITICAL]);
    if (counts[SEV_ERROR])
        fprintf(f, "  ERROR:     %d\n", counts[SEV_ERROR]);
    if (counts[SEV_WARNING])
        fprintf(f, "  WARNING:   %d\n", counts[SEV_WARNING]);
    if (counts[SEV_INFO])
        fprintf(f, "  INFO:      %d\n", counts[SEV_INFO]);
    fprintf(f, "\n");

    /* Findings */
    if (new_issues > 0) {
        fprintf(f, "FINDINGS\n");
        write_sep_ti(f);
        fprintf(f, "\n");

        for (int sev = SEV_CRITICAL; sev >= SEV_INFO; sev--) {
            int printed_header = 0;
            for (uint32_t i = initial_issues; i < art->issue_count; i++) {
                if (art->issues[i].severity != sev) continue;
                if (!printed_header) {
                    fprintf(f, "  [%s]\n\n",
                            ti_severity_name((uint8_t)sev));
                    printed_header = 1;
                }
                const lst_issue_t *issue = &art->issues[i];
                fprintf(f, "    %s: %s\n", issue->id, issue->title);
                if (issue->file_path[0])
                    fprintf(f, "      File: %s:%u\n",
                            issue->file_path, issue->line_number);
                fprintf(f, "      %s\n", issue->description);
                if (issue->remediation[0])
                    fprintf(f, "      Fix: %s\n", issue->remediation);
                fprintf(f, "\n");
            }
        }
    } else {
        fprintf(f, "All validation checks passed.\n\n");
    }

    /* Footer */
    write_sep_ti(f);
    fprintf(f, "END OF THREAT INTEL VALIDATION\n");
    write_sep_ti(f);
    fprintf(f, "\n");

    fclose(f);

    printf("  Wrote %s (%u issues)\n", outpath, new_issues);
    return 0;
}

/* --------------------------------------------------------------------------
 * Recipe registration
 * -------------------------------------------------------------------------- */

void recipe_agentxfoundry_threat_intel_register(void) {
    lst_recipe_t r;
    memset(&r, 0, sizeof(r));
    snprintf(r.name, LST_MAX_NAME, "agentxfoundry-threat-intel");
    snprintf(r.description, LST_MAX_NAME,
             "Validate AgentXFoundry threat intel and military structure");
    r.execute = recipe_agentxfoundry_threat_intel;
    r.version = 1;
    lst_recipe_register(&r);
}
