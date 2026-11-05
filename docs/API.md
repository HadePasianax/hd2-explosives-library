# ABI 1 reference

Include `explosives_library.h` with HellForge SDK 16 headers available (provider 0.3.0 requires HellForge 0.4.0). Target Windows x64, default C ABI. Use the supplied header unchanged for ABI declarations. It includes Windows/PSAPI declarations for provider discovery.

## Discovery

`const ElApi* ElFind(void)` returns the single compatible loaded provider, or `NULL` when none is available, module enumeration fails, a matching export has an incompatible API, or multiple matching providers are found. It enumerates up to 1,024 loaded modules. It does not initialize the library or register a dependency.

The provider exports `const ElApi* explosives_library_get_api(uint32_t abi)`. ABI 1 returns its service table; unsupported ABI values return `NULL`. Consumers should use `ElFind()` rather than assume the DLL filename.

## ElExplosion

Zero-initialize the structure, then set its fields:

| Field | Contract |
| --- | --- |
| `size` | Exactly `sizeof(ElExplosion)` |
| `base` | Native explosion ID 1–422 on supported build 25480438; not a private explosion ID |
| `damage` | Valid private damage ID exposed by HellForge's `damage_row`; mandatory |
| `flags` | Zero, `EL_RADII`, `EL_SHRAPNEL`, or both; unknown bits rejected |
| `name` | Nonempty, slash-containing, globally namespaced key, maximum 127 bytes excluding NUL |
| `package` | Effect package hash to hold, or zero if managed by the consumer |
| `inner_radius` | Applied only with `EL_RADII` |
| `outer_radius` | Applied only with `EL_RADII` |
| `shockwave_radius` | Applied only with `EL_RADII` |
| `shrapnel_count` | Applied only with `EL_SHRAPNEL`; supporting donor data is inherited |

With `EL_RADII`, all three radii must be finite and satisfy `0 <= inner <= outer <= shockwave`. Without that flag, all donor radii are preserved and the supplied radius values are ignored. The radius names follow the SDK mapping; do not assume shockwave radius is an additional full-damage radius.

Changing shrapnel count alone does not create a valid bomblet projectile configuration. The provider does not validate every opaque donor field, so validate such donors and changes in game.

## ElApi functions

### register_explosion

`uint32_t register_explosion(const ElExplosion* definition)`

Call on HellForge's game thread after the provider registry and private damage rows are ready. Returns a private explosion ID on success, zero otherwise. It copies the donor descriptor, replaces its internal identity and damage link, applies flagged overrides, and publishes a stable pointer in the shared table.

Zero may mean invalid input, wrong thread, missing private damage, registry/package not ready, donor unavailable, conflicting key, or exhausted capacity. There is currently no per-request error code. `status()` describes provider installation only, so it may say `ready` even when a request is rejected.

The key, descriptor, and numeric settings are copied. The caller's `name` buffer can be temporary after the call. Asset/auxiliary pointers inherited from the donor remain native references. Registration is idempotent for the same key and effective settings. Donor, damage ID, package, flags, and enabled overrides participate in equality. Changing an ignored radius or shrapnel field does not change the definition.

### copy_explosion

`int copy_explosion(uint32_t id, HfExplosionSettings* out)`

Game-thread-only. Copies a registered private descriptor to caller-owned storage and returns 1. Returns 0 for a null output, wrong thread, unavailable registry, or an ID not registered by this provider. Editing this copy does not update the registered explosion. Its opaque pointers are borrowed native references; do not free or mutate their targets.

### status

`const char* status(void)`

Returns provider-owned text: `ready`, an initial waiting description, or an installation failure reason. Do not free the string. It is not the last registration error. Query it from the game thread with the other service operations.

## Damage API

There is no separate library damage allocator. Use `HfApi.add_damage(mod, &definition)` during `hellforge_mod_init`. The returned damage ID can then be supplied to `ElExplosion.damage` after HellForge's ready event. This deliberately uses one HellForge damage registry rather than introducing a competing allocator.

`HfDamage.base` selects a native damage row, and `scale` scales its standard/durable damage (zero means 1). After readiness, your own `HfDamageInfo` row exposes standard/durable damage, four AP values, demolition, stagger, push, and existing status slots. Keep its `id` unchanged. Native rows are not generally exposed by `damage_row`.

## Capacity and lifetime

ABI 1 reserves 64 explosion slots shared by all consumers, normally 423–486 on this build. Those numbers are implementation details, not identifiers for authors to hard-code. Damage/projectile allocation and other consumers may change the IDs your mod receives. Restart the game to discard registrations; no hot-unload or unregister is supported.

## Editor compatibility in 0.2.0

Gameplay ABI 1 is unchanged. The optional `explosives_library_editor_api(1)` bridge is for tooling: it enumerates registered EL definitions, reads radius values/defaults and queues radius overrides/reset requests. It does not replace `ElFind()` and is not required by consumers. Game-thread application validates ordered radii. Registration identities/defaults stay immutable even when the editor overrides a runtime row. See [EDITOR.md](EDITOR.md) for supported fields, alias naming, startup constraints and unverified behavior.
