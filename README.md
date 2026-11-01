# Explosives Library

An experimental shared explosion registry for HellForge mods. It lets cooperating mods create private explosion definitions and connect them to HellForge's private damage definitions, instead of changing a native explosion used by other weapons.

**Version:** 0.1.0 · **API ABI:** 1 · **HellForge SDK:** 15 or later · **Supported game build:** 25480438 · **Platform:** Windows x64

## For players

1. Download [Explosives Library 0.1.0](downloads/explosives-library-0.1.0.zip), import it into HellForge, and enable it.
2. Enable a mod that explicitly supports Explosives Library. Installing the library alone adds no weapons or stratagems and changes no native damage values.
3. Restart the game after changing the library or its consuming mods.

The supplied OPS debug consumer waits for the library to become ready, so its position above or below the library in the mod list does not matter. Other consumers must implement the same readiness handling. Keep only one library provider enabled.

The library extends the running game's explosion table. It does not edit game files on disk. Do not enable the older Private Explosion Lab alongside it: both attempt to own that table extension.

## For authors

Copy [`include/explosives_library.h`](include/explosives_library.h) into your mod's source. Use HellForge's `add_damage` and `add_projectile` during mod initialization. After `HF_EVENT_PROJECTILES_READY`, discover the shared provider with `ElFind()` from a game-thread callback, register an explosion, and connect the returned ID to your private projectile or explosive component.

Do not copy `library.c` or `registry_patch.h` into each consuming mod. Install one provider and share its registry.

- [Author guide](docs/AUTHOR-GUIDE.md)
- [API reference](docs/API.md)
- [Troubleshooting and validation](docs/TESTING.md)
- [Standalone consumer example](docs/examples/consumer.c.txt)
- [HellForge page text](HELLFORGE-PAGE.md)
- [Changelog](CHANGELOG.md)

## Current limits

There are **64 private explosion slots shared by all consumers**. Registration is session-local and definitions are immutable. Numeric IDs must never be hard-coded or carried across sessions.

The current API can override the private damage link, three radii, and shrapnel count. Other explosion data, including explosion effects and audio, is inherited from the native donor. It does not expose independent explosion FX/audio selection, invent new status mechanics, or automatically alter projectile flight or weapon firing sounds.

Damage registration uses HellForge's existing API; this library supplies the missing shared explosion registry. Native donors are copied from their loaded state. Edits applied to a donor before cloning may influence that copy; independence does not mean a guaranteed pristine vanilla snapshot. Opaque asset/auxiliary references remain shared.

Game updates require validation of the engine integration. This version checks the game build and engine instructions and refuses unsupported table layouts. Restart if initialization fails; do not unload or hot-reload the provider in a running session. Multiplayer compatibility and simultaneous use by independently authored consumer mods have not been validated.

## Testing status

Private explosion and damage handling was first tested through Impact grenade and Crossbow overwrites in a separate lab. The shared library was then exercised through an OPS overwrite: registration, private shell routing, and an explosion damage event near the impact were recorded. These are targeted tests, not proof that every explosion donor or delivery system works.

Source and public header are available under the [MIT license](LICENSE).

## Build and test from this repository

Use Windows x64 and a Zig C compiler. The checked build used Zig 0.17.0. HellForge SDK 15 headers are vendored under their MIT-0 license; see [third-party notices](THIRD-PARTY-NOTICES.md).

```powershell
./scripts/build.ps1 -Zig 'C:/path/to/zig.exe' -Test
python ./scripts/package.py
```

The build script compiles the provider, the [OPS debug consumer](examples/ops-debug), and the standalone documentation example. With `-Test`, it also runs the registry, OPS routing, and real DLL-discovery harnesses. Caches and outputs stay under `build/` in this checkout. `package.py` writes importable source ZIPs under `dist/`; a compiled DLL is not required in the HellForge package.

The checked-in `downloads/` ZIP is the initial documented 0.1.0 release. Future source changes require rebuilding and versioning the download; it does not automatically follow commits. Do not import the entire repository as a HellForge mod: use the packaged ZIP, which excludes tests and example source from the provider.

The OPS debug consumer is an optional test overwrite, not part of normal installation. Build/package it separately and disable it after testing. This repository does not include game files, personal runtime logs, or credentials.
