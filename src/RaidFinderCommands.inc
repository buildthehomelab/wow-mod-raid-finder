// Included in the command script (and in tests/finder.cpp against a stand-in core).
// The lineups, roles and bot drafting come from mod-optimal-bot-raid (OptimalBotRaidApi.h).
// Commands run on the world thread. No raw Player pointers survive a command.
// The finder's offers: one row per raid and size. id is the finder's own key (the addon's raid id
// and the `RaidFinder.<id>` switch); lineup is the mod-optimal-bot-raid alias. Shared maps
// (Onyxia, Naxxramas) have a 40-player row for levels 60-70 and 10/25 rows for level 80, as
// Individual Progression lets characters in. Stones are spawn GUIDs of the outdoor meeting
// stones; 0 means the raid has none and the group gathers at its exit (where you land when you
// leave it).
enum FinderHeader : uint8 { FINDER_CLASSIC, FINDER_TBC, FINDER_WOTLK };

struct FinderRaid
{
    char const* id;
    char const* lineup;
    uint32 map;
    char const* name;
    uint8 header;
    uint8 level;
    uint8 maxLevel;     // 0 = no upper limit
    uint8 size;
    uint32 meetingStoneSpawn;
};

static std::vector<FinderRaid> const& FinderRaids()
{
    static std::vector<FinderRaid> const raids = {
#ifdef RAID_FINDER_IP
        { "zg", "zg", 309, "Zul'Gurub", FINDER_CLASSIC, 60, 0, 20, 10124 },
        { "aq20", "aq20", 509, "Ruins of Ahn'Qiraj", FINDER_CLASSIC, 60, 0, 20, 49434 },
        { "mc", "mc", 409, "Molten Core", FINDER_CLASSIC, 60, 0, 40, 427 },
        { "ony40", "onyxia", 249, "Onyxia's Lair", FINDER_CLASSIC, 60, 70, 40, 7013 },
        { "bwl", "bwl", 469, "Blackwing Lair", FINDER_CLASSIC, 60, 0, 40, 428 },
        { "aq40", "aq40", 531, "Temple of Ahn'Qiraj", FINDER_CLASSIC, 60, 0, 40, 49434 },
        { "naxx40", "naxx", 533, "Naxxramas", FINDER_CLASSIC, 60, 70, 40, 361003 },
        { "kara", "kara", 532, "Karazhan", FINDER_TBC, 70, 0, 10, 14003 },
        { "za", "za", 568, "Zul'Aman", FINDER_TBC, 70, 0, 10, 100514 },
        { "gruul", "gruul", 565, "Gruul's Lair", FINDER_TBC, 70, 0, 25, 25330 },
        { "mag", "mag", 544, "Magtheridon's Lair", FINDER_TBC, 70, 0, 25, 25326 },
        { "ssc", "ssc", 548, "Serpentshrine Cavern", FINDER_TBC, 70, 0, 25, 23168 },
        { "tk", "tk", 550, "Tempest Keep", FINDER_TBC, 70, 0, 25, 23169 },
        { "hyjal", "hyjal", 534, "Hyjal Summit", FINDER_TBC, 70, 0, 25, 17269 },
        { "bt", "bt", 564, "Black Temple", FINDER_TBC, 70, 0, 25, 17355 },
        { "swp", "swp", 580, "Sunwell Plateau", FINDER_TBC, 70, 0, 25, 27754 },
        { "naxx10", "naxx", 533, "Naxxramas", FINDER_WOTLK, 80, 0, 10, 59520 },
        { "naxx25", "naxx", 533, "Naxxramas", FINDER_WOTLK, 80, 0, 25, 59520 },
        { "os10", "os", 615, "The Obsidian Sanctum", FINDER_WOTLK, 80, 0, 10, 66631 },
        { "os25", "os", 615, "The Obsidian Sanctum", FINDER_WOTLK, 80, 0, 25, 66631 },
        { "eoe10", "eoe", 616, "The Eye of Eternity", FINDER_WOTLK, 80, 0, 10, 62371 },
        { "eoe25", "eoe", 616, "The Eye of Eternity", FINDER_WOTLK, 80, 0, 25, 62371 },
        { "voa10", "voa", 624, "Vault of Archavon", FINDER_WOTLK, 80, 0, 10, 268681 },
        { "voa25", "voa", 624, "Vault of Archavon", FINDER_WOTLK, 80, 0, 25, 268681 },
        { "ony10", "onyxia", 249, "Onyxia's Lair", FINDER_WOTLK, 80, 0, 10, 7013 },
        { "ony25", "onyxia", 249, "Onyxia's Lair", FINDER_WOTLK, 80, 0, 25, 7013 },
        { "uld10", "uld", 603, "Ulduar", FINDER_WOTLK, 80, 0, 10, 65119 },
        { "uld25", "uld", 603, "Ulduar", FINDER_WOTLK, 80, 0, 25, 65119 },
        { "toc10", "toc", 649, "Trial of the Crusader", FINDER_WOTLK, 80, 0, 10, 0 },
        { "toc25", "toc", 649, "Trial of the Crusader", FINDER_WOTLK, 80, 0, 25, 0 },
        { "icc10", "icc", 631, "Icecrown Citadel", FINDER_WOTLK, 80, 0, 10, 0 },
        { "icc25", "icc", 631, "Icecrown Citadel", FINDER_WOTLK, 80, 0, 25, 0 },
        { "rs10", "rs", 724, "The Ruby Sanctum", FINDER_WOTLK, 80, 0, 10, 66631 },
        { "rs25", "rs", 724, "The Ruby Sanctum", FINDER_WOTLK, 80, 0, 25, 66631 },
#endif
    };
    return raids;
}

