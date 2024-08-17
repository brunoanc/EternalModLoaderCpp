/*
* This file is part of EternalModLoaderCpp (https://github.com/PowerBall253/EternalModLoaderCpp).
* Copyright (C) 2021 PowerBall253
*
* EternalModLoaderCpp is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* EternalModLoaderCpp is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with EternalModLoaderCpp. If not, see <https://www.gnu.org/licenses/>.
*/

#include <filesystem>
#include <climits>
#include "ProgramOptions.hpp"
#include "Utils.hpp"
#include "OnlineSafetySWF.hpp"
#include "OnlineSafety.hpp"

namespace fs = std::filesystem;

// New "Public Match" menu label and "Private Match" descriptive text (BattleMode)
static const std::map<std::string, std::tuple<std::string, std::string>> Languages = {
    {
        "english",
        std::make_tuple(
            "^8NO MODS IN PUBLIC MATCHES",
            "Play BATTLEMODE matches with the players in your party\n^3Public matchmaking is unavailable when using mods that might affect gameplay\nOnly private matches are allowed"
        )
    },
    {
        "french",
        std::make_tuple(
            "^8MATCH EN LIGNE (DÉSACTIVÉ)",
            "Jouez à des matchs BATTLEMODE avec les joueurs de votre groupe.\n^3Le matchmaking publique n'est pas disponible lors de l' utilisation de mod(s) affectant le gameplay. Seul les matchs privés sont autorisés."
        )
    },
    {
        "german",
        std::make_tuple(
            "^8ONLINE-MATCH (DEAKTIVIERT)",
            "Spiele BATTLE-MODUS-Matches mit den Spielern in deiner Gruppe.\n^3Öffentliches Matchmaking nicht verfügbar, während Modifikationen genutzt werden welche Auswirkungen auf den Spielverlauf haben."
        )
    },
    {
        "italian",
        std::make_tuple(
            "^8PARTITA ONLINE (DISABILITATA)",
            "Gioca in partite di BATTLEMODE con i giocatori del tuo gruppo\n^3Il matchmaking pubblico non è disponibile quando si usano mod che potrebbero influenzare il gameplay. Sono permesse solo in partite private."
        )
    },
    {
        "japanese",
        std::make_tuple(
            "^8NO MODS IN PUBLIC MATCHES",
            "パーティー内のプレイヤーとバトルモードのマッチをプレイする\n^3Public matchmaking is unavailable when using mods that might affect gameplay\nOnly private matches are allowed"
        )
    },
    {
        "korean",
        std::make_tuple(
            "^8NO MODS IN PUBLIC MATCHES",
            "파티에 속한 플레이어들과 전투 모드 플레이\n^3Public matchmaking is unavailable when using mods that might affect gameplay\nOnly private matches are allowed"
        )
    },
    {
        "latin_spanish",
        std::make_tuple(
            "^8PARTIDA ONLINE (DESHABILITADA)",
            "Disputa partidas de BATTLEMODE con jugadores de tu grupo.\n^3Las partidas públicas serán desactivadas al usar mods que alteren el gameplay.\nSolo partidas privadas serán permitidas."
        )
    },
    {
        "polish",
        std::make_tuple(
            "^8GRAJ ONLINE (WYŁĄCZONE)",
            "Graj w trybie BATTLEMODE z graczami z twojej grupy.\n^3Publiczne mecze są zablokowane podczas korzystania z modów, które mogą wpłynąć na rozgrywkę. Jedynie prywante mecze są dozwolone."
        )
    },
    {
        "portuguese",
        std::make_tuple(
            "^8JOGAR ONLINE (DESHABILITADO)",
            "Jogue partidas do BATTLEMODE com os jogadores do seu grupo.\n^3Partidas públicas estão desativadas quando se usam mods que podem afetar gameplay. Apenas partidas privadas são autorizadas."
        )
    },
    {
        "russian",
        std::make_tuple(
            "^8МАТЧ ПО СЕТИ (ОТКЛЮЧЕНО)",
            "Участвуйте в матчах в режиме BATTLEMODE с игроками вашего отряда\n^3Публичные матчи недоступны с модами, которые могут повлиять на геймплей\nРазрешены только приватные матчи"
        )
    },
    {
        "simplified_chinese",
        std::make_tuple(
            "^8NO MODS IN PUBLIC MATCHES",
            "与自己队伍的玩家玩战斗模式比赛\n^3Public matchmaking is unavailable when using mods that might affect gameplay\nOnly private matches are allowed"
        )
    },
    {
        "spanish",
        std::make_tuple(
            "^8PARTIDA ONLINE (DESHABILITADA)",
            "Disputa partidas de BATTLEMODE con jugadores de tu grupo.\n^3Las partidas públicas serán desactivadas al usar mods que alteren el gameplay.\nSolo partidas privadas serán permitidas."
        )
    },
    {
        "traditional_chinese",
        std::make_tuple(
            "^8NO MODS IN PUBLIC MATCHES",
            "與你隊伍中的玩家玩戰鬥模式對戰\n^3Public matchmaking is unavailable when using mods that might affect gameplay\nOnly private matches are allowed"
        )
    },
};

