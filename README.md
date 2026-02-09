# net

A command-line network management utility for BSD systems. Provides an interactive shell and direct command execution for managing network interfaces, addresses, routes, ARP/NDP caches, VRFs, bridges, VLANs, LAGGs, and tunnels.

## Features

- **Interface Management**: View and configure Ethernet, loopback, bridge, LAGG, VLAN, tunnel, and virtual (epair) interfaces
- **Address Management**: Add/remove IPv4 and IPv6 addresses with prefix lengths
- **Routing**: Display and manipulate routing tables across multiple VRFs
- **ARP/NDP**: Show and manage neighbor caches (IPv4 ARP and IPv6 NDP)
- **VRF Support**: Multi-VRF routing tables and interface assignment
- **Configuration Export**: Generate reproducible configuration commands from live system state
- **Interactive Shell**: Tab-completion and command history via libedit
- **Direct Execution**: Run single commands via `-c` flag without entering shell

## Overview

`net` is a network management tool designed for BSD systems that provides an intuitive command-line interface for viewing and modifying network configuration. It supports both interactive mode with a persistent shell and non-interactive mode for scripting.

## Prerequisites

- BSD operating system (FreeBSD tested)
- C++17 compatible compiler (g++/clang++)
- CMake 3.13 or newer
- libedit (for command-line editing and history)
- Root privileges required for configuration changes

## Building

1. **Configure** the build (out-of-source recommended):

```bash
cmake -S . -B build
```

2. **Build** the project:

```bash
cmake --build build -- -j$(sysctl -n hw.ncpu)
```

This produces the `net` executable at `build/net`.

3. **Install** (optional):

```bash
sudo cmake --install build
```

## Usage

### Interactive Mode

Launch the interactive shell:

```bash
net
```

The prompt `net>` accepts commands with tab-completion and history navigation.

### Direct Command Mode

Execute a single command (pass command as positional arguments):

```bash
net show interface
```

### Reading from STDIN

Commands can be read from a file or pipe:

```bash
# From a file
sudo net < config.txt

# From a pipe
cat config.txt | sudo net

# From command output
net -g | sudo net
```

Empty lines and lines starting with `#` are treated as comments and skipped.

### Configuration Generation

Export current network state as CLI commands:

```bash
net -g > network-config.txt
```

## Command Reference

### Show Commands

```text
show interface [name <name>] [type <type>] [group <group>]
show routes [vrf <number>]
show arp [ip <address>] [interface <name>]
show ndp [ip <address>] [interface <name>]
```

### Set Commands

```text
set interface name <name> [type <type>] [inet|inet6 address <addr/prefix>] [mtu <bytes>] [vrf <num>] [status up|down]
set route [protocol static] dest <prefix> [nexthop <ip>] [interface <iface>] [vrf <num>] [blackhole|reject]
set arp ip <address> mac <mac> [interface <name>] [permanent|temp] [pub]
set ndp ip <address> mac <mac> [interface <name>] [permanent|temp]
set vrf fibs <count>
```

### Delete Commands

```text
delete interface name <name> [inet|inet6 address <addr/prefix>]
delete route dest <prefix> [nexthop <ip>] [vrf <num>]
delete arp ip <address> [interface <name>]
delete ndp ip <address> [interface <name>]
```

### Examples

### Show All Interfaces

```bash
net show interface
```

Output:
```text
Flags: U=UP, B=BROADCAST, D=DEBUG, L=LOOPBACK, P=POINTOPOINT,
       e=NEEDSEPOCH, R=RUNNING, N=NOARP, O=PROMISC, p=PPROMISC,
       A=ALLMULTI, a=PALLMULTI, M=MULTICAST, s=SIMPLEX, q=OACTIVE,
       0/1/2=LINK0/1/2, C=CANTCONFIG, m=MONITOR, x=DYING, z=RENAMING

Index Interface Group  Type          Address       Status   MTU VRF Flags
----- --------- ------ ------------- ------------- ------  ---- --- -----
1     re0       -      Ethernet      -             active  1500   0 UBRMs
2     lo0       lo     Loopback      ::1/128       active 16384   0 ULRM 
                                     fe80::1/64                         
                                     127.0.0.1/8                        
3     re0.25    vlan   VLAN          10.1.0.21/18  active  1500   0 UBRMs
4     bridge0   bridge Bridge        -             down    1500   0 BMs  
5     gif0      gif    GenericTunnel -             down    1280   2 PM   
6     lagg0     lagg   LinkAggregate -             down    1500   2 BMs  
7     lo1       lo     Loopback      fe80::1/64    active 16384   0 ULRM 
8     epair14a  epair  Virtual       -             active  1500   0 BRMs 
9     epair14b  epair  Virtual       192.0.0.2/31  active  1500   2 UBRMs
```

