# Changelog

## 0.1.0

- Initial experimental shared explosion provider, ABI 1, game build 25480438.
- 64 session-local explosion slots with namespaced registration and duplicate-definition checks.
- Private damage links through HellForge's existing damage registry.
- Native descriptor cloning, optional radius and shrapnel-count overrides.
- Guarded engine table relocation, including the kick-stage bound needed for working radial damage.
- Runtime API discovery without relying on a fixed DLL filename.
- Public header, author guide, API reference, troubleshooting guide, and standalone example.
- Separate OPS Debug consumer used to exercise the library through orbital delivery.

Documentation was added after the initial debug packages; runtime code and ABI remain unchanged.
