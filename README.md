# net

A command-line network management utility for FreeBSD. Provides an interactive
shell and direct command execution for managing network interfaces, addresses,
routes, ARP/NDP caches, VRFs, bridges, VLANs, LAGGs, tunnels, and wireless
interfaces.

## Features

- **Interface Management** — View and configure Ethernet, loopback, bridge,
  LAGG, VLAN, tunnel, TAP, WLAN, CARP, and virtual (epair) interfaces
- **Address Management** — Add and remove IPv4 and IPv6 addresses
- **Routing** — Display and manipulate routing tables across multiple VRFs
- **ARP / NDP** — Show and manage neighbor caches
- **VRF Support** — Multi-VRF routing tables and per-interface FIB assignment
- **Configuration Export** — Generate reproducible CLI commands from the live
  system state (`-g`), suitable for backup and replay
- **Interactive Shell** — Tab-completion and command history via libedit
- **Scripting** — Run single commands with `-c`, or pipe a command file to
  STDIN

## Prerequisites

- FreeBSD (tested on 14.x and 15-CURRENT)
- C++26 compiler (clang++ 19+ or equivalent)
- CMake 3.25+
- libedit
- Root privileges for configuration changes

## Building

```sh
cmake -S . -B build
cmake --build build -j$(sysctl -n hw.ncpu)
```

The executable is produced at `build/net`.

An optional `NETCLI_CLIENT` variable selects the configuration backend:

| Value | Description |
|-------|-------------|
| `lite` *(default)* | Uses FreeBSD system calls (ioctl / sysctl / routing sockets) |
| `netconf` | Stub backend for future NETCONF/YANG client |

```sh
cmake -S . -B build -DNETCLI_CLIENT=netconf
```

### Install

```sh
sudo cmake --install build
```

## Usage

### Interactive mode

```sh
sudo ./build/net
```

The `net>` prompt accepts commands with tab-completion and history.

### Single command

```sh
sudo ./build/net -c "show interfaces"
```

### Generate configuration

Export the running network state as replayable CLI commands:

```sh
./build/net -g > network-config.txt
```

### Read commands from STDIN

```sh
# From a file
sudo ./build/net < config.txt

# From a pipe
cat config.txt | sudo ./build/net

# Round-trip: capture and restore
./build/net -g > backup.txt
sudo ./build/net < backup.txt
```

Blank lines and lines starting with `#` are skipped.

### Command-line flags

| Flag | Long | Description |
|------|------|-------------|
| `-c CMD` | `--command` | Execute a single command and exit |
| `-g` | `--generate` | Print configuration commands for the running system |
| `-i` | `--interactive` | Enter interactive mode (default) |
| `-h` | `--help` | Show help |

## Command Reference

### show

```
show interface [name <name>] [type <type>] [group <group>]
show routes    [vrf <number>]
show arp       [ip <address>] [interface <name>]
show ndp       [ip <address>] [interface <name>]
```

**Interface types:** `bridge`, `carp`, `ethernet`, `lagg`, `loopback`, `tap`,
`tunnel`, `virtual`, `vlan`, `wlan`

### set

```
set interface name <name> [type <type>]
    [inet|inet6 address <addr/prefix>]
    [mtu <bytes>] [vrf <num>] [group <name>]
    [status up|down]
    # Bridge options
    [member <iface> ...]
    # LAGG options
    [member <iface> ...] [protocol failover|lacp|loadbalance|roundrobin|none]
    # VLAN options
    [vid <id>] [parent <iface>]
    # Tunnel options
    [source <ip>] [destination <ip>] [tunnel-vrf <num>]
    # WLAN options
    [ssid <name>] [wlandev <parent>]

set route [protocol static] dest <prefix>
    [nexthop <ip>] [interface <iface>] [vrf <num>]
    [blackhole|reject]

set arp ip <address> mac <mac> [interface <name>] [permanent|temp] [pub]
set ndp ip <address> mac <mac> [interface <name>] [permanent|temp]

set vrf fibs <count>
```

### delete

```
delete interface name <name> [inet|inet6 address <addr/prefix>] [group <name>]

delete route [protocol static] dest <prefix> [nexthop <ip>] [vrf <num>]

delete arp ip <address> [interface <name>]
delete ndp ip <address> [interface <name>]

delete vrf name <name>
```

## Examples

### Show all interfaces

