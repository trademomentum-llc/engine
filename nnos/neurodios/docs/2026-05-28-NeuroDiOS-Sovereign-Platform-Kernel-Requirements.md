# Requirements Specification: NeuroDiOS Sovereign Platform Kernel (Option D)

**Document ID:** NEURODIOS-SOVEREIGN-KERNEL-REQ-001  
**Version:** 0.1 (Initial Strategic Framing)  
**Date:** 2026-05-28  
**Status:** Strategic Option D – Sovereign Custom Platform

---

## 1. Strategic Context and Motivation

The current developer-portal + OpenChoreo stack is a high-friction assembly of third-party systems. OpenChoreo has been deliberately chosen as the load-bearing platform orchestrator across three of the five planes (Developer Control, Platform Orchestration, and Security).

While this delivers short-term functionality, it creates long-term dependency, complexity in translation layers (`score2openchoreo`), namespace ownership confusion, and reconciliation choreography that is difficult to reason about or own.

**Option D** is the explicit strategic path of building a sovereign, proprietary platform foundation using the Jasterish language and the NeuroDiOS evolutionary line, with the explicit goal of reducing or eventually eliminating dependence on external heavyweight orchestrators for core platform concerns.

This is not a rejection of all third-party tools, but a deliberate shift toward owning the critical "weight-bearing" layers.

---

## 2. Vision

A minimal, deterministic, Jasterish-native **Platform Kernel** that can eventually serve as the orchestration and control foundation for a full Internal Developer Platform (and the broader NeuroDiOS vision).

The kernel should provide the primitives that allow higher-level services (catalog, policy, delivery, observability integration, secret management, environment abstraction) to be built as user-space or co-kernel components without requiring an external third-party orchestrator to be the source of truth for workload lifecycle.

---

## 3. Scope (What This System Must Eventually Own or Strongly Abstract)

### Core Responsibilities to Internalize or Abstract
- Workload / Component lifecycle management (definition, deployment, promotion across environments).
- Environment and namespace strategy (cohesive model, not fighting auto-generated `dp-*` namespaces).
- Secret and configuration projection with strong provenance.
- Reconciliation / drift correction for platform add-ons and workloads.
- Developer authoring surface (Score or equivalent) with direct, first-class support (no mandatory heavy translation layer).
- Catalog / inventory of components, environments, and deployments that is authoritative.

### Out of Scope (at least initially)
- Full CI execution engine (can continue to use Gitea Actions or equivalent).
- Git hosting (Gitea or equivalent remains viable).
- Backstage UI itself (can remain or be replaced later).
- Low-level container runtime (Kubernetes or a future NeuroDiOS-native equivalent).

---

## 4. Functional Requirements (High-Level)

- **FR-1** The platform kernel must provide a native (or near-native) way to express workloads without requiring a mandatory custom translation binary for common developer authoring formats (Score or a NeuroDiOS-native equivalent).
- **FR-2** Workload and environment state must be queryable and authoritative from within the NeuroDiOS ecosystem (not primarily living inside an external orchestrator's CRDs).
- **FR-3** Namespace and tenancy strategy must be coherent and owned by the platform (not split between "developer view" and "auto-generated data-plane namespaces").
- **FR-4** Secret and configuration materialization must support strong provenance and policy gating.
- **FR-5** The system must support deterministic, auditable promotion of changes across environments.
- **FR-6** Higher-level developer experience tools (catalog, policy, UI) must be able to integrate cleanly without fighting the underlying orchestration model.

---

## 5. Non-Functional Requirements

- **Sovereignty**: The core orchestration model and workload state must be understandable and modifiable without depending on upstream changes to a third-party project.
- **Determinism**: Where possible, behavior should be reproducible given the same inputs (aligning with Jasterish and NeuroDiOS philosophy).
- **Minimal Trusted Base**: The privileged kernel surface should remain as small as possible (true to the micro-kernel lineage already present).
- **Evolvability**: The design must allow incremental replacement or coexistence with current third-party components during the long transition.

---

## 6. Constraints and Assumptions

- Primary implementation language for new platform logic: Jasterish (with Rust/Go where systems pragmatics demand it).
- The existing Jasterish Micro-Kernel (`jasterish-microkernel/`) is the starting privileged base.
- The JStar compiler and self-hosting work in `apps/` is the language toolchain.
- This is a multi-year strategic program, not a short-term replacement project.
- Coexistence with (or gradual migration away from) current tools (OpenChoreo, Backstage, Flux, etc.) is acceptable during the transition.

---

## 7. Success Criteria (Long-Term)

- A meaningful workload can be authored, promoted, and run using primarily NeuroDiOS-owned components without requiring OpenChoreo as the central reconciler.
- The translation tax (`score2openchoreo` or equivalent) is dramatically reduced or eliminated for core flows.
- Ownership boundaries are clear and the platform feels intentional rather than assembled.
- The system demonstrably supports the neurodivergent, deterministic, and morphogenetic principles of the broader NeuroDiOS vision.

---

**End of Initial Requirements Specification (Option D Framing)**

This document establishes Option D as a first-class strategic direction. Design and Technical Specifications will follow in subsequent revisions as the scope is refined against the existing Jasterish Micro-Kernel and compiler assets.

This work is being created per the governing persona rules for any new functional system or major directional module.