// Individual Progression runs the 40-player Onyxia and Naxxramas in the 10-man heroic slot; its
// access rows for that slot are the level 50/60-70 ones (the normal 10-man rows require 80).
static Difficulty FinderDifficulty(FinderRaid const& raid)
{
    if (raid.size == 40 && (raid.map == 249 || raid.map == 533)) return RAID_DIFFICULTY_10MAN_HEROIC;
    return raid.size == 25 ? RAID_DIFFICULTY_25MAN_NORMAL : RAID_DIFFICULTY_10MAN_NORMAL;
}

static void FinderSend(Player* player, uint32 request, std::string const& payload)
{
    WorldPacket packet;
    ChatHandler::BuildChatPacket(packet, CHAT_MSG_WHISPER, LANG_ADDON,
        player->GetGUID(), player->GetGUID(), "RAIDFINDER\t3\t" + std::to_string(request) + "\t" + payload,
        CHAT_TAG_NONE, player->GetName());
    player->GetSession()->SendPacket(&packet);
}

static bool FinderEnabled(FinderRaid const& raid)
{
    return sConfigMgr->GetOption<bool>("RaidFinder.Enable", false) &&
        sConfigMgr->GetOption<bool>(std::string("RaidFinder.") + raid.id, true);
}

// Summon to Stone is the Legacy perk Summoning Stone; without mod-legacy it stays free.
static bool FinderCanSummon(Player* player)
{
#ifdef RAID_FINDER_LEGACY
    if (!sConfigMgr->GetOption<bool>("RaidFinder.SummonNeedsLegacyPerk", true) || !Legacy::GetConfig().enabled)
        return true;
    return Legacy::Rank(player, Legacy::PERK_SUMMONING_STONE) > 0;
#else
    (void)player;
    return true;
#endif
}

// Where a raid gathers: its outdoor meeting stone, or the raid's exit when it has none. Use the
// realm's loaded spawn and trigger data; never accept a client destination.
struct FinderPoint
{
    uint32 map;
    float x, y, z, o;
    bool stone;
};

static Optional<FinderPoint> FinderStone(FinderRaid const& raid)
{
    uint32 const spawn = sConfigMgr->GetOption<uint32>(
        std::string("RaidFinder.") + raid.id + ".MeetingStoneSpawn", raid.meetingStoneSpawn);
    if (!spawn)
    {
        auto const* exit = sObjectMgr->GetGoBackTrigger(raid.map);
        auto const* map = exit ? sMapStore.LookupEntry(exit->target_mapId) : nullptr;
        if (!map || map->Instanceable()) return {};
        return FinderPoint{ exit->target_mapId, exit->target_X, exit->target_Y, exit->target_Z, exit->target_Orientation, false };
    }
    auto const* stone = sObjectMgr->GetGameObjectData(spawn);
    if (!stone) return {};
    auto const* object = sObjectMgr->GetGameObjectTemplate(stone->id);
    auto const* map = sMapStore.LookupEntry(stone->mapid);
    if (!object || object->type != GAMEOBJECT_TYPE_MEETINGSTONE || !map || map->Instanceable())
        return {};
    // Land beside the object rather than inside its collision.
    return FinderPoint{ stone->mapid, stone->posX + 3.0f * std::cos(stone->orientation),
        stone->posY + 3.0f * std::sin(stone->orientation), stone->posZ + 0.5f, stone->orientation, true };
}

