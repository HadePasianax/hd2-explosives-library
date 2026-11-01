# Mod name

Explosives Library

# Short description

A shared framework for HellForge mods to create private explosions with independent damage and blast settings.

# Description & installation

## Explosives Library

Explosives Library gives HellForge mod authors a shared way to create custom explosion definitions and connect them to private damage types. Mods can tune their own blast radius, damage, armor penetration, stagger, and demolition force without changing the native explosion used by another weapon.

It provides one shared explosion registry for compatible mods, with private IDs allocated at runtime. Damage types use HellForge's existing private damage system.

## For players

Install this when another mod lists **Explosives Library** as a requirement. The library alone adds no weapons or stratagems and does not change native damage values.

1. Import the library ZIP into HellForge and enable it.
2. Enable the compatible mod that requires it.
3. Restart the game after changing the library or its consuming mods.

Keep only one copy of the library enabled. The supplied example waits for the library to become ready, so it does not require a particular position in the mod list. Other consuming mods must also handle readiness correctly.

---

## For developers

Source code, documentation, and examples are available in the [Explosives Library GitHub repository](https://github.com/HadePasianax/hd2-explosives-library).

- [Author guide](https://github.com/HadePasianax/hd2-explosives-library/blob/main/docs/AUTHOR-GUIDE.md) — integrate the library into your mod.
- [API reference](https://github.com/HadePasianax/hd2-explosives-library/blob/main/docs/API.md) — registration, settings, and API requirements.
- [Public C header](https://github.com/HadePasianax/hd2-explosives-library/blob/main/include/explosives_library.h) — include this in your consumer mod.
- [Examples](https://github.com/HadePasianax/hd2-explosives-library/tree/main/examples) — includes the orbital strike debug consumer. The author guide also links a standalone Crossbow example.

The library supplies a shared private explosion registry; damage types use HellForge's existing private damage API. Use the shared provider rather than copying its engine patches into each mod.

Use a [versioned release](https://github.com/HadePasianax/hd2-explosives-library/releases) for downloads, and read the documentation at the matching release tag when targeting an older version. The links above follow the current `main` branch. Report bugs through [GitHub Issues](https://github.com/HadePasianax/hd2-explosives-library/issues).

## Compatibility and current status

**Experimental version 0.1.0. Requires HellForge SDK 15 support and game build 25480438.** Game updates may require a library update.

Private explosion and damage behavior has been tested through grenade and Crossbow overwrites in the precursor lab, followed by a library-backed orbital strike test with logged nearby explosion damage. Multiplayer compatibility and broad compatibility with independently authored consumers have not yet been validated.

Do not enable the older a second library provider, or another mod that independently extends the same native explosion table alongside this library. An unsupported table layout is rejected during initialization. Restart after changing the mod set; hot-unloading is unsupported.

Explosion effects and audio are inherited from the chosen native donor. This version does not expose separate explosion FX/audio selection or introduce new status-effect mechanics. Donors are copied from their loaded state, so earlier edits to a donor can influence the initial copy.

The **OPS Debug** mod is a separate test download to show the library in action, not required for normal use. It overwrites vanilla Orbital Precision Strike with a larger private plasma blast and zero direct-hit damage. Disable it after testing unless you intentionally want that overwrite.

## AI disclaimer
GPT-6.1 Sol assisted with research, implementation, debugging and documentation of this tool.

Source and public header are supplied under the MIT license.

# Initial release patch notes

Initial experimental release.

Adds a shared private explosion registry with 64 slots, HellForge private damage integration, a public C API, developer documentation, and a standalone example. Validated through a library-backed OPS test with private blast damage and zero direct-hit damage.
