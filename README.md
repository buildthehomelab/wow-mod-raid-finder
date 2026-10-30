# mod-raid-finder

A **Raid Finder** for AzerothCore (3.3.5a) with mod-playerbots: pick a raid in a retail-style
window, fill the raid with bots, gather everyone at the raid's outdoor meeting stone, and walk
in through the normal entrance. Molten Core and Blackwing Lair are supported.

There is no separate LFR difficulty, no new loot table, no wing skipping and no simulated queue.
It is a menu for the existing Normal raids.

## Requirements

- [mod-playerbots](https://github.com/mod-playerbots/mod-playerbots) and its AzerothCore fork.
- [mod-optimal-bot-raid](https://github.com/buildthehomelab/wow-mod-optimal-bot-raid): the raid
  lineups, the role of each member and the bot drafting all come from its
  `OptimalBotRaidApi.h`. The build stops with an error if it isn't installed.
- [mod-individual-progression](https://github.com/ZhengPeiRu21/mod-individual-progression),
  compiled in and enabled. It decides which raids a character may join; without it the finder
  offers no raids.
- [mod-dungeon-clear](https://github.com/jrad7/mod-dungeon-clear) runs the raid once everyone is
  inside (**Start Run**, **Pause / Resume**, **Stop Run** use its `.dc` commands).
- Recommended client addons: DungeonClear, and Journal for the **Bosses & Loot** button.

## Install

```sh
cd <azerothcore>/modules
git clone https://github.com/buildthehomelab/wow-mod-raid-finder.git mod-raid-finder
```

Clone into `mod-raid-finder` (without `wow-`): AzerothCore derives the loader name from the
folder. Re-run CMake, rebuild, and copy `conf/mod_raid_finder.conf.dist` to
`mod_raid_finder.conf`.

Copy `addon/RaidFinder` into the client's `Interface/AddOns`, or hand it out with Portalkeeper
(`sql/portalkeeper_addon.sql`). Open it with `/lfr` or `/raidfinder`. `/lfr` replaces the client's Raid Browser command; the
Raid Browser is still in the Looking For Group window. The server module and the
addon ship together so their protocol versions match.

## Configuration

Everything is off by default. Turn on the master switch and each raid you want:

```ini
RaidFinder.Enable = 1
RaidFinder.mc = 1
RaidFinder.bwl = 1
RaidFinder.mc.MeetingStoneSpawn = 427
RaidFinder.bwl.MeetingStoneSpawn = 428
```

The meeting-stone settings are **gameobject spawn GUIDs**, not template entries. Spawn 427
(template 179584) is the stone at the Blackrock Depths approach, used for MC; spawn 428
(template 179585) is the one at the Blackrock Spire approach, used for BWL. Both are on map 0.
Coordinates come from the loaded spawn data, with a small landing offset beside the stone. A
missing spawn, an object that isn't a meeting stone or a stone inside an instance is refused;
the finder never falls back to teleporting into the raid. Check the landing spot and the walk to
the entrance against your terrain and client patches.

## How it works

1. **Pick a raid.** The server sends the enabled raids this character and the whole group have
   unlocked: level 60, the raid's access requirements, and Individual Progression's stage
   (Molten Core is the first raid; Blackwing Lair appears once Molten Core progression is done).
   It respects Individual Progression's random-bot and account exclusions. A lower-stage human
   or alt in the group blocks a later raid, and an offline member is never assumed eligible.
   The same checks run again before every action, so typing the commands by hand or an old
   window can't skip progression.
2. **Find Raid** (group leader). Humans and bots already in the group count toward the lineup;
   mod-optimal-bot-raid drafts the rest at level 60. A full roster that's short of tanks or
   healers counts as a failure. **Dismiss Bots** releases only the bots drafted for this leader
   (`.botraid dismiss`, with its cooldown).
3. **Summon to Stone.** The leader travels to the outdoor meeting stone and brings the drafted
   bots. Other humans each confirm their own travel; the leader never teleports them. Bots you
   invited yourself and alts keep their own travel. Recruiting needs the leader outside any
   instance. **Gather Bots** retries the bot summon while the leader stands at the stone.
4. **Enter** through the normal entrance, leader first so the instance is shared. Following
   through the portal is mod-playerbots' job. **Start Run** checks all 40 members (role, access,
   combat, same instance) and then asks Dungeon Clear to start; it won't start while the group
   is still outside. Stopping doesn't clear saves or leave the raid.

Random bots in the raid drop their own save to it when it differs from the leader's (bots
keep saves from earlier runs), so they follow the leader into one instance; a bot already inside
keeps its save. Humans and alts keep theirs, and a human or alt with a different permanent save
blocks the run. The save
shown is the player's own, not a promise of a fresh instance or loot. Human healers and damage
dealers can lead; a bot tank leads the run (human tanks aren't supported yet). Drafted bots
follow raid leadership and are forgotten on a worldserver restart.

The addon talks to the server with `.raidfinder <action> <raid> <request id>` and reads the
replies as addon whispers (prefix `RAIDFINDER`, protocol 2). Every reply carries the request id,
so a late packet can't pass for the answer to a retry, and quest, level, group and zone changes
refresh the list.

## Before you turn a raid on

Nothing here has been run in a game client or on a live worldserver. Test on your realm: solo
and friends-party assembly, role shortages, progression and access failures, save conflicts,
each human's confirmation, the stone landing and the walk in, partial arrivals, manually invited
bots and alts, a failed teleport, pause and resume, a wipe, a reconnect and a restart, boss and
loot progress, and a complete run.

## Tests

With a C++17 compiler and Lua 5.1:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -Wno-missing-field-initializers tests/finder.cpp -o /tmp/finder-test && /tmp/finder-test
c++ -std=c++17 -Wall -Wextra -Werror -Wno-missing-field-initializers -DRAID_FINDER_IP tests/finder.cpp -o /tmp/finder-ip-test && /tmp/finder-ip-test
lua5.1 tests/model.lua
lua5.1 tests/ui.lua
```

`tests/finder.cpp` runs the real command code (`src/RaidFinderCommands.inc`) against a small
stand-in core and a stand-in for mod-optimal-bot-raid's API; the Lua tests run the addon's real
protocol model and UI handlers. CI runs them next to the shared checks. They don't replace a
full build.

## License

GNU AGPL v3.0, see [LICENSE](LICENSE). The code started in mod-optimal-bot-raid, which uses the
same license.