### Filter by Type

Show only virtual (epair) interfaces:

```bash
net show interface type virtual
```

Output:
```text
peer_a   VRF  MTU Status Flags peer_b   VRF  MTU Status Flags
-------- --- ---- ------ ----- -------- --- ---- ------ -----
epair14a   0 1500   DOWN BRMs  epair14b   2 1500     UP UBRMs
```

### Filter by Group

Show interfaces in a specific group:

```bash
net show interface group epair
```

### Managing Addresses

Add an IPv4 address to an interface (requires root):

```bash
sudo net set interface name epair14b inet address 192.0.0.8/31
```

Add an IPv6 address:

```bash
sudo net set interface name lo1 inet6 address 2001:db8::1/64
```

Delete an address from an interface (requires root):

```bash
sudo net
net> delete interface name epair14b inet address 192.0.0.4/31
```

**Note:** Write operations require root privileges. Without root access:

```text
net> delete interface name epair14b inet address 192.0.0.4/31
delete interface: failed to remove address '192.0.0.4/31': Operation not permitted
```

### Routing Tables

Show routes in default VRF:

```bash
net show routes
```

Output:
```text
Routes (VRF: 0)

Flags: U=up, G=gateway, H=host, S=static, B=blackhole, R=reject

Destination       Gateway  Interface Flags Scope Expire
----------------- -------- --------- ----- ----- ------
0.0.0.0/0         10.1.0.1 re0.25    UGS   -     -     
10.1.0.0/18       link#3   re0.25    U     -     -     
127.0.0.1/32      link#2   lo0       UH    -     -     
```

Show routes in specific VRF:

```bash
net show routes vrf 2
```

Add a static route (requires root):

```bash
sudo net set route protocol static dest 192.168.52.0/24 nexthop 10.1.0.1 interface re0.25
```

Add a blackhole/reject route (requires root):

```bash
sudo net set route protocol static dest 192.168.100.0/24 nexthop reject vrf 2
```

Delete a route (requires root):

```bash
sudo net delete route protocol static dest 192.168.52.0/24 nexthop 10.1.0.1
```

### ARP and NDP Management

Show ARP cache:

```bash
net show arp
```

Output:
```text
IP Address    MAC Address       Interface Expire   Flags
------------- ----------------- --------- -------- -----
10.1.0.1      aa:bb:cc:dd:ee:ff re0.25    23:59:45 -    
10.1.0.50     00:11:22:33:44:55 re0.25    -        P    
```

Show NDP (IPv6 neighbor discovery) cache:

```bash
net show ndp
```

Add a static ARP entry (requires root):

```bash
sudo net set arp ip 10.1.0.50 mac 00:11:22:33:44:55 interface re0.25 permanent
```

Add a static NDP entry (requires root):

```bash
sudo net set ndp ip fe80::1234 mac 00:11:22:33:44:55 interface re0 permanent
```

Delete an ARP entry (requires root):

```bash
sudo net delete arp ip 10.1.0.50
```

## Configuration Generation

The `-g` flag generates a complete set of CLI commands that reproduce the current network configuration. This is useful for persisting the router configuration state across reboots.

Generate configuration:

```bash
net -g > network-config.txt
```

Example output:

```text
set vrf fibs 255
set interface name lo0 type loopback inet6 address ::1/128 mtu 16384 status up
set interface name lo0 type loopback inet address 127.0.0.1/8
set interface name epair14b type epair vrf 2 inet address 192.0.0.2/31 status up
set interface name re0 mtu 1500 status up
set interface name bridge0 type bridge member epair0a member epair1a status up
set interface name lagg0 type lagg vrf 2 member epair0b member epair1b status up
set interface name re0.25 type vlan inet address 10.1.0.21/18 vid 25 parent re0 status up
set interface name gif0 type tunnel vrf 2 source 192.0.0.0 destination 192.0.0.1 tunnel-vrf 3 status down
set route protocol static dest 0.0.0.0/0 nexthop 10.1.0.1 interface re0.25
set route protocol static dest 10.1.0.0/18 nexthop-interface re0.25
```

The generated commands can be saved to a file and later replayed to restore configuration:

