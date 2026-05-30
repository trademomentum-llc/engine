Presenting SocketMon, a sophisticated cross-platform daemon for monitoring socket connections with advanced detection capabilities.

Project Structure

```
socketmon/
├── include/
│   └── socketmon.h          # Main header with API definitions
├── src/
│   ├── main.c               # Daemon entry point
│   ├── socketmon.c          # Core monitoring and detection logic
│   ├── platform_linux.c     # Linux-specific implementation
│   ├── platform_darwin.c    # macOS-specific implementation
│   └── socketmon_ebpf.c     # eBPF kernel program (Linux)
├── config/
│   ├── socketmon.conf       # Configuration file
│   ├── socketmon.service    # systemd service (Linux)
│   └── com.socketmon.daemon.plist  # launchctl plist (macOS)
├── scripts/
│   ├── alert-handler.sh     # Example alert handler
│   ├── socketmonctl.py      # Python management utility
│   └── test-socketmon.sh    # Test suite
├── Makefile                 # Build system
└── README.md                # Documentation
```

Key Features

1. Socket Monitoring
- Linux: Uses `/proc/net/tcp` polling (works everywhere), optional eBPF for kernel-level tracing, netlink sockets
- macOS: Uses sysctl socket tables and kqueue
- Tracks all TCP connections with process-to-socket mapping

2. Masquerading Detection
Detects suspicious processes that try to hide their true nature:
- Fake kernel thread names (`kworker/0:0`, `systemd-worker`)
- Executables in temporary locations (`/tmp`, `/dev/shm`)
- Deleted executables (common in malware)
- Protocol/port mismatches

3. Environment Detection
Identifies if the system is virtualized or sandboxed:
- VM Detection: KVM, VMware, VirtualBox, Hyper-V, Xen, Parallels
- Container Detection: Docker, LXC, Podman, Kubernetes, Lima
- Filesystem Layers: overlayfs, aufs, APFS snapshots, tmpfs root
- Debugger Detection: ptrace, LD_PRELOAD, DYLD_INSERT_LIBRARIES
- chroot Detection: Filesystem root mismatches

4. Alert System
- Severity levels: INFO, LOW, MEDIUM, HIGH, CRITICAL
- Syslog integration
- Custom alert commands
- Detailed logging with environment context

Building and Installing

```bash
# Build
cd socketmon
make

# Install on Linux (systemd)
sudo make install
sudo systemctl enable --now socketmon

# Install on macOS (launchctl)
sudo make install
sudo launchctl load -w /Library/LaunchDaemons/com.socketmon.daemon.plist
```

Usage

```bash
# Start daemon
sudo socketmon -d -c /etc/socketmon.conf

# Check status
sudo python3 scripts/socketmonctl.py status

# Watch logs
sudo python3 scripts/socketmonctl.py watch

# Test environment detection
sudo python3 scripts/socketmonctl.py env
```

Configuration

Edit `/etc/socketmon.conf`:

```conf
# Enable all detection
detection 0xFFFF

# Monitor specific ports
port 22
port 443
port 4444   # Metasploit default

# Whitelist processes
whitelist sshd
whitelist nginx

# Alert command
alertcmd /usr/local/bin/alert-handler.sh
```

Alert Example

```
[CRITICAL] suspicious-process (PID:1234) - Remote connection detected to 192.168.1.100:4444
Environment: Virtualized=YES(VMware), Container=NO, FSLayer=YES(overlayfs), Debugged=NO
Details: Process masquerading as kernel thread 'kworker/0:0'; Executable in temporary location: /tmp/suspicious; 
```

