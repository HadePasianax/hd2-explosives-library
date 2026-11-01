# Mod name

Explosives Library

# Short description

A shared framework for HellForge mods to create private explosions with independent damage and blast settings.

# Description & installation

## Explosives Library

Explosives Library gives HellForge mod authors a shared way to create custom explosion definitions and connect them to private damage types. Mods can tune their own blast radius, damage, armor penetration, stagger, and demolition force without changing the native explosion used by another weapon.

It provides one shared explosion registry for compatible mods, with private IDs allocated at runtime. Damage types use HellForge's existing private damage system.

### For players

Install this when another mod lists **Explosives Library** as a requirement. The library alone adds no weapons or stratagems and does not change native damage values.

1. Import the library ZIP into HellForge and enable it.
2. Enable the compatible mod that requires it.
3. Restart the game after changing the library or its consuming mods.

Keep only one copy of the library enabled. The supplied example waits for the library to become ready, so it does not require a particular position in the mod list. Other consuming mods must also handle readiness correctly.

### For mod authors

The download includes a public C header, source code, developer documentation, an API reference, and a standalone consumer example.

The current API supports:

- Private explosion definitions cloned from native donors.
- Independent private damage links and optional blast-radius overrides.
- Optional shrapnel-count overrides for suitable donors.
- A shared registry with 64 explosion slots across compatible mods.
- Namespaced registration keys and duplicate-definition checks.

Use HellForge's private damage rows to set standard and durable damage, armor penetration, demolition, stagger, push, and supported existing status fields. The library handles the engine-side explosion registry; consumers should use its API rather than bundle their own table patches.

Start with `README.md`, then `docs/AUTHOR-GUIDE.md` and `docs/API.md` in the ZIP. The public header is `include/explosives_library.h`.

### Compatibility and current status

**Experimental version 0.1.0. Requires HellForge SDK 15 support and game build 25480438.** Game updates may require a library update.

Private explosion and damage behavior has been tested through grenade and Crossbow overwrites in the precursor lab, followed by a library-backed orbital strike test with logged nearby explosion damage. Multiplayer compatibility and broad compatibility with independently authored consumers have not yet been validated.

Do not enable the older **Private Explosion Lab**, a second library provider, or another mod that independently extends the same native explosion table alongside this library. An unsupported table layout is rejected during initialization. Restart after changing the mod set; hot-unloading is unsupported.

Explosion effects and audio are inherited from the chosen native donor. This version does not expose separate explosion FX/audio selection or introduce new status-effect mechanics. Donors are copied from their loaded state, so earlier edits to a donor can influence the initial copy.

The **OPS Debug** mod is a separate test download, not required for normal use. It overwrites vanilla Orbital Precision Strike with a larger private plasma blast and zero direct-hit damage. Disable it after testing unless you intentionally want that overwrite.

Source and public header are supplied under the MIT license.

# Initial release patch notes

Initial experimental release.

Adds a shared private explosion registry with 64 slots, HellForge private damage integration, a public C API, developer documentation, and a standalone example. Validated through a library-backed OPS test with private blast damage and zero direct-hit damage.
