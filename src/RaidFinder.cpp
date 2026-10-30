/*
 * mod-raid-finder: a Raid Finder tab in the Raid Browser (addon/RaidFinder) for full bot raids,
 * from Zul'Gurub to Icecrown Citadel.
 *
 * The raid lineups, roles and bot drafting come from mod-optimal-bot-raid through its
 * OptimalBotRaidApi.h; Individual Progression decides which raids a character may join, and
 * mod-legacy's Summoning Stone perk unlocks Summon to Stone.
 *
 * Released under the GNU AGPL v3.0, see LICENSE.
 */

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Config.h"
#include "DBCStores.h"
#include "GameObjectData.h"
#include "Group.h"
#include "InstanceSaveMgr.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotMgr.h"
#include "RandomPlayerbotMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#if __has_include("OptimalBotRaidApi.h")
#include "OptimalBotRaidApi.h"
#else
#error "mod-raid-finder needs mod-optimal-bot-raid (https://github.com/buildthehomelab/wow-mod-optimal-bot-raid) installed next to it."
#endif
#if __has_include("IndividualProgression.h")
#include "IndividualProgression.h"
#define RAID_FINDER_IP 1
#endif
#if __has_include("Legacy.h")
#include "Legacy.h"
#define RAID_FINDER_LEGACY 1
#endif
#include <cmath>
#include <string>
#include <unordered_set>
#include <vector>

using namespace Acore::ChatCommands;

class raid_finder_commandscript : public CommandScript
{
public:
    raid_finder_commandscript() : CommandScript("raid_finder_commandscript") { }

    // The addon sends `.raidfinder <action> <raid> <request id>` and reads the replies as addon
    // whispers; typing it by hand works too, but the answers only show in the window.
    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable = {
            { "raidfinder", HandleFinder, SEC_PLAYER, Console::No },
        };
        return commandTable;
    }

    #include "RaidFinderCommands.inc"
};

void AddRaidFinderScripts()
{
    new raid_finder_commandscript();
}
