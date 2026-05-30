# NNOS: Start Here Guide
## Complete Walkthrough for First-Time Implementation

**You are here**: You have the specs, and you want working firmware.

**Goal**: Feed these specs into an AI code generator (Claude Code, Qwen Coder, DeepSeek, etc.) and get compiled daemons running on your Jetson or NUC.

---

## What You're Building

A **constellation of 6 C++ daemons** that:
1. Monitor your physiology in real-time (heart rate, sensory load, etc.)
2. Prevent burnout by enforcing task limits and context switch budgets
3. Detect and warn against "masking" (overcommitting when exhausted)
4. Synchronize state across all your devices via encrypted Ethernet

All compiled, no interpreted code, boots with your system.

---

## Prerequisites

### Hardware
- **Jetson Orin Nano** OR **ASUS NUC 15 Pro** (or any x86_64/ARM64 Linux box)
- 1Gbps Ethernet or WiFi 6 (for device sync)

### Software
- Ubuntu 22.04+ or Jetson Linux 36.4+
- GCC 11+ or Clang 14+
- CMake 3.18+
- OpenSSL 3.0+
- Git

Install on Ubuntu/Debian:
```bash
sudo apt update
sudo apt install -y build-essential cmake git \
  libssl-dev iconv file

