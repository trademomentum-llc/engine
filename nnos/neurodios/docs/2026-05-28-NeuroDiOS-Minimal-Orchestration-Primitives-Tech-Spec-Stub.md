# Technical Specification (Stub): Minimal Viable Orchestration Primitives for NeuroDiOS Sovereign Platform Kernel

**Document ID:** NEURODIOS-SOVEREIGN-ORCH-PRIMITIVES-TECH-001  
**Version:** 0.1 (Stub)  
**Date:** 2026-05-28  
**Predecessors:** NEURODIOS-SOVEREIGN-KERNEL-REQ-001, NEURODIOS-SOVEREIGN-KERNEL-DS-001

---

## 1. Purpose of This Stub

This document defines the **minimal viable set of orchestration primitives** that a sovereign NeuroDiOS platform layer must provide to begin reducing dependence on external heavyweight orchestrators (such as OpenChoreo) for core weight-bearing concerns.

It is intentionally a stub. Full detail will be added as the Jasterish Micro-Kernel matures and the Design is refined.

---

## 2. Guiding Principle

**Start extremely small.** The first version of the sovereign orchestration layer should only own what is absolutely necessary to make a workload's lifecycle, environment context, and state authoritative inside the NeuroDiOS ecosystem — without trying to replace Kubernetes or the full OpenChoreo feature set immediately.

---

## 3. Minimal Viable Orchestration Primitives

### 3.1 Workload / Component Model
- Declarative specification (initially YAML/JSON, later potentially native JStar representations).
- Core concepts: Component (desired state), Workload (realized instance), Environment.
- Versioning and promotion metadata.

### 3.2 Environment & Tenancy Abstraction
- Explicit Environment entity with clear boundaries.
- Tenancy/namespace mapping strategy (platform-owned mapping, not fighting auto-generated namespaces).
- Environment promotion rules.

### 3.3 Workload Lifecycle Operations (Minimal)
- Create / Update / Promote / Retire (or equivalent).
- Observation / status reporting.
- Basic rollout strategy (at least rolling or recreate initially).

### 3.4 Secret / Configuration Projection
- Projection of secrets and config into workloads with provenance.
- Policy gating hooks (who can consume what).

### 3.5 State & Inventory
- Authoritative queryable state for Components, Workloads, Environments, and Deployments.
- Event stream for higher-level services (catalog, policy, UI).

### 3.6 Reconciliation (Minimal)
- A simple, auditable loop that drives desired state toward reality for the primitives above.
- Support for external reconciliation (coexistence) during transition.

---

## 4. What Is Explicitly Out of Scope for the First Version

- Full CI/CD execution engine.
- Complex deployment strategies (canary, blue-green, etc.).
- Advanced secret management (beyond basic projection with provenance).
- Multi-cluster or multi-cloud concerns.
- Full developer authoring surface (Score adapter can remain external initially).
- Complete replacement of OpenChoreo's UI or internal controllers.

---

## 5. Implementation Path (High-Level)

1. Model the above primitives as first-class concepts inside the NeuroDiOS ecosystem (likely as services running on or beside the Jasterish Micro-Kernel).
2. Build minimal reconciliation that can drive these primitives (initially on Kubernetes as a bridge).
3. Provide clean APIs / CRDs or native representations that higher layers (catalog, policy, Backstage) can consume.
4. Build adapters (e.g., improved score2openchoreo or native Score support) that emit into the sovereign model.
5. Gradually take ownership of more reconciliation and runtime concerns as the foundation stabilizes.

---

## 6. Relationship to Current Assets

- Builds directly on the existing Jasterish Micro-Kernel primitives (process, IPC, VFS, ELF loading, etc.).
- Depends on the maturing JStar compiler for implementation of higher layers where possible.
- The existing developer-portal Policy Guard Layer and Milestone System specs provide the governance requirements these primitives must ultimately support.

---

**End of Stub**

This document will be expanded into a full Technical Specification once the Design is further refined and the current Jasterish Micro-Kernel capabilities are more precisely mapped against the required primitives.