Explosives Library gives HellForge mod authors a shared way to create custom explosion definitions and connect them to private damage types. Mods can tune their own blast radius, damage, armor penetration, stagger, and demolition force without changing the native explosion used by another weapon.

It provides one shared explosion registry for compatible mods, with private IDs allocated at runtime. Damage types use HellForge's existing private damage system.

Version **0.2.0** also adds compatibility with HellForge's **built-in editor**, allowing custom **Explosion, Projectile, and Damage Type IDs** to be displayed and their supported fields edited in the existing Advanced categories. No replacement editor is required.

## For players

Install this when another mod lists **Explosives Library** as a requirement. The library alone adds no weapons or stratagems and does not change native damage values.

1. Import the library ZIP into HellForge and enable it.
2. Enable the compatible mod that requires it.
3. Restart the game after changing the library or its consuming mods.

Keep only one copy of the library enabled. The supplied example waits for the library to become ready, so its gameplay does not require a particular position in the mod list. Other consuming mods must also handle readiness correctly. For editor integration, keep EL before the editor where ordering is configurable: custom records must be registered before the editor reads its catalogue.

To edit custom records, open **F8 > Advanced > Projectiles / Damage Types / Explosions** and search their namespace/source name. Confirm an input with **Enter/Tab**, then **APPLY/F9**. Resets also need APPLY. Projectile payload references use named runtime views; shared explosion or damage records affect every projectile that uses them.

Editor integration can be disabled with `[editor] enabled=0` in `explosives-library.ini`, followed by a restart. The explosion registry remains available.

---

## For developers

Source code, documentation, and examples are available in the [Explosives Library GitHub repository](https://github.com/HadePasianax/hd2-explosives-library).

- [Author guide](https://github.com/HadePasianax/hd2-explosives-library/blob/main/docs/AUTHOR-GUIDE.md) — integrate the library into your mod.
- [API reference](https://github.com/HadePasianax/hd2-explosives-library/blob/main/docs/API.md) — registration, settings, and API requirements.
- [Public C header](https://github.com/HadePasianax/hd2-explosives-library/blob/main/include/explosives_library.h) — include this in your consumer mod.
- [Examples](https://github.com/HadePasianax/hd2-explosives-library/tree/main/examples) — includes the orbital strike debug consumer. The author guide also links a standalone Crossbow example.
- [Editor guide](https://github.com/HadePasianax/hd2-explosives-library/blob/main/docs/EDITOR.md) — custom-record editing, troubleshooting, and current limitations.

The library supplies a shared private explosion registry; damage types use HellForge's existing private damage API. Use the shared provider rather than copying its engine patches into each mod. Gameplay **ABI 1 remains compatible** with existing consumers. Use the IDs returned by the API rather than hard-coding them. The shared pool holds **64 unique private explosions**; firing an existing definition repeatedly uses no additional slots.

Use a [versioned release](https://github.com/HadePasianax/hd2-explosives-library/releases) for downloads, and read the documentation at the matching release tag when targeting an older version. The links above follow the current `main` branch. Report bugs through [GitHub Issues](https://github.com/HadePasianax/hd2-explosives-library/issues).

## Compatibility and current status

**Version 0.2.0. Requires HellForge SDK 15 support and game build 25480438.** Game updates may require a library update.

Private explosion and damage behavior has been tested through grenade and Crossbow overwrites in the precursor lab, followed by a library-backed orbital strike test with logged nearby explosion damage. Custom-record rendering and editing have also worked in user testing with other mods. Multiplayer compatibility, persistence after numeric IDs are reassigned, and every startup configuration have not been validated. The new named payload references have passed offline tests but still need in-game confirmation.

Do not enable a second library provider, an older **Private Explosion Lab**, or another mod that independently extends the same native explosion table alongside this library. An unsupported table layout is rejected during initialization. Restart after changing the mod set; hot-unloading is unsupported.

Editor discovery captures custom projectile and damage records registered before its catalogue is read; later records may be absent. EL explosion views resolve impact payloads at runtime and expose radii and linked damage, rather than every native explosion field. Dedicated expiry-explosion edit views are not included. Radius edits must maintain **inner <= outer <= shockwave**. Consumers may reapply their own profiles, so recheck values after changing their settings or the mod set.

Explosion effects and audio are inherited from the chosen native donor. This version does not expose separate explosion FX/audio selection or introduce new status-effect mechanics. Donors are copied from their loaded state, so earlier edits to a donor can influence the initial copy.

The **OPS Debug** mod on GitHub is a separate test download to show the library in action, not required for normal use. It overwrites vanilla Orbital Precision Strike with a larger private plasma blast and zero direct-hit damage. It also has a reported, unresolved editor-staging issue where an edit can disappear before APPLY. Disable it after testing unless you intentionally want that overwrite.

## AI disclaimer

GPT-6.1 Sol assisted with research, implementation, debugging and documentation of this tool.
