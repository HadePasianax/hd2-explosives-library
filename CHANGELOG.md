## 0.2.1

- Added a callback-slot fallback adapted from Beelzebub's Armor Transmog 0.1.70. EL still requests a normal HellForge game-thread callback first. If registration is refused, it validates the supported game build's callsite and joins HellForge's shared hook chain for game.dll+0x13270B0.
- The fallback recognizes the frame caller through the stack; the secondary caller is used only after 250 ms without the primary caller. After 600 hook calls it reports whether recognized calls ran the library. No operating-system worker thread is created.
- If game.dll is not loaded when registration is refused, fallback installation is deferred until GAME_LOADED. Unsupported callsites or failed hook installation are logged and refused.
- ABI 1, registration API, explosion capacity and editor integration are unchanged. Existing 0.1.0/0.2.0 consumers remain compatible.
- Offline fallback policy, registration, provider and editor tests pass. In-game callback exhaustion and coexistence with other fallback hooks still need testing.

# 0.2.0

- Adds built-in HellForge Editor compatibility for rendering and editing custom Projectile, Damage Type and EL Explosion records in the existing Advanced categories; no editor replacement.
- Adds explosion-damage views resolving the current payload's private damage record.
- Adds named projectile payload references using namespace/source view labels, translating back to real IDs before writes.
- Preserves per-session custom-field baselines and reset recovery; replays sandbox saved changes so reset markers do not resurrect on reopen.
- Adds write/readback/application diagnostics and optional editor integration configuration.
- Keeps gameplay ABI 1, 64 explosion slots, native-donor FX/audio and existing consumers compatible.
- Removes the binary-specific editor staging probe. Known limitations: OPS Debug staging exception, startup/late registration, unverified multiplayer and persistence across reassigned IDs.

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
