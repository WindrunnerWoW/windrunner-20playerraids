# windrunner-20playerraids

Turns the 40-player raids into 20-player raids for a single-player server, so a
roster needs 19 bots instead of 39.

Place this repository at `modules/windrunner-20playerraids` in the TortoiseWoW source
tree (usually as a git submodule).

## What it does

When `Raid20.Enable = 1`, at startup the module:

- caps each map in `Raid20.Maps` at `Raid20.MaxPlayers` (default 20). GMs do not count.
- gives every creature on those maps (bosses, trash, summons) `Raid20.HealthPercent`
  (default 50%) of its template health. The multiplier is fixed, applied on spawn,
  respawn, entry change and scripted health changes in Razorgore and Kel'Thuzad's adds,
  so players joining or dying never change a fight.
  Player pets and totems are excluded.
- bypasses the player-count autoscaler (`AutoScalerEnable`) on those maps so there is no
  second health or damage adjustment. Gold drops stay at their normal values.
- switches on the 20-player encounter adjustments in the core raid scripts.

Incoming damage, loot, lockouts and the raid-group requirement are unchanged.
There is no SQL; the map limit is changed in memory.

## Encounter adjustments

| Raid | Change (20-player mode only) |
| --- | --- |
| Blackwing Lair | Chromaggus Brood Affliction hits 6-8 targets per cast (was 11-15). |
| Temple of Ahn'Qiraj | Huhuran Poison Bolt Volley max 8 targets. AQ40 Drain Mana 6 targets (was 12). Viscidus freeze at 50/75/100 frost hits (was 100/150/200), shatter at 25/50/75 melee hits (was 50/100/150). |
| Naxxramas | Gothik needs at least 5 living players per side at the default cap (was 11); other caps require `max(1, floor(MaxPlayers / 4))` per side. Noth Curse of the Plaguebringer max 10 targets. Faerlina Poison Bolt Volley 5 targets (was 10). Sapphiron Life Drain 4-5 targets (was 7-10). |

Scripts use `sRaidSizeOverride` (`src/game/Maps/RaidSizeOverride.h`) to check whether
the current map is a reduced raid.

## Not done yet

- Four Horsemen back-corner soaking and Razuvious understudy control: unchanged, need a
  20-player test.
- Onyxia, Molten Core and custom raids: add counts and phases unchanged.

## Config

Copy `conf/windrunner-20playerraids.conf.dist` to `windrunner-20playerraids.conf` in the module
config directory. Settings are read once at startup; changes need a restart.

`Raid20.MaxPlayers` must be at least 2. `Raid20.HealthPercent` must be finite and
in the range `(0, 100]`. Invalid settings leave the module disabled. Map IDs must
be unsigned decimal integers; malformed entries are logged and skipped.
