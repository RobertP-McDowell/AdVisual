#pragma once
#define DEBUG_MODE

#ifdef DEBUG_MODE
#define DBPRINT(p_output) do { cout << p_output << "\n"; } while(0)
// unlike DBPRINT, DBBREAKPOINT's are meant to be temporarily used (with a tool like gdb),
// and should be left out of pr's/commits. Prefer assert otherwise.
//raise(SIGTRAP); // Commenting out for now, for windows. TODO: check for POSIX.
#define DBBREAKPOINT(p_output) do { \
	cout << p_output << "\n"; \
} while(0)
#else
#define DBPRINT(p_output) do {} while(0)
#define DBBREAKPOINT(p_output) do { \
	cerr << p_output << " This breakpoint shouldn't exist in release/shared builds, Breakpoint name: " << "\n"; \
} while(0)
#endif