# Testing and troubleshooting

## What has been observed

The precursor lab successfully used private explosion and private damage definitions through an Impact grenade and Exploding Crossbow. The separate shared-library OPS consumer then registered explosion 423 with private damage 650 and routed private projectile 351 through vanilla OPS.

In the OPS session, the consumer logged an explosion request followed 10 ms later by a kind-4 explosion damage event of 400 against a target about 4.57 m from the explosion position. Direct-hit damage was configured to zero. This strongly supports working radial damage; the nearby event is a correlation, and other gameplay mods were active. It does not establish the full damage falloff curve or every armor interaction.

Offline checks cover registry capacity, duplicate-key behavior, conflict rejection, invalid parameters, donor isolation, thread restrictions, engine-reference/bound planning, the kick-stage bound regression, OPS shell routing, and actual Windows DLL API discovery. A compiling mod and an explosion request alone are not proof of successful AoE.

## Player test procedure

1. Enable Explosives Library and the separate OPS Debug mod. Disable the previous Private Explosion Lab and competing OPS edits. Restart.
2. Select the regular Orbital Precision Strike. The debug mod overwrites its shell delivery; it does not add a second selectable stratagem.
3. Throw it beside a target, so the shell does not directly hit that target. The debug blast is configured for 800 standard / 800 durable, AP5, and 6/8/10 m radii. Its projectile direct damage is zero.
4. Check actual damage to nearby targets. Armor, hit parts, and game damage rules can affect the observed amount; do not expect every event to equal the configured damage.
5. Disable the debug mod and restart when finished. It is a test overwrite, not a balanced OPS release. Keep the library enabled for other consumers that need it.

The debug consumer preserves OPS's delivery timing and shell pattern. Its native OPS carrier uses Epoch projectile effects and a private explosion cloned from native explosion 407. Explosion 407 itself is not edited.

## Logs

Look in the latest HellForge session directory under `Documents/HellForge/logs/sessions/<session>/game/`:

| Log | Useful entries |
| --- | --- |
| `mod-hade.explosives-library.log` | Registry installation, `EL REGISTER`, or `EL DISABLED` |
| `mod-hade.explosives-library-ops-debug.log` | `OPS DEBUG ACTIVE`, explosion requests, and correlated nearby explosion damage |
| `modloader.log` | Compilation/loading failures or unsupported SDK/build |

An example successful registration is `EL REGISTER ... explosion=423 donor=407 private_damage=650 radii=6.00/8.00/10.00`. The IDs can change. A successful registration confirms a row exists; actual off-target damage is the functional test.

## Common problems

| Symptom | Check |
| --- | --- |
| `ElFind()` returns NULL | Enable one provider, check ABI 1, inspect loading errors, and wait for modules to load. No DLL filename should be assumed. |
| Provider says `ready` but registration returns zero | Validate private damage ID, native donor range, structure size, namespaced key, flags, finite ordered radii, game thread, package residency, key conflicts, and remaining slots. |
| `EL DISABLED` | Inspect the failure reason and build. An external explosion-table extension or engine layout mismatch is rejected. Restart with the conflicting extension disabled; a new game build needs code validation. |
| Visible explosion, no radial damage | Check the actual private explosion link and damage row, engine registration, effect package residency, and off-target damage. Projectile visuals alone do not connect a blast. |
| Projectile explodes in flight | Check carrier lifetime/fuse and `expiry_explosion`; a plasma weapon's flight behavior may be unsuitable for artillery. |
| Wrong firing sound | Weapon firing sounds are separate from projectile flight effects and explosion audio. Configure the weapon component. |
| Editing one blast changes another | They may share the same private damage ID. Allocate distinct damage rows for independent payloads. |

## Scope of support

Only game build 25480438 is supported. Multiplayer, broad donor coverage, and multiple independently authored consumers have not been validated. Use a matching mod set when conducting any future multiplayer test; numeric explosion registration order is not synchronized by this library. Do not claim multiplayer support based on the current local tests.

## 0.2.1 callback fallback

Offline tests cover normal callback preference, delayed game.dll arrival, invalid callsite refusal, shared-hook registration success/failure, primary-caller preference, the strictly-greater-than-250 ms secondary threshold, and forwarding the original function. The policy test substitutes synthetic caller addresses; it does not reproduce a real HellForge hook chain's stack. In-game tests are still required with 16 occupied callback slots, only the secondary caller active, multiple fallback mods, and both editor and gameplay consumers.

## 0.3.0 / SDK 16 coverage

The format-2 fixture splits native projectile, damage and explosion templates into separate listed files. Tests read the index and each category through the actual file-interception adapter, exercise partial reads and handle cleanup, and verify custom objects and named payload references with byte-identical original files. Invalid relative paths and build/schema mismatches are rejected. Existing edit/reset, registry capacity, donor isolation and discovery tests remain. Callback fallback tests were removed with that code. Runtime HellForge 0.4.0 and multiplayer remain unverified.