static bool FinderAtStone(Player* player, FinderPoint const& stone)
{
    float const dx = player->GetPositionX() - stone.x;
    float const dy = player->GetPositionY() - stone.y;
    float const dz = player->GetPositionZ() - stone.z;
    return player->GetMapId() == stone.map && dx * dx + dy * dy + dz * dz <= 400.0f;
}

static bool FinderTravelToStone(Player* player, FinderPoint const& stone)
{
    if (FinderAtStone(player, stone)) return true;
    return player->TeleportTo(stone.map, stone.x, stone.y, stone.z, stone.o);
}

static std::string FinderGatherDrafts(Player* player, FinderPoint const& stone)
{
    std::unordered_set<ObjectGuid> const drafted = OptimalBotRaid::DraftedBots(player->GetGUID());
    Group* group = player->GetGroup();
    if (!group) return "You need an assembled raid.";
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        if (Player* bot = ref->GetSource(); bot && drafted.count(bot->GetGUID()) &&
            PlayerbotsMgr::instance().GetPlayerbotAI(bot) && sRandomPlayerbotMgr.IsRandomBot(bot))
            if (!FinderTravelToStone(bot, stone))
                return bot->GetName() + ": could not be summoned. Some members may already be travelling; use Gather Bots at the stone to retry.";
    return "";
}

// Persistent eligibility is shared by catalogue filtering and every action.
// Do not use transient combat/travel state to hide an otherwise unlocked raid.
static std::string FinderAccessProblem(Player* member, [[maybe_unused]] FinderRaid const& raid)
{
    if (!member || !member->IsInWorld()) return "A group member is offline; eligibility cannot be checked.";
#ifndef RAID_FINDER_IP
    return "Individual Progression support is required for Raid Finder.";
#else
    std::string const who = member->GetName() + ": ";
    if (!sIndividualProgression->enabled)
        return "Individual Progression must be enabled for Raid Finder.";
    if (member->GetLevel() < raid.level) return who + "requires level " + std::to_string(raid.level) + ".";
    if (raid.maxLevel && member->GetLevel() > raid.maxLevel)
        return who + "is above level " + std::to_string(raid.maxLevel) + "; this is the 40-player version.";
    // Individual Progression's own entry rules (stage, attunements, keys), for normal accounts.
    IndividualProgression::InstanceAccess const access = sIndividualProgression->CheckInstanceAccess(member, raid.map);
    if (!access.allowed)
        return who + (access.message.empty() ? "cannot enter this raid yet (Individual Progression)." : access.message);
    if (!member->Satisfy(sObjectMgr->GetAccessRequirement(raid.map, FinderDifficulty(raid)), raid.map, false))
        return who + "does not meet the raid's access requirements (level, quest or item).";
    return "";
#endif
}

static std::string FinderCatalogueProblem(Player* player, FinderRaid const& raid)
{
    if (!FinderEnabled(raid)) return "This raid is not enabled for Raid Finder testing.";
    if (!FinderStone(raid)) return "The raid has no valid gathering point.";
    if (Group* group = player->GetGroup())
    {
        for (auto const& slot : group->GetMemberSlots())
            if (std::string problem = FinderAccessProblem(ObjectAccessor::FindConnectedPlayer(slot.guid), raid); !problem.empty())
                return problem;
        return "";
    }
    return FinderAccessProblem(player, raid);
}