```sh
./build/net -c "show interface"
```

```
Flags: U=UP, B=BROADCAST, D=DEBUG, L=LOOPBACK, P=POINTOPOINT,
       e=NEEDSEPOCH, R=RUNNING, N=NOARP, O=PROMISC, p=PPROMISC,
       A=ALLMULTI, a=PALLMULTI, M=MULTICAST, s=SIMPLEX, q=OACTIVE,
       0/1/2=LINK0/1/2, C=CANTCONFIG, m=MONITOR, x=DYING, z=RENAMING

Index Interface Group  Type     Address              Status       MTU VRF Flags
----- --------- ------ -------- -------------------- ---------- ----- --- -----
1     re0       phys   Ethernet -                    active      1500   0 UBRMs
2     lo0       lo     Loopback ::1/128              active     16384   0 ULRM
                                fe80::1/64
                                127.0.0.1/8
3     re0.25    vlan   VLAN     10.1.0.21/18         active      1500   0 UBRMs
4     bridge0   bridge Bridge   -                    active      1500   3 UBRMs
5     epair0a   epair  Virtual  -                    active      1500   2 UBRMs
```

### Filter by type

```sh
./build/net -c "show interface type virtual"
```

```
peer_a   VRF  MTU Status Flags peer_b   VRF  MTU Status Flags
-------- --- ---- ------ ----- -------- --- ---- ------ -----
epair0a    2 1500     UP UBRMs epair0b    0 1500   DOWN BRMs
```

### Filter by group

```sh
./build/net -c "show interface group epair"
```

### Configure an address

```sh
sudo ./build/net -c "set interface name epair0b inet address 192.0.0.2/31"
sudo ./build/net -c "set interface name lo1 inet6 address 2001:db8::1/64"
```

### Remove an address

```sh
sudo ./build/net -c "delete interface name epair0b inet address 192.0.0.2/31"
```

### Routing

```sh
# Show routes (default VRF)
./build/net -c "show routes"

# Show routes in VRF 2
./build/net -c "show routes vrf 2"

# Add a static route
sudo ./build/net -c "set route protocol static dest 192.168.52.0/24 nexthop 10.1.0.1 interface re0.25"

# Add a blackhole route
sudo ./build/net -c "set route protocol static dest 192.168.100.0/24 nexthop reject vrf 2"

# Delete a route
sudo ./build/net -c "delete route protocol static dest 192.168.52.0/24 nexthop 10.1.0.1"
```

### ARP and NDP

```sh
# Show ARP cache
./build/net -c "show arp"

# Add a static ARP entry
sudo ./build/net -c "set arp ip 10.1.0.50 mac 00:11:22:33:44:55 interface re0.25 permanent"

# Delete an ARP entry
sudo ./build/net -c "delete arp ip 10.1.0.50"

# Show NDP cache
./build/net -c "show ndp"

# Add a static NDP entry
sudo ./build/net -c "set ndp ip fe80::1234 mac 00:11:22:33:44:55 interface re0 permanent"
```

### Configuration backup and restore

```sh
# Capture the running config
./build/net -g > backup-$(date +%Y%m%d).txt

# Restore
sudo ./build/net < backup-20260208.txt
```

Example generated output:

```
set vrf fibs 256
set interface name lo0 type loopback inet6 address ::1/128 mtu 16384 status up
set interface name lo0 type loopback inet address 127.0.0.1/8
set interface name epair0a type epair vrf 2 status up
set interface name re0 mtu 1500 status up
set interface name bridge0 type bridge member epair1a status up
set interface name re0.25 type vlan inet address 10.1.0.21/18 vid 25 parent re0 status up
set route protocol static dest 0.0.0.0/0 nexthop 10.1.0.1 interface re0.25
```

## Troubleshooting

**Permission denied** — Most `set` and `delete` commands require root. Run
with `sudo`.

**Interface not found** — Verify with `show interface` first.

**Build errors** — Ensure CMake 3.25+ and a C++26-capable compiler are
installed. On FreeBSD: `pkg install cmake llvm19`.

```sh
rm -rf build && cmake -S . -B build && cmake --build build
```

## License

BSD 2-Clause. See [LICENSE](LICENSE).

## See Also

`net` combines the functionality of several FreeBSD utilities into one
consistent CLI:

- ifconfig(8), route(8), arp(8), ndp(8), netstat(1)