// Online safe keywords
static const std::vector<std::string> OnlineSafeModNameKeywords = {
    "/eternalmod/", ".tga", ".png", ".swf", ".bimage", "/advancedscreenviewshake/", "/audiolog/", "/audiologstory/", "/automap/", "/automapplayerprofile/",
    "/automapproperties/", "/automapsoundprofile/", "/env/", "/font/", "/fontfx/", "/fx/", "/gameitem/", "/globalfonttable/", "/gorebehavior/",
    "/gorecontainer/", "/gorewounds/", "/handsbobcycle/", "/highlightlos/", "/highlights/", "/hitconfirmationsoundsinfo/", "/hud/", "/hudelement/",
    "/lightrig/", "/lodgroup/", "/material2/", "/md6def/", "/modelasset/", "/particle/", "/particlestage/", "/renderlayerdefinition/", "/renderparm/",
    "/renderparmmeta/", "/renderprogflag/", "/ribbon2/", "/rumble/", "/soundevent/", "/soundpack/", "/soundrtpc/", "/soundstate/", "/soundswitch/",
    "/speaker/", "/staticimage/", "/swfresources/", "/uianchor/", "/uicolor/", "/weaponreticle/", "/weaponreticleswfinfo/", "/entitydef/light/", "/entitydef/fx",
    "/impacteffect/", "/uiweapon/", "/globalinitialwarehouse/", "/globalshell/", "/warehouseitem/", "/warehouseofflinecontainer/", "/tooltip/", "/livetile/",
    "/tutorialevent/", "maps/game/dlc/", "maps/game/dlc2/", "maps/game/horde/", "maps/game/hub/", "maps/game/shell/", "maps/game/sp/", "maps/game/tutorials/",
    "/decls/campaign/"
};

// Online unsafe resource names
static const std::vector<std::string> UnsafeResourceNameKeywords = {
    "gameresources", "pvp", "shell", "warehouse"
};

std::vector<ResourceModFile> GetMultiplayerDisablerMods()
{
    // Get multiplayer disabler mods
    Mod parentMod;
    parentMod.LoadPriority = INT_MIN;

    std::vector<ResourceModFile> multiplayerDisablerMods;
    multiplayerDisablerMods.reserve(1 + Languages.size());

    // Battlemode
    ResourceModFile multiplayerDisablerSwf(parentMod, "swf/hud/menus/battle_arena/play_online_screen.swf", "gameresources_patch2", false);
    multiplayerDisablerSwf.FileBytes = std::vector<std::byte>(reinterpret_cast<const std::byte*>(SWFData),
        reinterpret_cast<const std::byte*>(SWFData) + sizeof(SWFData));
    multiplayerDisablerMods.push_back(multiplayerDisablerSwf);

    // Localization for "Public Match" menu label and "Private Match" menu description
    for (auto& language : Languages) {
        std::string resourceName;

        if (!ProgramOptions::BlangFileContainerRedirect.empty()) {
            resourceName = ProgramOptions::BlangFileContainerRedirect;
        }
        else {
            resourceName = "gameresources_patch3";
        }

        ResourceModFile multiplayerDisablerBlang(parentMod, "EternalMod/strings/" + language.first + ".json", resourceName, false);
        multiplayerDisablerBlang.IsBlangJson = true;

        auto [publicMatchText, privateMatchText] = language.second;
        std::string blangJson = R"({"strings":[{"name":"#eternalmod_no_online_mods","text":")" + publicMatchText
            + R"("},{"name":"#str_decl_pvp_private_lobby_desc_GHOST71267","text":")" + privateMatchText + R"("}]})";

        multiplayerDisablerBlang.FileBytes = std::vector<std::byte>(reinterpret_cast<const std::byte*>(blangJson.c_str()),
            reinterpret_cast<const std::byte*>(blangJson.c_str()) + blangJson.length());
        multiplayerDisablerMods.push_back(multiplayerDisablerBlang);
    }

    return multiplayerDisablerMods;
}

bool IsModSafeForOnline(const std::map<size_t, std::vector<ResourceModFile>>& resourceModFiles)
{
    std::vector<ResourceModFile> assetsInfoJsons;

    for (const auto& resource : resourceModFiles) {
        // Skip resources with no mods
        // Shouldn't happen, just a failsafe
        if (resource.second.empty()) {
            continue;
        }

        // Check if current resource is unsafe for modifications
        bool isUnsafeResource = false;

        for (const auto& keyword : UnsafeResourceNameKeywords) {
            if (StartsWith(ToLower(resource.second[0].ResourceName), keyword)) {
                isUnsafeResource = true;
                break;
            }
        }

        // Iterate through mod files to check safety
        for (const auto& modFile : resource.second) {
            // Skip accidentally included OS files
            if (EndsWith(ToLower(modFile.Name), "desktop.ini") || EndsWith(ToLower(modFile.Name), ".ds_store")) {
                continue;
            }

            // Check assets info files last
            if (modFile.IsAssetsInfoJson) {
                assetsInfoJsons.push_back(modFile);
                continue;
            }

            // Files with .lwo extension are unsafe
            if (fs::path(modFile.Name).string().find(".lwo") != std::string::npos && isUnsafeResource) {
                return false;
            }

            // Allow modification of anything outside of "generated/decls/", except .entities files
            if (!StartsWith(ToLower(modFile.Name), "generated/decls/") && !EndsWith(ToLower(modFile.Name), ".entities")) {
                continue;
            }

            // Check if mod file is on whitelist
            bool found = false;

            for (const auto& keyword : OnlineSafeModNameKeywords) {
                if (ToLower(modFile.Name).find(keyword) != std::string::npos) {
                    found = true;
                    break;
                }
            }

            // Do not allow mods to modify non-whitelisted files in unsafe resources
            if (!found && isUnsafeResource) {
                return false;
            }
        }
    }

    // Don't allow adding unsafe mods in safe resource files into unsafe resources files
    // Otherwise, don't mark the mod as unsafe, it should be fine for single-player if
    // the mod is not modifying a critical resource
    for (const auto& assetsInfo : assetsInfoJsons) {
        if (assetsInfo.AssetsInfo.has_value()) {
            for (const auto& keyword : UnsafeResourceNameKeywords) {
                if (StartsWith(ToLower(assetsInfo.ResourceName), keyword)) {
                    return false;
                }
            }
        }
    }

    return true;
}
