# Explosives Library

**0.3.0 | Gameplay API ABI 1 | HellForge 0.4.0 / SDK 16 | game build 25480438 | Windows x64**

Explosives Library provides shared private explosion definitions for HellForge mods. Version 0.2.0 also adds compatibility with the **built-in HellForge Editor**, allowing custom **Explosion, Projectile and Damage Type IDs** to be displayed and their supported fields edited in the existing Advanced categories. No replacement editor is required.

## Install and use

1. Download [Explosives Library 0.3.0](downloads/explosives-library-0.3.0.zip), import it into HellForge, and enable it instead of an older release or editor lab. Keep one EL provider enabled.
2. Enable a compatible consumer, such as More Plasma Stratagems. The library alone adds no weapons or stratagems. Other mods' HellForge-registered custom projectiles/damage can also appear without using EL for explosions.
3. Keep the provider before the built-in editor where load order is configurable, then fully restart the game. Consumers must implement readiness retries; loading EL first is not a substitute for that.
4. Open F8 > ADVANCED > **Projectiles**, **Damage Types** or **Explosions**. Search the registered namespace/source name, such as `hade/plas/mortar_max_shell`.
5. Enter a value and confirm with Enter/Tab, then **APPLY/F9**. A reset must also be applied. Reopen the editor to check the value, and test a fresh projectile/deployment in game.

The editor DLL and its catalogue on disk are not replaced. EL expands the catalogue in memory when the editor reads it, routes custom fields to their runtime records, and replays sandbox-session saved edits during reload so reset removals do not reappear as old overrides.

### What can be edited

| Category | Integration |
| --- | --- |
| Projectiles | Custom HellForge projectile records: flight fields, damage reference, impact and expiry payload references where the backend permits them |
| Damage Types | Custom direct/blast damage records, including normal/durable damage, penetration and force fields supplied by the editor template |
| Explosions | Runtime impact-explosion views for discovered custom projectiles: EL inner/outer/shockwave radii and a section for the referenced damage record |

The editor exposes record fields; it does not renumber record identities. Projectile payload references use named namespace/source aliases corresponding to their explosion views, with `None` for zero. The aliases resolve to actual numeric IDs before a write reaches the backend; virtual UI IDs are not written into the game's projectile table. Labels describe the projectile's runtime payload view, not necessarily the canonical EL registration key. Multiple views may share one actual explosion/damage record, so editing it affects every consumer of that record.

EL explosion visuals/audio are inherited from a native donor. This release does not import new effects/audio, add new status-effect mechanics, or expose every explosion field. Radii must satisfy **inner <= outer <= shockwave**. Raise shockwave before raising outer beyond it, or apply a consistent set. A queued request is not proof it applied: check `EL EDIT APPLIED`.

### Optional editor integration

`explosives-library.ini` defaults to `[editor] enabled=1`. Set `enabled=0` in the package/unpacked mod configuration and restart to use the explosion registry without the editor hooks. No binary-specific editor staging probe is included in this release.

## Limitations and test status

- Community/user testing confirmed rendering and editing with other mods. The OPS Debug consumer has an unresolved, reported pre-APPLY staging issue and is treated as a known test exception; its cause is not established.
- Multiplayer has **not** been verified for this editor integration. Do not assume host/client synchronization is established.
- Discovery captures custom projectile/damage records registered **before the editor reads its catalogue**. Records registered later, or an editor initialized before the library, can be missing. Load-order testing is encouraging but does not cover every configuration.
- Explosion views resolve impact IDs after EL becomes ready. Non-EL/native explosions are not editable through these EL views; use their normal editor entries. Expiry payload labels are supported, but dedicated expiry-explosion edit views are not provided.
- Saved field keys use registered mod/name/field identities, but persistence when mod sets change and numeric IDs are reassigned remains unverified. Payload-swap selections must be rechecked after such changes. Renaming registered records can orphan saved overrides.
- A consumer can intentionally reapply its own profiles and replace editor values. Change consumer presets before editing and verify actual gameplay; registration/read logs alone do not prove damage behavior.
- Baseline preservation covers the current session from the first editor write. Full restart/default recovery across all consumers remains unverified. The sandbox reload correction is included, but not every editor/version combination is covered.
- 64 **unique private explosion definitions** are shared by all consumers. Firing a registered explosion repeatedly uses no additional slots. Capacity is unchanged in 0.2.0.
- Supported game build is **25480438**. The catalogue/build and backend validation must succeed. Backend-rejected fields/swaps remain rejected; this is not a general validation bypass.

See [editor usage and troubleshooting](docs/EDITOR.md) for logs, test steps and recovery.

## For mod authors

Copy [include/explosives_library.h](include/explosives_library.h). Register private damage/projectile records with HellForge during initialization. After PROJECTILES_READY, discover EL with `ElFind()` on the game thread; retry until the provider and required packages are ready. Register an explosion and assign the **returned ID** to your private projectile. Never hard-code or persist allocated explosion IDs.

Gameplay **ABI 1 is unchanged**: existing consumers do not need rebuilding for 0.2.0. The optional editor bridge is separate from that gameplay ABI. Definitions retain their registration identity/defaults; editor radius overrides affect runtime rows without changing registration idempotence.

- [Author guide](docs/AUTHOR-GUIDE.md)
- [Gameplay API](docs/API.md)
- [Editor integration](docs/EDITOR.md)
- [OPS debug example](examples/ops-debug): useful for inspecting the API; currently a known editor-staging test exception.

Only one library should own the table extension. Do not embed `library.c`/`registry_patch.h` in every consumer or run the old Private Explosion Lab alongside EL.

## Build and licence

Use `powershell -File scripts/build.ps1 -Zig C:/path/to/zig.exe -Test`; output/caches remain in `build/`. The fixture uses a small generated catalogue under `tests/fixtures` for reproducible source builds. Project validation additionally used the supplied full stock catalogue.

The combined 0.2.0 integration is distributed under **GPL-3.0**, because it incorporates HellForge's JSON implementation. The original library's permissive notice is retained in `LIBRARY-LICENSE-MIT.txt`; SDK headers retain their separate licence. See [third-party notices](THIRD-PARTY-NOTICES.md).

## HellForge 0.4.0 and downloads

Requires HellForge 0.4.0 beta or newer exposing SDK 16. The gameplay API is still ABI 1, so existing ABI 1 consumers continue to work. The library supports the new split built-in editor catalogue and retains its legacy catalogue parser for regression coverage. Injection remains in memory; the generated editor files and editor DLL are not replaced.

Install the release ZIP through HellForge's Add mods/import control, enable one provider and restart. The same ZIP is used on [Nexus Mods](https://www.nexusmods.com/helldivers2/mods/18533), [GitHub Releases](https://github.com/HadePasianax/hd2-explosives-library/releases), and HellForge. It is a HellForge mod package; install it through HellForge rather than a vanilla-game mod manager.

## Game-thread callbacks

The fixed 16-slot callback pool is gone in HellForge 0.4.0. EL uses the normal dynamically growing callback list and refuses initialization if registration fails. The former shared-hook fallback and caller stack scanning have been removed. Consumer mods still need to check their own registration results. No worker thread is created.

Live unloading remains unsupported. Restart after changing the provider or consumers. Current offline tests verify split-file injection, names, edit/reset behavior and registry operation; real in-game HellForge 0.4.0 testing is pending.
