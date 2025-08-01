# Ancible

A high-performance, C-based reimplementation of [Ansible](https://www.redhat.com/en/ansible-collaborative). It mirrors Ansible's declarative, YAML-based playbook interface, but re-engineers the backend for raw speed and minimal dependencies.

## Features

- **High Performance**: Pure C implementation for maximum speed and efficiency
- **Minimal Dependencies**: Lightweight design with few external dependencies
- **Compatible Interface**: Uses the same YAML playbook format as Ansible
- **Inventory Management**: Supports INI-style inventory files with groups
- **Flexible Execution**: Run commands locally or remotely via SSH
- **Module System**: Extensible module architecture (currently supports command/shell)
- **State Tracking**: Maintains execution state and results in JSON format
- **Cross-Platform**: Works on Linux, macOS, and other Unix-like systems
- **Fast Startup**: No Python interpreter overhead, instant execution

## Building

```bash
make
```

## Running

```bash
./bin/ancible-playbook -i inventory.ini playbook.yml
```

### Options

- `--help`: Display help message
- `-v, --verbose`: Increase verbosity
- `-c, --color`: Enable Colored output 
- `-i INVENTORY`: Specify inventory file (default: ./inventory.ini)

### Example Playbooks

The `examples/playbooks/` directory contains several sample playbooks:

- `1_simple.yml` - Basic system commands (hostname, whoami, date)
- `2_multiple_tasks.yml` - Multiple system information tasks
- `3_file_operations.yml` - File creation and manipulation
- `4_cpu_intense.yml` - CPU-intensive operations
- `5_network_operations.yml` - Network connectivity checks
- `6_system_admin.yml` - System administration tasks
- `7_security_checks.yml` - Security auditing operations
- `8_database_operations.yml` - Database-related tasks
- `9_conditions.yml` - When Conditions in Playbooks

Run an example with:

```bash
./bin/ancible-playbook -i examples/inventory_local.ini examples/playbooks/1_simple.yml
```

## Project Structure

```
ancible/
├── bin/                      # Compiled executables
├── cli/                      # Command-line interface code
│   ├── args.c                # - Command-line argument parsing
│   └── main.c                # - Main entry point
├── core/                     # Core engine components
│   ├── context.c             # - Execution context management
│   ├── condition.c           # - Condition engine
│   ├── executor.c            # - Task execution engine
│   ├── inventory.c           # - Host inventory parser
│   ├── parser.c              # - YAML playbook parser
│   └── state.c               # - Runtime state management
├── examples/                 # Example playbooks and inventory files
│   ├── inventory.ini         # - Sample multi-host inventory
│   ├── inventory_local.ini   # - Local-only inventory
│   └── playbooks/            # - Example playbook collection
├── include/                  # Header files
│   ├── ancible.h             # - Main header
│   ├── cli/                  # - CLI headers
│   ├── core/                 # - Core engine headers
│   ├── modules/              # - Module system headers
│   └── transport/            # - Transport layer headers
├── modules/                  # Module implementations
│   ├── command.c             # - Command module
│   └── module.c              # - Module system core
├── runtime/state/            # Runtime state storage Per-Host
├── tests/unit/               # Unit tests
└── transport/                # Transport implementations
    ├── runner.c              # - Command execution abstraction
    └── ssh.c                 # - SSH transport
```

## Testing

```bash
make test
```

## Performance

Ancible has been benchmarked against Ansible for various playbooks. The results show significant performance improvements across different types of tasks.

| Playbook                                                                    | Ancible | Ansible | 
|-----------------------------------------------------------------------------|---------|---------| 
| [#1](./examples/playbooks/1_simple.yml) -               Simple Echo         | 0.113s  | 1.454s  | 
| [#2](./examples/playbooks/2_multiple_tasks.yml) -       Multiple Tasks      | 0.115s  | 1.897s  | 
| [#3](./examples/playbooks/3_file_operations.yml) -      File Operations     | 0.141s  | 2.087s  | 
| [#4](./examples/playbooks/4_cpu_intense.yml) -          CPU Intense Tasks   | 0.363s  | 2.126s  | 
| [#5](./examples/playbooks/5_network_operations.yml) -   Network Operations  | 2.094s  | 4.186s  | 
| [#6](./examples/playbooks/6_system_admin.yml) -         System Admin Stuff  | 0.062s  | 1.813s  | 
| [#7](./examples/playbooks/7_security_checks.yml) -      Security Checks     | 0.019s  | 1.157s  | 
| [#8](./examples/playbooks/8_database_operations.yml) -  Database Operations | 0.064s  | 1.509s  | 
| [#9](./examples/playbooks/9_conditions.yml) -           Conditions          | 0.076s  | 2.708s  | 

## Future Development (aka TODO List)

The following features are planned for future implementation, some are necessary to fully replace Ansible's functionality, while others are more like an incentive to gain more features and improve the user experience.

### Core Features

Those features are necessary to run playbooks, they are not strictly necessary to run the `command` module, but they are needed to fully replace Ansible's functionality.

- [x] Execute Basic Playbooks
- [x] Conditional Execution: Support for `when` conditionals
- [ ] Blocks: Support for task grouping and error handling with blocks
- [ ] Variable Registration: Support for `register` to capture command output

### Additional Modules

While using the `command` module, you can run any command on the remote host, you can copy, use git or create files. Thereby we only need them to fully replace Ansible's functionality, but they are not strictly necessary to run playbooks, since the `command` module can execute any command. However, you can see this as an incentive to implement these modules.

- [ ] File module (create, delete, chmod)
- [ ] Copy module
- [ ] Template module
- [ ] Service module
- [ ] Package module
- [ ] Git module

### Infrastructure

Here are some features that are not strictly necessary to run playbooks, but they would improve the user experience and make Ancible more powerful. Like the Additional Modules, those are more like an incentive to implement them, to fully replace Ansible's functionality.

- [x] Colored Output
- [ ] Better error handling and reporting
- [ ] Support for roles and includes
- [ ] Variable templating with Jinja2-like syntax

## License

MIT
