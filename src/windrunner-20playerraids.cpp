/*
 * windrunner-20playerraids
 *
 * Turns the 40-player raids into 20-player raids for a single-player server:
 *  - caps entry to Raid20.MaxPlayers (default 20),
 *  - gives every creature on those maps Raid20.HealthPercent of its template health,
 *  - bypasses the player-count autoscaler on those maps,
 *  - switches the encounter tweaks in the core raid scripts on (via RaidSizeOverride).
 *
 * Settings are read once at startup; changes need a restart.
 */

#include "ScriptObjects.h"
#include "Config/Config.h"
#include "Log.h"
#include "Map.h"
#include "Database/SQLStorages.h"
#include "RaidSizeOverride.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    char const* const DefaultMaps = "44,45,249,409,469,531,533,807,814";

    std::vector<uint32> ParseMapList(std::string const& text)
    {
        std::vector<uint32> maps;
        std::stringstream stream(text);
        std::string token;
        while (std::getline(stream, token, ','))
        {
            size_t const begin = token.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos)
                continue;

            try
            {
                size_t consumed = 0;
                unsigned long long const mapId = std::stoull(token.substr(begin), &consumed);
                if (token[begin] < '0' || token[begin] > '9' ||
                    token.find_first_not_of(" \t\r\n", begin + consumed) != std::string::npos ||
                    mapId > std::numeric_limits<uint32>::max())
                {
                    sLog.outError("[windrunner-20playerraids] Ignoring invalid map id '%s' in Raid20.Maps.", token.c_str());
                    continue;
                }

                maps.push_back(static_cast<uint32>(mapId));
            }
            catch (std::exception const&)
            {
                sLog.outError("[windrunner-20playerraids] Ignoring invalid map id '%s' in Raid20.Maps.", token.c_str());
            }
        }
        return maps;
    }

    class Raid20WorldScript : public WorldScript
    {
    public:
        Raid20WorldScript()
            : WorldScript("windrunner-20playerraids_world", { WORLDHOOK_ON_STARTUP })
        {
        }

        void OnStartup() override
        {
            if (!sConfig.GetBoolDefault("Raid20.Enable", false))
            {
                sLog.outString("[windrunner-20playerraids] Disabled (Raid20.Enable = 0).");
                return;
            }

            int32 const maxPlayers = sConfig.GetIntDefault("Raid20.MaxPlayers", 20);
            float const healthPercent = sConfig.GetFloatDefault("Raid20.HealthPercent", 50.0f);

            if (maxPlayers < 2 || !std::isfinite(healthPercent) || healthPercent <= 0.0f || healthPercent > 100.0f)
            {
                sLog.outError("[windrunner-20playerraids] Invalid settings (MaxPlayers %d, HealthPercent %.1f). Module stays off.", maxPlayers, healthPercent);
                return;
            }

            float const healthMultiplier = healthPercent / 100.0f;
            uint32 applied = 0;

            for (uint32 mapId : ParseMapList(sConfig.GetStringDefault("Raid20.Maps", DefaultMaps)))
            {
                MapEntry const* entry = sMapStorage.LookupEntry<MapEntry>(mapId);
                if (!entry)
                {
                    sLog.outError("[windrunner-20playerraids] Map %u not found in map_template, skipped.", mapId);
                    continue;
                }

                if (!entry->IsRaid())
                {
                    sLog.outError("[windrunner-20playerraids] Map %u (%s) is not a raid (type %u), skipped.", mapId, entry->name, entry->mapType);
                    continue;
                }

                uint32 const originalMaxPlayers = entry->maxPlayers;
                if (originalMaxPlayers <= static_cast<uint32>(maxPlayers))
                {
                    sLog.outError("[windrunner-20playerraids] Map %u (%s) already allows only %u players, skipped.", mapId, entry->name, originalMaxPlayers);
                    continue;
                }

                if (originalMaxPlayers != 40)
                    sLog.outError("[windrunner-20playerraids] Map %u (%s) has player limit %u (expected 40); reducing anyway.", mapId, entry->name, originalMaxPlayers);

                // The map template is loaded once and never reloaded, and no instance
                // exists yet at startup, so changing the limit in place is safe.
                const_cast<MapEntry*>(entry)->maxPlayers = static_cast<uint32>(maxPlayers);
                sRaidSizeOverride.Register(mapId, originalMaxPlayers, static_cast<uint32>(maxPlayers), healthMultiplier);
                ++applied;

                sLog.outString("[windrunner-20playerraids] %s (map %u): %u -> %d players, %.0f%% creature health.",
                    entry->name, mapId, originalMaxPlayers, maxPlayers, healthPercent);
            }

            sLog.outString("[windrunner-20playerraids] Enabled on %u raid map(s).", applied);
        }
    };
}

void Addwindrunner_20playerraidsScripts()
{
    new Raid20WorldScript();
}

// CMake calls Add<directory>Scripts(). This alias keeps the current
// modules/mod-20playerraids checkout building until that folder is renamed.
void Addmod_20playerraidsScripts()
{
    Addwindrunner_20playerraidsScripts();
}
