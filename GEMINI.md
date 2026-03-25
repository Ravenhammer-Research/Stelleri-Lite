# Stelleri Project Context

You are working on **Stelleri**, a networking configuration and management utility for FreeBSD. It provides a CLI and a server-client architecture for managing network interfaces, routing, ARP, NDP, and other networking subsystems using YANG models and Netconf principles.

## Tech Stack
- **Language:** C++26
- **Build System:** CMake
- **OS Target:** FreeBSD
- **Architecture:**
    - `include/`: Header files defining the core logic, configurations, and interfaces.
    - `src/`: Implementation files.
    - `src/executive/`: Command execution logic for Set, Delete, Show, and Commit operations.
    - `src/data/`: Data models and structures.
    - `src/parser/`: CLI command parsing.
    - `src/generator/`: Logic for generating system commands or configuration files.
    - `/usr/local/include/libyang`: libyang
    - `/usr/local/include/libnetconf2`: libnetconf2
    - `/usr/local/share/yang/modules/yang/standard/ietf/RFC`: IETF Yang schemas
    - `/usr/local/share/yang/modules/yang/standard/iana`: IANA Yang schemas

## Key Conventions
- **Header-Only where appropriate:** Many configuration structures are defined in headers.
- **YANG Integration:** The project uses YANG for data modeling.
- **Surgical Updates:** When modifying `Execute...` files, ensure you maintain the dispatch logic and error handling consistent with the existing codebase.
- **Testing:** Ensure that networking changes are validated against FreeBSD's expected behavior.

## Workflow Mandates
- Always check `include/` for the corresponding configuration class before modifying implementation logic.
- Adhere to the `.clang-format` file for all code changes.
- When adding new interface types, update the `InterfaceTypeDispatch` and relevant `Generate...Commands` logic.