static std::string FinderMemberProblem(Player* member, FinderRaid const& raid)
{
    if (std::string problem = FinderAccessProblem(member, raid); !problem.empty()) return problem;
    std::string const who = member->GetName() + ": ";
    if (!member->IsAlive()) return who + "must be alive.";
    if (member->IsInCombat()) return who + "is in combat.";
    if (member->IsBeingTeleported() || member->IsInFlight() || member->InBattleground() || member->InArena())
        return who + "is travelling or in PvP.";
    if (member->GetRaidDifficulty() != FinderDifficulty(raid))
    {
        if (FinderDifficulty(raid) == RAID_DIFFICULTY_10MAN_HEROIC)
            return who + "must select 10 Player (Heroic) raid difficulty, the 40-player version.";
        return who + (raid.size == 25 ? "must select 25 Player raid difficulty." : "must select 10 Player raid difficulty.");
    }
    if (member->GetMap()->Instanceable() && member->GetMapId() != raid.map)
        return who + "must leave the current instance first.";
    // A solo/party preflight is not entry: the assembler has yet to create the raid.
    // CannotEnter also deliberately rejects players ALREADY in the target map.
    // TeleportTo enforces the full checks when an actual entrant crosses maps.
    if (member->GetGroup() && member->GetGroup()->isRaidGroup() && member->GetMapId() != raid.map &&
        sMapMgr->PlayerCannotEnter(raid.map, member))
        return who + "cannot enter this raid (access, instance save or capacity).";
    return "";
}

// Random bots keep saves from their own runs. Drop a random bot's save to this raid
// when it differs from the leader's, so it follows the leader into one instance.
// Humans and alts keep their saves, and so does a bot already inside the raid.
static uint32 FinderClearBotSaves(Player* player, FinderRaid const& raid)
{
    Group* group = player->GetGroup();
    if (!group) return 0;
    auto const* leaderBind = sInstanceSaveMgr->PlayerGetBoundInstance(group->GetLeaderGUID(), raid.map, FinderDifficulty(raid));
    uint32 const leaderSave = leaderBind && leaderBind->save ? leaderBind->save->GetInstanceId() : 0;
    uint32 cleared = 0;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* bot = ref->GetSource();
        if (!bot || bot->GetMapId() == raid.map || !PlayerbotsMgr::instance().GetPlayerbotAI(bot) || !sRandomPlayerbotMgr.IsRandomBot(bot))
            continue;
        auto const* bind = sInstanceSaveMgr->PlayerGetBoundInstance(bot->GetGUID(), raid.map, FinderDifficulty(raid));
        if (!bind || !bind->save || bind->save->GetInstanceId() == leaderSave) continue;
        sInstanceSaveMgr->PlayerUnbindInstance(bot->GetGUID(), raid.map, FinderDifficulty(raid), true, bot);
        ++cleared;
    }
    return cleared;
}

// The leader sets the group (or their own) raid difficulty to the selected size, as long as
// nobody is inside an instance; the core refuses changes there, and so does this.
static void FinderSetDifficulty(Player* player, FinderRaid const& raid)
{
    Difficulty const difficulty = FinderDifficulty(raid);
    Group* group = player->GetGroup();
    if (!group)
    {
        if (player->GetRaidDifficulty() != difficulty && !player->GetMap()->Instanceable())
        {
            player->SetRaidDifficulty(difficulty);
            player->SendRaidDifficulty(false);
        }
        return;
    }
    if (!group->IsLeader(player->GetGUID()) || group->GetRaidDifficulty() == difficulty) return;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        if (Player* member = ref->GetSource(); member && member->GetMap()->Instanceable()) return;
    group->SetRaidDifficulty(difficulty);
}

