// Contract harness: compile and execute the production command implementation
// against a small in-memory core and a stand-in for mod-optimal-bot-raid's API,
// including entry refusals and roster mutations.
#include <cassert>
#include <cmath>
#include <type_traits>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using uint8 = uint8_t;
using uint32 = uint32_t;
using ObjectGuid = uint32;
template<class T> using Optional = std::optional<T>;
using Difficulty = int;
constexpr Difficulty RAID_DIFFICULTY_10MAN_NORMAL = 0, RAID_DIFFICULTY_25MAN_NORMAL = 1, RAID_DIFFICULTY_10MAN_HEROIC = 2;
constexpr int CHAT_MSG_WHISPER = 1, LANG_ADDON = 2, CHAT_TAG_NONE = 0;
namespace OptimalBotRaid {
enum class Role : uint8 { Tank, Healer, Melee, Ranged, Unknown };
struct Lineup { uint32 tanks = 0; uint32 healers = 0; };
}
using OptimalBotRaid::Role;
struct WorldPacket { std::string data; };
struct Session { std::string last; std::vector<std::string> packets; void SendPacket(WorldPacket* p) { last = p->data; packets.push_back(last); } };
struct Map { bool instance = false; bool Instanceable() const { return instance; } };
struct Group;
struct Player {
    uint32 id, map = 0, instance = 0, level = 60, cls = 1, stage = 1;
    bool alive = true, combat = false, moving = false, world = true, bot = false, random = false, access = true, teleport = true, excluded = false, perk = true;
    Difficulty raidDifficulty = RAID_DIFFICULTY_10MAN_NORMAL;
    float x = 0, y = 0, z = 0, facing = 0;
    Role role = Role::Ranged;
    Group* group = nullptr;
    Session session; Map location;
    std::string GetName() const { return "Player" + std::to_string(id); }
    ObjectGuid GetGUID() const { return id; }
    bool IsInWorld() const { return world; }
    bool IsAlive() const { return alive; }
    bool IsInCombat() const { return combat; }
    bool IsBeingTeleported() const { return moving; }
    bool IsInFlight() const { return false; }
    bool InBattleground() const { return false; }
    bool InArena() const { return false; }
    uint32 GetLevel() const { return level; }
    Difficulty GetRaidDifficulty() const { return raidDifficulty; }
    void SetRaidDifficulty(Difficulty d) { raidDifficulty = d; }
    void SendRaidDifficulty(bool) { }
    Map* GetMap() { location.instance = map != 0; return &location; }
    uint32 GetMapId() const { return map; }
    uint32 GetInstanceId() const { return instance; }
    uint32 getClass() const { return cls; }
    Group* GetGroup() const { return group; }
    Session* GetSession() { return &session; }
    bool Satisfy(void const* requirement, uint32, bool) const { return access && (!requirement || level >= 80); }
    float GetPositionX() const { return x; } float GetPositionY() const { return y; }
    float GetPositionZ() const { return z; } float GetOrientation() const { return facing; }
    bool TeleportTo(uint32 target, float tx, float ty, float tz, float tf, int = 0, Player* owner = nullptr) {
        if (!teleport) return false;
        map = target; instance = (target == 409 || target == 469) ? (owner ? owner->instance : 10) : 0;
        x = tx; y = ty; z = tz; facing = tf; return true;
    }
};
struct GroupReference {
    Player* player; GroupReference* following = nullptr;
    Player* GetSource() { return player; }
    GroupReference* next() { return following; }
};
struct Slot { ObjectGuid guid; };
struct Group {
    uint32 leader = 1; bool raid = true, lfg = false; Difficulty raidDifficulty = RAID_DIFFICULTY_10MAN_NORMAL;
    std::vector<Slot> slots; std::vector<GroupReference> refs;
    uint32 GetMembersCount() const { return slots.size(); }
    bool isRaidGroup() const { return raid; } bool isLFGGroup() const { return lfg; }
    bool IsLeader(ObjectGuid g) const { return leader == g; }
    ObjectGuid GetLeaderGUID() const { return leader; }
    auto const& GetMemberSlots() const { return slots; }
    GroupReference* GetFirstMember() {
        for (size_t i = 0; i < refs.size(); ++i) refs[i].following = i + 1 < refs.size() ? &refs[i+1] : nullptr;
        return refs.empty() ? nullptr : &refs[0];
    }
    void Add(Player& p) { slots.push_back({p.id}); refs.push_back({&p}); p.group = this; }
    Difficulty GetRaidDifficulty() const { return raidDifficulty; }
    void SetRaidDifficulty(Difficulty d) { raidDifficulty = d; for (auto& r : refs) r.player->raidDifficulty = d; }
};
std::unordered_map<ObjectGuid, Player*> players;
namespace ObjectAccessor { Player* FindConnectedPlayer(ObjectGuid id) { auto i = players.find(id); return i == players.end() ? nullptr : i->second; } }
struct InstanceSave { uint32 id = 10; uint32 GetInstanceId() const { return id; } time_t GetResetTime() const { return 100; } };
struct Bind { InstanceSave* save = nullptr; bool perm = false; Difficulty difficulty = RAID_DIFFICULTY_10MAN_NORMAL; };
struct Saves {
    std::map<ObjectGuid, Bind> binds;
    Bind* PlayerGetBoundInstance(ObjectGuid id, uint32, Difficulty d) { auto it = binds.find(id); return it == binds.end() || it->second.difficulty != d ? nullptr : &it->second; }
    void PlayerUnbindInstance(ObjectGuid id, uint32, Difficulty d, bool, Player*) { if (binds.count(id) && binds[id].difficulty == d) binds.erase(id); }
} saves;
auto* sInstanceSaveMgr = &saves;
constexpr uint32 GAMEOBJECT_TYPE_MEETINGSTONE = 23;
struct GameObjectData { uint32 id = 179584, mapid = 0; float posX = -7590.69f, posY = -1139.77f, posZ = 260.681f, orientation = -2.76635f; };
struct GameObjectTemplate { uint32 type = GAMEOBJECT_TYPE_MEETINGSTONE; };
struct MapEntry { bool instance = false; bool Instanceable() const { return instance; } };
struct MapStore {
    MapEntry world{false}, raid{true};
    MapEntry const* LookupEntry(uint32 id) { return id == 0 || id == 1 || id == 530 || id == 571 ? &world : &raid; }
} sMapStore;
struct AreaTriggerTeleport { uint32 target_mapId; float target_X, target_Y, target_Z, target_Orientation; };
struct Objects {
    bool missing = false;
    AreaTriggerTeleport icecrownExit{571, 5873.82f, 2110.98f, 636.01f, 3.55f};
    AreaTriggerTeleport const* GetGoBackTrigger(uint32 map) { return map == 631 || map == 649 ? &icecrownExit : nullptr; }
    GameObjectData stone;
    GameObjectTemplate object;
    // The shared maps' normal 10/25 rows require level 80, as in the realm's dungeon_access_template.
    int level80 = 0;
    void const* GetAccessRequirement(uint32 map, Difficulty d) { return (map == 249 || map == 533) && d != RAID_DIFFICULTY_10MAN_HEROIC ? &level80 : nullptr; }
    GameObjectData const* GetGameObjectData(uint32 spawn) { return !missing && spawn ? &stone : nullptr; }
    GameObjectTemplate const* GetGameObjectTemplate(uint32) { return &object; }
} objects;
auto* sObjectMgr = &objects;
struct Maps {
    int calls = 0;
    bool PlayerCannotEnter(uint32 map, Player* p) { ++calls; return !p->group || !p->group->raid || p->map == map || !p->access; }
} maps;
auto* sMapMgr = &maps;
struct Config { bool enabled = true; template<class T> T GetOption(std::string const&, T fallback) { if constexpr (std::is_same_v<T, bool>) return enabled; return fallback; } } config;
auto* sConfigMgr = &config;
struct PlayerbotsMgr {
    static PlayerbotsMgr& instance() { static PlayerbotsMgr m; return m; }
    void* GetPlayerbotAI(Player* p) { return p->bot ? p : nullptr; }
};
struct RandomBots { bool IsRandomBot(Player* p) { return p->random; } } sRandomPlayerbotMgr;
struct ChatHandler;
// Stand-in for OptimalBotRaidApi.h: a 2-tank, 3-healer lineup and a per-leader draft list.
std::unordered_map<ObjectGuid, std::unordered_set<ObjectGuid>> drafted;
int assembleCalls = 0;
namespace OptimalBotRaid {
bool GetLineup(std::string const&, uint32 size, Lineup& lineup) { if (size != 10 && size != 20 && size != 25 && size != 40) return false; lineup = {2, 3}; return true; }
Role GetRaidRole(Player* member) { return member->role; }
bool Assemble(ChatHandler*, std::string const&, uint32, uint32, uint32) { ++assembleCalls; return true; }
std::unordered_set<ObjectGuid> DraftedBots(ObjectGuid leader) { auto it = drafted.find(leader); return it == drafted.end() ? std::unordered_set<ObjectGuid>{} : it->second; }
}
#ifdef RAID_FINDER_IP
// Stand-in for Individual Progression's CheckInstanceAccess: BWL needs stage 1, and random or
// excluded accounts are always let in.
struct IndividualProgression {
    struct InstanceAccess { bool allowed = true; std::string message; };
    bool enabled = true;
    InstanceAccess CheckInstanceAccess(Player* p, uint32 map) const {
        if (p->random || p->excluded || map != 469 || p->stage >= 1) return {};
        return { false, "must complete Molten Core progression to enter Blackwing Lair." };
    }
} progression;
auto* sIndividualProgression = &progression;
#endif
#ifdef RAID_FINDER_LEGACY
namespace Legacy {
enum Perk : uint8 { PERK_SUMMONING_STONE };
struct Config { bool enabled = true; } legacyConfig;
Config const& GetConfig() { return legacyConfig; }
uint8 Rank(Player const* p, Perk) { return p->perk ? 1 : 0; }
}
#endif
struct ChatHandler {
    Player* player;
    struct Holder { Player* p; Player* GetPlayer() { return p; } } holder;
    Holder* GetSession() { holder.p = player; return &holder; }
    static void BuildChatPacket(WorldPacket& p, int, int, ObjectGuid, ObjectGuid, std::string const& text, int, std::string const&) { p.data = text; }
};
struct Commands {
#include "../src/RaidFinderCommands.inc"
};
int main() {
    Player leader{1}; players[1] = &leader;
    ChatHandler handler{&leader};
    auto catalogue = [&]() {
        leader.session.packets.clear();
        Commands::HandleFinder(&handler, "catalog", {});
        std::string result;
        for (auto const& packet : leader.session.packets) result += packet + "\n";
        return result;
    };
#ifndef RAID_FINDER_IP
    assert(Commands::FinderRaids().empty());
    assert(catalogue().find("RAID\t") == std::string::npos);
    Commands::HandleFinder(&handler, "assemble", std::string("bwl"));
    assert(assembleCalls == 0);
    std::cout << "PASS: missing Individual Progression offers no raids or assembly\n";
    return 0;
#endif
    leader.session.packets.clear();
    Commands::HandleFinder(&handler, "catalog", std::string("-"), uint32(123));
    for (auto const& packet : leader.session.packets)
        assert(packet.find("RAIDFINDER\t3\t123\t") == 0);
    leader.session.packets.clear();
    Commands::HandleFinder(&handler, "ready", std::string("bwl"), uint32(124));
    assert(leader.session.last.find("RAIDFINDER\t3\t124\tRESULT\tready\tbwl\t") == 0);
    auto byId = [](std::string const& id) {
        for (auto const& r : Commands::FinderRaids()) if (id == r.id) return r;
        assert(false && "unknown raid id"); return Commands::FinderRaids()[0];
    };
    auto raid = byId("mc");
    auto point = [&]() { return *Commands::FinderStone(raid); };
    config.enabled = false;
    assert(!Commands::FinderProblem(&leader, raid).empty());
    config.enabled = true;
    assert(Commands::FinderProblem(&leader, raid).empty()); // solo may prepare
    assert(maps.calls == 0); // must not ask the core to enter an uncreated raid
#ifdef RAID_FINDER_IP
    progression.enabled = false;
    assert(catalogue().find("RAID\t") == std::string::npos);
    assert(Commands::FinderProblem(&leader, raid).find("must be enabled") != std::string::npos);
    progression.enabled = true;
    leader.stage = 0;
    assert(catalogue().find("RAID\tmc\t") != std::string::npos);
    assert(catalogue().find("RAID\tbwl\t") == std::string::npos);
    for (auto const& action : {"assemble", "summon", "rally", "ready"}) {
        Commands::HandleFinder(&handler, action, std::string("bwl"));
        assert(leader.session.last.find("complete Molten Core") != std::string::npos);
    }
    assert(assembleCalls == 0 && leader.map == 0);
    assert(Commands::FinderProblem(&leader, byId("bwl")).find("complete Molten Core") != std::string::npos);
    leader.stage = 1;
    assert(Commands::FinderProblem(&leader, byId("bwl")).empty());
    assert(catalogue().find("RAID\tbwl\t") != std::string::npos);
    leader.stage = 18;
    assert(catalogue().find("RAID\tmc\t") != std::string::npos); // prior tiers remain unlocked
    leader.level = 59;
    assert(catalogue().find("RAID\t") == std::string::npos);
    leader.level = 60; leader.access = false;
    assert(catalogue().find("RAID\t") == std::string::npos);
    leader.access = true; leader.stage = 1;
#endif
    Group group; group.Add(leader);
    group.raid = false;
    assert(Commands::FinderProblem(&leader, raid).empty()); // ordinary party may prepare
    group.raid = true;
#ifdef RAID_FINDER_IP
    Player friendAtStart{99}; friendAtStart.stage = 0; players[99] = &friendAtStart;
    group.Add(friendAtStart);
    assert(catalogue().find("RAID\tbwl\t") == std::string::npos);
    assert(!Commands::FinderProblem(&leader, byId("bwl")).empty());
    // Alt bots on normal accounts keep their character gates; random/excluded accounts follow IP policy.
    friendAtStart.bot = true;
    assert(catalogue().find("RAID\tbwl\t") == std::string::npos);
    friendAtStart.random = true;
    assert(catalogue().find("RAID\tbwl\t") != std::string::npos);
    friendAtStart.random = false; friendAtStart.excluded = true;
    assert(catalogue().find("RAID\tbwl\t") != std::string::npos);
    players.erase(99);
    assert(catalogue().find("RAID\t") == std::string::npos); // unknown member is not assumed eligible
    group.slots.pop_back(); group.refs.pop_back();
#endif
    std::vector<Player> bots; bots.reserve(39);
    for (uint32 id = 2; id <= 40; ++id) {
        bots.push_back(Player{id}); auto& bot = bots.back(); bot.bot = bot.random = true;
        bot.role = id <= 3 ? Role::Tank : id <= 6 ? Role::Healer : Role::Ranged;
        players[id] = &bot; group.Add(bot); drafted[1].insert(id);
    }
    assert(Commands::FinderProblem(&leader, raid).empty());
    assert(!Commands::FinderProblem(&leader, raid, true).empty()); // not arrived
    // Validate destination type/map before offering or summoning anyone.
    objects.missing = true; assert(catalogue().find("RAID\t") == std::string::npos); objects.missing = false;
    objects.object.type = 0; assert(catalogue().find("RAID\t") == std::string::npos); objects.object.type = GAMEOBJECT_TYPE_MEETINGSTONE;
    objects.stone.mapid = 409; assert(catalogue().find("RAID\t") == std::string::npos); objects.stone.mapid = 0;
    bots.back().bot = bots.back().random = false;
    drafted[1].erase(40);
    ChatHandler friendHandler{&bots.back()};
    Commands::HandleFinder(&friendHandler, "summon", std::string("mc"));
    assert(Commands::FinderAtStone(&bots.back(), point())); // humans may gather in any order outdoors
    assert(!Commands::FinderAtStone(&leader, point())); // a friend's confirmation never moves another human
    Commands::HandleFinder(&handler, "summon", std::string("mc"));
    assert(Commands::FinderAtStone(&leader, point()));
    for (auto& bot : bots) assert(Commands::FinderAtStone(&bot, point()));
    assert(leader.map == 0 && leader.instance == 0); // no direct raid entry
    assert(!Commands::FinderProblem(&leader, raid, true).empty()); // Start Run still requires real entry
    Commands::HandleFinder(&handler, "rally", std::string("mc"));
    assert(leader.session.last.find("RESULT\trally\tmc\t1") != std::string::npos);
    // Simulate normal portal travel; the finder itself never sends this teleport.
    leader.map = 409; leader.instance = 10;
    for (auto& bot : bots) { bot.map = 409; bot.instance = 10; }
    assert(Commands::FinderProblem(&leader, raid, true).empty());
    int const beforeInsideRecruit = assembleCalls;
    Commands::HandleFinder(&handler, "assemble", std::string("mc"));
    assert(assembleCalls == beforeInsideRecruit && leader.session.last.find("Leave the instance") != std::string::npos);
    bots.back().instance = 999;
    assert(!Commands::FinderProblem(&leader, raid, true).empty());
    bots.back().instance = 10;
    bots[0].combat = true;
    assert(!Commands::FinderProblem(&leader, raid, true).empty()); bots[0].combat = false;
    bots[0].world = false;
    assert(!Commands::FinderProblem(&leader, raid).empty()); bots[0].world = true;
    bots[0].role = Role::Ranged;
    assert(!Commands::FinderProblem(&leader, raid, true).empty()); bots[0].role = Role::Tank;
    InstanceSave leaderSave{10}, otherSave{20};
    saves.binds[1] = {&leaderSave, true}; saves.binds[2] = {&otherSave, true};
    assert(Commands::FinderProblem(&leader, raid).find("different raid save") != std::string::npos);
    saves.binds.clear();
    // A random bot outside the raid drops a stale save; a human, a bot inside and a bot on the leader's save keep theirs.
    InstanceSave staleSave{30};
    bots[5].map = 0;
    saves.binds[bots[5].id] = {&staleSave, true}; saves.binds[bots[6].id] = {&staleSave, true};
    saves.binds[40] = {&staleSave, true}; saves.binds[1] = {&leaderSave, true}; saves.binds[bots[7].id] = {&leaderSave, true};
    assert(Commands::FinderClearBotSaves(&leader, raid) == 1);
    assert(!saves.binds.count(bots[5].id) && saves.binds.count(bots[6].id) && saves.binds.count(40) && saves.binds.count(bots[7].id));
    saves.binds.clear(); bots[5].map = 409;
    players.erase(40);
    assert(!Commands::FinderProblem(&leader, raid).empty()); players[40] = &bots.back();
    ChatHandler follower{&bots.back()};
    Commands::HandleFinder(&follower, "assemble", std::string("mc"));
    assert(bots.back().session.last.find("Only the raid leader") != std::string::npos);
    Commands::FinderTravelToStone(&leader, point());
    for (auto& bot : bots) Commands::FinderTravelToStone(&bot, point());
    bots[0].x = 0; bots[0].y = 0; bots[0].z = 0;
    drafted[1].erase(2);
    Commands::HandleFinder(&handler, "rally", std::string("mc"));
    assert(!Commands::FinderAtStone(&bots[0], point())); // never teleport a manually invited bot
    // A player alt is not moved even if a stale ownership entry remains.
    bots[1].random = false; bots[1].x = bots[1].y = bots[1].z = 0;
    Commands::HandleFinder(&handler, "rally", std::string("mc"));
    assert(!Commands::FinderAtStone(&bots[1], point()));
    bots[1].random = true; Commands::FinderTravelToStone(&bots[1], point());
    drafted[1].insert(2); bots[0].teleport = false;
    Commands::HandleFinder(&handler, "rally", std::string("mc"));
    assert(leader.session.last.find("could not be summoned") != std::string::npos);
    // Drafts follow raid leadership (mod-optimal-bot-raid moves them); rally
    // must use the new leader's list even after the former owner's entry is gone.
    drafted[40] = drafted[1]; drafted.erase(1);
    group.leader = 40;
    bots[0].teleport = true;
    Commands::HandleFinder(&friendHandler, "rally", std::string("mc"));
    assert(Commands::FinderAtStone(&bots[0], point()));
    drafted[1] = drafted[40]; drafted.erase(40); group.leader = 1;
    leader.map = leader.instance = 0; leader.x = leader.y = leader.z = 0; leader.teleport = false;
    Commands::HandleFinder(&handler, "summon", std::string("mc"));
    assert(leader.map == 0 && leader.session.last.find("Summon refused") != std::string::npos);
    // Size-specific offers: the 40-player Onyxia is for levels 60-70, the 10/25 versions for 80.
    group.slots.clear(); group.refs.clear(); leader.group = nullptr;
    leader.map = 0; leader.instance = 0; leader.teleport = true; leader.level = 60;
    assert(catalogue().find("RAID\tony40\t") != std::string::npos);
    assert(catalogue().find("RAID\tnaxx40\t") != std::string::npos);
    assert(catalogue().find("RAID\tony25\t") == std::string::npos);
    // The 40-player versions run in the 10-man heroic slot; MC has no other version.
    Commands::FinderSetDifficulty(&leader, byId("ony40"));
    assert(leader.raidDifficulty == RAID_DIFFICULTY_10MAN_HEROIC);
    assert(Commands::FinderMemberProblem(&leader, byId("ony40")).empty());
    assert(Commands::FinderMemberProblem(&leader, byId("mc")).find("10 Player raid") != std::string::npos);
    leader.raidDifficulty = RAID_DIFFICULTY_10MAN_NORMAL;
    assert(Commands::FinderMemberProblem(&leader, byId("naxx40")).find("(Heroic)") != std::string::npos);
    assert(catalogue().find("RAID\tkara\t") == std::string::npos); // level 70 raid
    leader.level = 80;
    assert(catalogue().find("RAID\tony40\t") == std::string::npos);
    assert(catalogue().find("RAID\tony25\t") != std::string::npos);
    assert(catalogue().find("RAID\tmc\t") != std::string::npos); // older raids stay open
    assert(catalogue().find("\tOnyxia's Lair\t249\t2\t80\t25\t2\t3\t20\t1") != std::string::npos);
    // A raid without a meeting stone gathers at its exit, outside the instance.
    auto icc = byId("icc25");
    auto exit = Commands::FinderStone(icc);
    assert(exit && !exit->stone && exit->map == 571 && exit->x == objects.icecrownExit.target_X);
    objects.icecrownExit.target_mapId = 631; assert(!Commands::FinderStone(icc)); objects.icecrownExit.target_mapId = 571;
    // 25-player raids set the solo leader's raid difficulty and check saves of that difficulty.
    Commands::FinderSetDifficulty(&leader, icc);
    assert(leader.raidDifficulty == RAID_DIFFICULTY_25MAN_NORMAL);
    assert(Commands::FinderMemberProblem(&leader, icc).empty());
    assert(Commands::FinderMemberProblem(&leader, byId("icc10")).find("10 Player") != std::string::npos);
    leader.raidDifficulty = RAID_DIFFICULTY_10MAN_NORMAL;
    Group raid25; raid25.Add(leader);
    Player bot25{50}; bot25.bot = bot25.random = true; players[50] = &bot25; raid25.Add(bot25);
    InstanceSave tenSave{40}, twentyFiveSave{41};
    saves.binds[50] = {&tenSave, true, RAID_DIFFICULTY_10MAN_NORMAL};
    assert(Commands::FinderClearBotSaves(&leader, icc) == 0); // a 10-player save doesn't block a 25-player run
    saves.binds[50] = {&twentyFiveSave, true, RAID_DIFFICULTY_25MAN_NORMAL};
    assert(Commands::FinderClearBotSaves(&leader, icc) == 1 && !saves.binds.count(50));
    Commands::FinderSetDifficulty(&leader, icc);
    assert(raid25.raidDifficulty == RAID_DIFFICULTY_25MAN_NORMAL && bot25.raidDifficulty == RAID_DIFFICULTY_25MAN_NORMAL);
    bot25.map = 631; raid25.SetRaidDifficulty(RAID_DIFFICULTY_10MAN_NORMAL);
    Commands::FinderSetDifficulty(&leader, icc);
    assert(raid25.raidDifficulty == RAID_DIFFICULTY_10MAN_NORMAL); // never while someone is inside
#ifdef RAID_FINDER_LEGACY
    // Summon to Stone is the Legacy perk; Gather Bots at the stone stays free.
    raid25.slots.clear(); raid25.refs.clear(); leader.group = nullptr; leader.level = 60;
    Group perkGroup; perkGroup.Add(leader);
    Player helper{60}; helper.bot = helper.random = true; players[60] = &helper; perkGroup.Add(helper);
    drafted[1] = { 60 };
    leader.perk = false;
    leader.session.packets.clear();
    Commands::HandleFinder(&handler, "status", std::string("mc"));
    assert(leader.session.packets.front().find("\tNo personal raid save.\t0\tstone") != std::string::npos);
    Commands::HandleFinder(&handler, "summon", std::string("mc"));
    assert(leader.session.last.find("Legacy perk Summoning Stone") != std::string::npos && !Commands::FinderAtStone(&leader, point()));
    Commands::FinderTravelToStone(&leader, point());
    Commands::HandleFinder(&handler, "rally", std::string("mc"));
    assert(leader.session.last.find("RESULT\trally\tmc\t1") != std::string::npos);
    assert(Commands::FinderAtStone(&helper, point())); // the stone still gathers drafted bots
    Legacy::legacyConfig.enabled = false;
    leader.map = 1;
    Commands::HandleFinder(&handler, "summon", std::string("mc"));
    assert(leader.session.last.find("Legacy perk") == std::string::npos); // Legacy off: summon is free
    Legacy::legacyConfig.enabled = true; leader.perk = true;
#endif
    std::cout << "PASS: progression, exterior stone validation, human consent, full bot gathering, no instance teleport, portal readiness, roles, saves, ownership, refusal, sizes, exits, difficulty, Legacy perk\n";
}
