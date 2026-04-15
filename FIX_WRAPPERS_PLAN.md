# ocaml-solo5 Shell Wrapper Fix Plan

## Problem

The ocaml-solo5 cross-compiler uses shell wrappers with shebang (`#!/bin/sh` or similar) that:
1. Include appended bytecode data
2. Fail in nix sandbox builds because `posix_spawn()` doesn't execute shebangs

## Current Wrapper Format

The OCaml compiler tools (ocamlc, ocamlopt, etc.) are wrapped as shell scripts:
```sh
#!/path/to/ocamlrun
<binary bytecode data appended>
```

This format works on native systems but fails in sandboxed nix builds.

## Solution

Replace shebang-based wrappers with a portable approach using `#!/usr/bin/env`.

### Approach 1: Wrapper Scripts (Preferred)

Replace the current wrapper format with a simple shell script:
```sh
#!/usr/bin/env sh
exec /path/to/ocamlrun "$0.bin" "$@"
```

**Pros:**
- Portable across systems
- Works with nix sandbox
- Simple to implement

**Cons:**
- Adds a small startup overhead

### Approach 2: Direct Native Code

Build the OCaml cross-compiler as native code instead of bytecode.

**Pros:**
- No wrapper needed
- Faster execution

**Cons:**
- May require significant build system changes
- May not be supported by OCaml build system

## Files to Modify

1. **Makefile** - Modify `install-ocaml` target to generate portable wrappers instead of raw bytecode
2. **gen_dot_install.sh** - May need updating if wrapper approach changes

## Implementation Steps

### Step 1: Understand Current Installation

Currently, the Makefile's `install-ocaml` target copies:
- `ocaml/runtime/ocamlrun` - The bytecode interpreter
- Various OCaml stdlib files
- Compiler tools (ocamlc, ocamllex, etc.)

The compiler tools are currently raw bytecode files.

### Step 2: Create Wrapper Template

In the Makefile, generate a wrapper script template:
```makefile
define WRAPPER_TEMPLATE
#!/usr/bin/env sh
exec $(OCAMLRUN_PATH) $(TOOL_NAME).byte "$$@"
endef
```

### Step 3: Modify Install Target

Change `install-ocaml` to wrap the bytecode files:
```makefile
install-ocaml:
    # Create wrappers for each tool
    for tool in ocamlc ocamlopt ocamllex ocamldep; do \
        echo '#!/usr/bin/env sh' > $(MAKECONF_SYSROOT)/bin/$$tool; \
        echo 'exec ocamlrun $(LIBDIR)/$$tool.byte "$$@"' >> ...; \
        chmod +x ...; \
    done
```

### Step 4: Test

Test the changes by:
1. Building ocaml-solo5
2. Running nix build with the updated package
3. Verifying the unikernel builds work

## Compatibility

- This fix should work for all targets: unix, macosx, ahv, hvt, virtio
- The fix is purely at the build/installation level
- No changes needed to the solo5 runtime or tender
- Should be acceptable for upstream (it's a portability fix)

## Notes

- The OCaml bytecode runs on the **solo5 runtime** (the tender), not on the host
- On the host (macOS), we only need the tools to compile code
- The shebang issue is specific to how nix executes these files during build
- The bytecode will run fine on the actual unikernel when deployed

## References

- OCaml bytecode format: https://ocaml.org/manual/intf-c.html
- nix sandbox: https://nixos.org/manual/nix/stable/security/sandboxing
- ocaml-solo5: https://github.com/solo5/ocaml-solo5