// Only status is allowed during combat; actions recheck live group membership and saves.
static std::string FinderProblem(Player* player, FinderRaid const& raid, bool ready = false)
{
    if (!FinderEnabled(raid)) return "This raid is not enabled for Raid Finder.";
    OptimalBotRaid::Lineup lineup;
    if (!OptimalBotRaid::GetLineup(raid.lineup, raid.size, lineup)) return "The raid's lineup is unavailable.";
    if (!ready && !FinderStone(raid)) return "The raid has no valid gathering point.";
    Group* group = player->GetGroup();
    if (group && group->GetMembersCount() > raid.size) return "The group is too large for this raid.";
    if (group && group->isLFGGroup()) return "Leave the Dungeon Finder group before preparing a raid.";
    auto const* leaderBind = sInstanceSaveMgr->PlayerGetBoundInstance(
        group ? group->GetLeaderGUID() : player->GetGUID(), raid.map, FinderDifficulty(raid));
    uint32 const leaderSave = leaderBind && leaderBind->save ? leaderBind->save->GetInstanceId() : 0;
    uint32 tanks = 0, healers = 0;
    bool botTank = false;
    auto check = [&](Player* member) -> std::string
    {
        if (std::string problem = FinderMemberProblem(member, raid); !problem.empty()) return problem;
        auto const* bind = sInstanceSaveMgr->PlayerGetBoundInstance(member->GetGUID(), raid.map, FinderDifficulty(raid));
        if (bind && bind->perm && bind->save && bind->save->GetInstanceId() != leaderSave)
            return member->GetName() + ": has a different raid save. Raid Finder never clears players' saves.";
        OptimalBotRaid::Role role = OptimalBotRaid::GetRaidRole(member);
        if (role == OptimalBotRaid::Role::Tank) { ++tanks; botTank |= PlayerbotsMgr::instance().GetPlayerbotAI(member) != nullptr; }
        if (role == OptimalBotRaid::Role::Healer) ++healers;
        if (ready && (member->GetMapId() != raid.map || member->GetInstanceId() != player->GetInstanceId()))
            return member->GetName() + ": has not arrived in this instance.";
        return "";
    };
    if (group)
    {
        // MemberSlots includes disconnected members, unlike GroupReference iteration.
        for (auto const& slot : group->GetMemberSlots())
            if (std::string problem = check(ObjectAccessor::FindConnectedPlayer(slot.guid)); !problem.empty()) return problem;
    }
    else if (std::string problem = check(player); !problem.empty()) return problem;
    if (ready)
    {
        if (!group || group->GetMembersCount() != raid.size)
            return "Recruitment is incomplete: " + std::to_string(raid.size) + " members are required.";
        if (!botTank || tanks < lineup.tanks || healers < lineup.healers)
            return "The lineup is short of tanks or healers. A bot tank must lead the run.";
    }
    return "";
}

