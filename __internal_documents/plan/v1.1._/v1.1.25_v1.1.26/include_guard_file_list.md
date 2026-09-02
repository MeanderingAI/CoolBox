# List of Files Updated: #pragma once → include guards

This document lists all files in the codebase where `#pragma once` was replaced with a lower_snake_case `#ifndef`/`#define`/`#endif` include guard.

## Files Updated

- _libraries/packages/ML/bayesian_network_db/headers/bayesian_network.h
- ___DISABLED_binaries/services/__rej_system_monitor/include/system_monitor.h
- ___DISABLED_binaries/abbs/__host_scanner/host_scanner.h
- ___DISABLED_binaries/services/__rej_proxy_service/include/proxy_server.h
- _libraries/packages/CHEMISTRY/include/chemistry/periodic_table.h
- _libraries/packages/ELECTRONICS/circuitry/include/wire.h
- _libraries/packages/ELECTRONICS/circuitry/include/resistor.h
- _libraries/packages/ELECTRONICS/circuitry/include/circuitry.h
- _libraries/packages/ELECTRONICS/circuitry/include/component.h
- _libraries/packages/ELECTRONICS/circuitry/include/circuit_solver.h
- _libraries/packages/ELECTRONICS/circuitry/include/battery.h

<!-- Add more files here as the conversion progresses -->

Each file now uses a unique, lower_snake_case macro for its include guard, replacing the previous `#pragma once` directive.