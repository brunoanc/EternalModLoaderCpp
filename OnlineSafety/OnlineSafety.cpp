#include <algorithm>
#include <tuple>
#include <climits>
#include "OnlineSafetySWF.hpp"
#include "EternalModLoader.hpp"

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
    "/entitydef/", "/impacteffect/", "/uiweapon/", "/globalinitialwarehouse/", "/globalshell/", "/warehouseitem/", "/warehouseofflinecontainer/", "/tooltip/",
    "/livetile/", "/tutorialevent/", "/maps/game/dlc/", "/maps/game/dlc2/", "/maps/game/hub/", "/maps/game/shell/", "/maps/game/sp/", "/maps/game/tutorials/",
    "/decls/campaign"
};

// Online unsafe resource names
static const std::vector<std::string> UnsafeResourceNameKeywords = {
    "gameresources", "pvp", "shell", "warehouse"
};

/**
 * @brief Get the multiplayer disabler mods
 * 
 * @return Vector containing the multiplayer disabler mods
 */
std::vector<ResourceModFile> GetMultiplayerDisablerMods()
{
    // Get multiplayer disabler mods
    Mod parentMod;
    parentMod.LoadPriority = INT_MIN;

    std::vector<ResourceModFile> multiplayerDisablerMods;
    multiplayerDisablerMods.reserve(1 + Languages.size());

    // Battlemode
    ResourceModFile multiplayerDisablerSwf(parentMod, "swf/hud/menus/battle_arena/play_online_screen.swf", "gameresources_patch2", false);
    multiplayerDisablerSwf.FileBytes = std::vector<std::byte>((std::byte*)SWFData, (std::byte*)SWFData + sizeof(SWFData));
    multiplayerDisablerMods.push_back(multiplayerDisablerSwf);

    // Localization for "Public Match" menu label and "Private Match" menu description
    for (auto &language : Languages) {
        ResourceModFile multiplayerDisablerBlang(parentMod, "EternalMod/strings/" + language.first + ".json", "gameresources_patch1", false);
        multiplayerDisablerBlang.IsBlangJson = true;

        auto [publicMatchText, privateMatchText] = language.second;
        std::string blangJson = R"({"strings":[{"name":"#eternalmod_no_online_mods","text":")" + publicMatchText
            + R"("},{"name":"#str_decl_pvp_private_lobby_desc_GHOST71267","text":")" + privateMatchText + R"("}]})";

        multiplayerDisablerBlang.FileBytes = std::vector<std::byte>((std::byte*)blangJson.c_str(), (std::byte*)blangJson.c_str() + blangJson.length());
        multiplayerDisablerMods.push_back(multiplayerDisablerBlang);
    }

    return multiplayerDisablerMods;
}

/**
 * @brief Checks wether a given mod's files is safe for online use
 * 
 * @param resourceModFiles Mod's resource mod files
 * @return True if the mod is safe for online, false otherwise 
 */
bool IsModSafeForOnline(const std::map<int32_t, std::vector<ResourceModFile>> &resourceModFiles)
{
    bool isSafe = true;
    bool isModifyingUnsafeResource = false;
    std::vector<ResourceModFile> assetsInfoJsons;

    for (const auto &resource : resourceModFiles) {
        for (const auto &modFile : resource.second) {
            if (EndsWith(ToLower(modFile.Name), "desktop.ini") || EndsWith(ToLower(modFile.Name), ".ds_store")) {
                continue;
            }

            // Check assets info files last
            if (modFile.IsAssetsInfoJson) {
                assetsInfoJsons.push_back(modFile);
                continue;
            }

            for (const auto &keyword : UnsafeResourceNameKeywords) {
                if (StartsWith(ToLower(modFile.ResourceName), keyword)) {
                    isModifyingUnsafeResource = true;
                    break;
                }
            }

            // Files with .lwo extension are unsafe
            if (fs::path(modFile.Name).string().find(".lwo") != std::string::npos) {
                isSafe = false;
            }

            // Allow modification of anything outside of "generated/decls/"
            if (!StartsWith(ToLower(modFile.Name), "generated/decls/")) {
                continue;
            }

            if (isSafe) {
                bool found = false;

                for (const auto &keyword : OnlineSafeModNameKeywords) {
                    if (ToLower(modFile.Name).find(keyword) != std::string::npos) {
                        found = true;
                        break;
                    }
                }

                isSafe = found;
            }
        }
    }

    if (isSafe) {
        return true;
    }
    else if (isModifyingUnsafeResource) {
        return false;
    }

    // Don't allow adding unsafe mods in safe resource files into unsafe resources files
    // Otherwise, don't mark the mod as unsafe, it should be fine for single-player if
    // the mod is not modifying a critical resource
    for (const auto &assetsInfo : assetsInfoJsons) {
        if (assetsInfo.AssetsInfo.has_value()) {
            for (const auto &keyword : UnsafeResourceNameKeywords) {
                if (StartsWith(ToLower(assetsInfo.ResourceName), keyword)) {
                    return false;
                }
            }
        }
    }

    return true;
}