static bool HandleFinder(ChatHandler* handler, std::string action, Optional<std::string> optRaid, Optional<uint32> optRequest = {})
{
    Player* player = handler->GetSession()->GetPlayer();
    uint32 const request = optRequest ? *optRequest : 0;
    auto send = [&](std::string const& payload) { FinderSend(player, request, payload); };
    if (action == "catalog")
    {
        send("BEGIN");
        for (auto const& raid : FinderRaids())
        {
            OptimalBotRaid::Lineup q;
            if (!OptimalBotRaid::GetLineup(raid.lineup, raid.size, q) || !FinderCatalogueProblem(player, raid).empty()) continue;
            send("RAID\t" + std::string(raid.id) + "\t" + raid.name + "\t" + std::to_string(raid.map) + "\t" +
                std::to_string(raid.header) + "\t" + std::to_string(raid.level) + "\t" + std::to_string(raid.size) + "\t" +
                std::to_string(q.tanks) + "\t" + std::to_string(q.healers) + "\t" +
                std::to_string(raid.size - q.tanks - q.healers) + "\t" + (FinderEnabled(raid) ? "1" : "0"));
        }
        send("END");
        return true;
    }
    FinderRaid const* selected = nullptr;
    for (auto const& raid : FinderRaids()) if (optRaid && *optRaid == raid.id) selected = &raid;
    auto reply = [&](bool ok, std::string const& message)
    {
        send("RESULT\t" + action + "\t" + (selected ? selected->id : "-") + "\t" +
            (ok ? "1" : "0") + "\t" + message);
        return true;
    };
    if (!selected) return reply(false, "Select a raid first.");
    FinderRaid const& raid = *selected;
    Group* group = player->GetGroup();
    if (action != "status" && action != "summon" && group && !group->IsLeader(player->GetGUID()))
        return reply(false, "Only the raid leader can prepare or control the raid.");
    if (action == "status")
    {
        auto const* bind = sInstanceSaveMgr->PlayerGetBoundInstance(player->GetGUID(), raid.map, FinderDifficulty(raid));
        std::string save = "No personal raid save.";
        if (bind && bind->save)
            save = "Saved instance " + std::to_string(bind->save->GetInstanceId()) + "; resets " +
                std::to_string(bind->save->GetResetTime()) + ".";
        auto const point = FinderStone(raid);
        send("STATUS\t" + std::string(raid.id) + "\t" +
            std::to_string(group ? group->GetMembersCount() : 1) + "\t" +
            std::to_string(uint32(OptimalBotRaid::GetRaidRole(player))) + "\t" + save + "\t" +
            (FinderCanSummon(player) ? "1" : "0") + "\t" + (point && !point->stone ? "entrance" : "stone"));
        std::string problem = FinderProblem(player, raid);
        return reply(problem.empty(), problem.empty() ? "Ready to prepare. " + save : problem);
    }
    if (action != "assemble" && action != "summon" && action != "rally" && action != "ready")
        return reply(false, "Unknown Raid Finder action.");
    if (action == "summon" && !FinderCanSummon(player))
        return reply(false, "Summon to Stone is the Legacy perk Summoning Stone (Adventure). Travel to the meeting stone, then use Gather Bots.");
    if (!group || group->IsLeader(player->GetGUID()))
    {
        if (action != "ready") FinderSetDifficulty(player, raid);
        FinderClearBotSaves(player, raid);
    }
    if (std::string problem = FinderProblem(player, raid, action == "ready"); !problem.empty())
        return reply(false, problem);
    if (action == "assemble")
    {
        if (player->GetMap()->Instanceable())
            return reply(false, "Leave the instance before recruiting through Raid Finder; groups gather outside the raid.");
        if (OptimalBotRaid::GetRaidRole(player) == OptimalBotRaid::Role::Tank)
            return reply(false, "Autonomous runs currently support human healers and damage dealers. A bot tank leads.");
        OptimalBotRaid::Assemble(handler, raid.lineup, raid.size, raid.level, raid.level);
        group = player->GetGroup();
        if (group && group->IsLeader(player->GetGUID())) FinderSetDifficulty(player, raid);
        FinderClearBotSaves(player, raid);
        if (!group || group->GetMembersCount() != raid.size)
            return reply(false, "Recruitment is incomplete. See chat for availability and cooldown details; retry or dismiss drafted bots.");
        // Check roles before offering entry, even though the assembler may have filled scarce roles with DPS.
        OptimalBotRaid::Lineup quota;
        OptimalBotRaid::GetLineup(raid.lineup, raid.size, quota);
        uint32 tanks = 0, healers = 0; bool botTank = false;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            if (Player* member = ref->GetSource())
            {
                OptimalBotRaid::Role role = OptimalBotRaid::GetRaidRole(member);
                if (role == OptimalBotRaid::Role::Tank) { ++tanks; botTank |= PlayerbotsMgr::instance().GetPlayerbotAI(member) != nullptr; }
                if (role == OptimalBotRaid::Role::Healer) ++healers;
            }
        if (!botTank || tanks < quota.tanks || healers < quota.healers)
            return reply(false, "Recruitment is short of tanks or healers. Dismiss drafted bots and retry later.");
        if (std::string problem = FinderProblem(player, raid); !problem.empty()) return reply(false, problem);
        return reply(true, FinderCanSummon(player)
            ? "Raid assembled. Summon to Stone to gather outside; each human confirms their own travel."
            : "Raid assembled. Travel to the meeting stone, then use Gather Bots to bring your bots.");
    }
    if (action == "summon")
    {
        if (!group || !group->isRaidGroup() || group->GetMembersCount() != raid.size)
            return reply(false, "The raid leader must assemble the complete roster first.");
        auto const stone = FinderStone(raid);
        if (!stone) return reply(false, "The raid has no valid gathering point.");
        if (!FinderTravelToStone(player, *stone))
            return reply(false, "Summon refused by the server. No instance entry was attempted.");
        // Only the leader moves the tracked random-bot drafts. Friends confirm
        // their own travel; manually invited bots and player alts are untouched.
        if (group->IsLeader(player->GetGUID()))
            if (std::string problem = FinderGatherDrafts(player, *stone); !problem.empty()) return reply(false, problem);
        return reply(true, "Summoning to the gathering point. Gather there, then use the normal raid entrance.");
    }
    if (action == "rally")
    {
        auto const stone = FinderStone(raid);
        if (!stone || !FinderAtStone(player, *stone))
            return reply(false, stone && !stone->stone
                ? "Stand at the raid's entrance to Gather Bots."
                : "Stand at the selected raid's meeting stone to Gather Bots.");
        if (std::string problem = FinderGatherDrafts(player, *stone); !problem.empty()) return reply(false, problem);
        return reply(true, "Drafted bots summoned. Enter the raid together through its normal entrance.");
    }
    return reply(true, "The roster is ready in this instance.");
}