```bash
# Generate and save configuration
net -g > backup-$(date +%Y%m%d).txt

# Restore configuration by piping to STDIN
sudo net < backup-20260207.txt

# Or using cat
cat backup-20260207.txt | sudo net
```

**Note:** When commands are read from STDIN (via pipe or file redirection), empty lines and lines starting with `#` are automatically skipped as comments.

## Architecture

### BSD System Calls Used

- `getifaddrs()`: Retrieve interface list and addresses
- `ioctl(SIOCGIFFLAGS)`: Get interface flags and status
- `ioctl(SIOCAIFADDR)`: Add addresses to interfaces
- `ioctl(SIOCDIFADDR)`: Delete addresses from interfaces
- `sysctl(NET_RT_DUMP)`: Retrieve routing tables
- `sysctl(NET_RT_FLAGS)`: Retrieve ARP/NDP neighbor caches
- Routing socket messages (RTM_ADD, RTM_DELETE): Add/delete routes

## License

See [LICENSE](LICENSE) file for details.

## Contributing

This project follows BSD coding conventions. When contributing:

1. Match existing code style (tabs for indentation)
2. Add appropriate error handling and validation
3. Update relevant table formatters for new features
4. Test on FreeBSD before submitting
5. Document new commands in this README

## Platform Support

Currently tested on FreeBSD 15+. The code uses BSD-specific APIs (`ioctl`, `sysctl`, routing sockets) and is not yet been ported to any other platform.

## Troubleshooting

### Permission Denied Errors

Most configuration changes require root privileges. Run with `sudo`:

```bash
sudo net -c "set interface name em0 inet address 192.168.1.10/24"
```

### Interface Not Found

Verify the interface exists:

```bash
net -c "show interface"
```

### Build Errors

Ensure all dependencies are installed:

```bash
# FreeBSD
pkg install cmake
```

Clean and rebuild:

```bash
rm -rf build/
cmake -S . -B build
cmake --build build
```

## TODO

### Stelleri NETCONF

A full-featured NETCONF implementation (non-lite version) is planned for future development. This will provide standardized network management protocol support for integration with enterprise management systems.

The only impediment to starting this was getting libyang to function properly with the ietf-network-instance schema, and that item can now be crossed off the list—it's been realized at [libyang-ni-example](https://github.com/Ravenhammer-Research/libyang-ni-example) thanks to the CESNET/libyang developers.

### Planned Features for Lite Version

The following features are planned for the `net` utility:

- **FIB Algorithm Configuration** (Medium Priority): Configure and display FIB (Forwarding Information Base) lookup algorithms specific to routing table lookups. FreeBSD supports multiple routing lookup algorithms optimized for different table sizes and use cases:
  - `radix` - Base system radix backend
  - `bsearch` - Lockless binary search for small FIBs (<16 routes, IPv4 only)
  - `radix_lockless` - Lockless immutable radix for small FIBs (<1000 routes)
  - `dpdk_lpm` - DPDK DIR24-8 lookups optimized for large FIBs (requires dpdk_lpm4.ko/dpdk_lpm6.ko modules)
  - `dxr` - IPv4 only, compressed lookup structure optimized for large BGP FIBs (requires fib_dxr.ko module)
  
  Algorithms are selected automatically based on routing table size, but manual configuration will allow tuning for specific workloads and performance requirements.

The following features are slated as lowest priority but open to discussion:

- **Switch Management** (`set/show/delete switch`): Configure and display switch settings via etherswitchcfg(8) and etherswitch(4) APIs. Devices with this type of hardware are where Stelleri really shines, enabling them to function as both L2 and L3 switches with management of hardware switch features like VLANs, port settings, and switch fabric configuration.

- **Netgraph Support** (`set/show/delete ng`): Integration with netgraph(4) for advanced network graph configuration. Relevance is still being evaluated, but netgraph's flexibility may prove useful for complex network topologies.

- **Firewall Management** (`set/show/delete fw`): Firewall rule management using the same kernel API as ipfw. This will provide a consistent CLI syntax for firewall configuration alongside other network settings.

- Intel SGX

## See Also

This tool combines the functionality of several BSD utilities into a cohesive configuration syntax that can be easily maintained:

- `ifconfig(8)` - BSD interface configuration utility
- `route(8)` - BSD routing table manipulation
- `arp(8)` - BSD ARP cache management
- `ndp(8)` - BSD IPv6 neighbor discovery protocol management
- `netstat(1)` - Network statistics
