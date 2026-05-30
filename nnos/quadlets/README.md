# Podman Quadlet Files for NNOS Daemon Constellation

Place these files in `~/.config/containers/systemd/` (rootless) or
`/etc/containers/systemd/` (rootful) and run `systemctl --user daemon-reload`.

Per-daemon overrides can be created by copying `nnos-daemon.container`
and changing the `Exec=` line to the desired binary.
