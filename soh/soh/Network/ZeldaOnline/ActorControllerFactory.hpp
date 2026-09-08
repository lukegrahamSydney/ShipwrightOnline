#ifndef ACTORCONTROLLERFACTORYH
#define ACTORCONTROLLERFACTORYH

#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include "AbstractActorController.hpp"
#include "ActorControllers/DekuBabaController.hpp"
#include "ActorControllers/KareBabaController.hpp"
#include "ActorControllers/GoroiwaController.hpp"
#include "ActorControllers/LiftController.hpp"
#include "ActorControllers/SkulltulaController.hpp"
#include "ActorControllers/SkullWalltulaController.hpp"
#include "ActorControllers/DogController.hpp"
#include "ActorControllers/MarketNpcController.hpp"
#include "ActorControllers/DekuScrubController.hpp"
#include "ActorControllers/NutsballController.hpp"
#include "ActorControllers/BossGomaController.hpp"
#include "ActorControllers/ENGomaController.hpp"
#include "ActorControllers/HintScrubController.hpp"
#include "ActorControllers/BgYdanHasiController.hpp"
#include "ActorControllers/PushBlockController.hpp"
#include "ActorControllers/CarpenterController.hpp"
#include "ActorControllers/TektiteController.hpp"
#include "ActorControllers/TorchController.hpp"
#include "ActorControllers/BossDodongoController.hpp"
#include "ActorControllers/BgDdanJdController.hpp"
#include "ActorControllers/BeamosController.hpp"
#include "ActorControllers/BabyDodongoController.hpp"
#include "ActorControllers/ArmosController.hpp"
#include "ActorControllers/KeeseController.hpp"
#include "ActorControllers/LizalfosController.hpp"
#include "ActorControllers/DodongoController.hpp"
#include "ActorControllers/TrapController.hpp"
#include "ActorControllers/HeishiController.hpp"
#include "ActorControllers/StalchildController.hpp"
#include "ActorControllers/BombController.hpp"
#include "ActorControllers/PotController.hpp"
#include "ActorControllers/BushController.hpp"
#include "ActorControllers/RockController.hpp"
#include "ActorControllers/NormalHorseController.hpp"
#include "ActorControllers/BombFlowerController.hpp"
#include "ActorControllers/ShopScrubController.hpp"
#include "ActorControllers/ShopnutsController.hpp"
#include "ActorControllers/WoodenCrateController.hpp"
#include "ActorControllers/CuccoController.hpp"
#include "ActorControllers/AttackingCuccoController.hpp"
#include "ActorControllers/PoeController.hpp"
#include "ActorControllers/GuayController.hpp"
#include "ActorControllers/WolfosController.hpp"
#include "ActorControllers/StalfosController.hpp"
#include "ActorControllers/OctorokController.hpp"
#include "ActorControllers/BiliController.hpp"
#include "ActorControllers/BubbleController.hpp"
#include "ActorControllers/BdanObjectsController.hpp"
#include "ActorControllers/BigokutaController.hpp"
#include "ActorControllers/RutoController.hpp"
#include "ActorControllers/ValiController.hpp"
#include "ActorControllers/TailpasaranController.hpp"
#include "ActorControllers/EiyerController.hpp"
#include "ActorControllers/BrobController.hpp"
#include "ActorControllers/BaController.hpp"
#include "ActorControllers/BarinadeController.hpp"
#include "ActorControllers/RedeadController.hpp"
#include "ActorControllers/RollingGoronController.hpp"
#include "ActorControllers/PeahatController.hpp"
#include "ActorControllers/SmallCrateController.hpp"
#include "ActorControllers/FlyingSkullController.hpp"
#include "ActorControllers/WallmasterController.hpp"
#include "ActorControllers/FloormasterController.hpp"
#include "ActorControllers/LikeLikeController.hpp"
#include "ActorControllers/DeadHandController.hpp"
#include "ActorControllers/DeadHandArmController.hpp"
#include "ActorControllers/MilkCrateController.hpp"
#include "ActorControllers/TalonController.hpp"
#include "ActorControllers/GravestoneController.hpp"
#include "ActorControllers/DampeController.hpp"
#include "ActorControllers/GraveyardKidController.hpp"
#include "ActorControllers/BowlWallController.hpp"
#include "ActorControllers/BombchuController.hpp"
#include "ActorControllers/MoriBigstController.hpp"
#include "ActorControllers/MoblinController.hpp"
#include "ActorControllers/PoeSistersController.hpp"
#include "ActorControllers/MoriHashira4Controller.hpp"
#include "ActorControllers/PoePaintingController.hpp"
#include "ActorControllers/MoriRakkatenjoController.hpp"
#include "ActorControllers/MoriElevatorController.hpp"
#include "ActorControllers/PhantomGanonController.hpp"
#include "ActorControllers/PhantomHorseController.hpp"
#include "ActorControllers/PhantomFireController.hpp"
#include "ActorControllers/MoriKaitenkabeController.hpp"
#include "ActorControllers/HidanSimaController.hpp"
#include "ActorControllers/HidanRockController.hpp"
#include "ActorControllers/FlyingTileController.hpp"
#include "ActorControllers/TorchSlugController.hpp"
#include "ActorControllers/HidanCurtainController.hpp"
#include "ActorControllers/HidanFsliftController.hpp"
#include "ActorControllers/HidanFirewallController.hpp"
#include "ActorControllers/HidanFwbigController.hpp"
#include "ActorControllers/HidanSyokuController.hpp"
#include "ActorControllers/FlareDancerController.hpp"
#include "ActorControllers/FlareDancerCoreController.hpp"
#include "ActorControllers/HidanDalmController.hpp"
#include "ActorControllers/HidanHamstepController.hpp"
#include "ActorControllers/VolvagiaController.hpp"
#include "ActorControllers/VolvagiaHoleController.hpp"
#include "ActorControllers/HidanHrockController.hpp"
#include "ActorControllers/HidanRsekizouController.hpp"
#include "ActorControllers/ShellbladeController.hpp"
#include "ActorControllers/MizuMovebgController.hpp"
#include "ActorControllers/DarkLinkController.hpp"
#include "ActorControllers/BlkobjController.hpp"
#include "ActorControllers/SiofukiController.hpp"
#include "ActorControllers/MizuWaterController.hpp"
#include "ActorControllers/ClamController.hpp"
#include "ActorControllers/StingerController.hpp"
#include "ActorControllers/MorphaController.hpp"
#include "ActorControllers/FreezardController.hpp"
#include "ActorControllers/IcicleController.hpp"
#include "ActorControllers/IceBlockController.hpp"
#include "ActorControllers/ScytheTrapController.hpp"
#include "ActorControllers/FlyingPotController.hpp"
#include "ActorControllers/TruthSpinnerController.hpp"
#include "ActorControllers/HakaTrapController.hpp"
#include "ActorControllers/HakaPlatformController.hpp"
#include "ActorControllers/SkullJarController.hpp"
#include "ActorControllers/ShadowShipController.hpp"
#include "ActorControllers/BongoBongoController.hpp"
#include "ActorControllers/Ge2Controller.hpp"
#include "ActorControllers/DaikuController.hpp"
#include "ActorControllers/GeldBController.hpp"
#include "ActorControllers/BongoFloorController.hpp"

#include "ActorControllers/PondFishController.hpp"
#include "ActorControllers/JyaGoroiwaController.hpp"
#include "ActorControllers/JyaCobraController.hpp"
#include "ActorControllers/IkController.hpp"
#include "ActorControllers/JyaIronobjController.hpp"
#include "ActorControllers/JyaZurerukabeController.hpp"
#include "ActorControllers/TwinrovaController.hpp"
#include "ActorControllers/TrController.hpp"
#include "ActorControllers/DoorKillerController.hpp"
#include "ActorControllers/AnubiceController.hpp"
#include "ActorControllers/AnubiceTagController.hpp"

#include "ActorControllers/ObjLightswitchController.hpp"
#include "ActorControllers/ReebaController.hpp"

#include "ActorControllers/GanonOtyukaController.hpp"
#include "ActorControllers/GanondorfController.hpp"
#include "ActorControllers/Zl3Controller.hpp"
#include "ActorControllers/HeavyBlockController.hpp"
#include "ActorControllers/GndIceblockController.hpp"

#include "ActorControllers/GndFiremeiroController.hpp"
#include "ActorControllers/GanonController.hpp"
#include "ActorControllers/GanonOrganController.hpp"

namespace ZeldaOnline {

template <typename T, typename = void> struct HasRegisterHooks : std::false_type {};

template <typename T>
struct HasRegisterHooks<T, std::void_t<decltype(T::RegisterHooks(std::declval<s16>(), std::declval<bool>()))>>
    : std::true_type {};

class ActorControllerFactory {
  public:
    using SpawnPredicate = bool (*)(s16 params);
    using CreateFn = AbstractActorController* (*)(Actor*, int, int, int, bool);

    static ActorControllerFactory& Instance() {
        static ActorControllerFactory sInstance;
        return sInstance;
    }

    bool IsNetworked(s16 actorID, s16 params) const {
        return FindEntry(actorID, params) != nullptr;
    }

    AbstractActorController* Create(s16 actorID, s16 params, Actor* actor, int networkID, int sceneKey, int roomIndex,
                                    bool isLeader) const {
        const FactoryEntry* entry = FindEntry(actorID, params);
        if (entry == nullptr) {
            return nullptr;
        }
        return entry->create(actor, networkID, sceneKey, roomIndex, isLeader);
    }

    AbstractActorController* Create(s16 actorID, Actor* actor, int networkID, int sceneKey, int roomIndex,
                                    bool isLeader) const {
        return Create(actorID, actor != nullptr ? actor->params : (s16)0, actor, networkID, sceneKey, roomIndex,
                      isLeader);
    }

    void RegisterActorHooks(bool enabled) {
        for (auto& pair : m_entries) {
            for (size_t i = 0; i < pair.second.size(); i++) {
                bool alreadyHooked = false;
                for (size_t j = 0; j < i; j++) {
                    if (pair.second[j].hooks == pair.second[i].hooks) {
                        alreadyHooked = true;
                        break;
                    }
                }
                if (!alreadyHooked) {
                    pair.second[i].hooks(pair.first, enabled);
                }
            }
        }
    }

  private:
    using HooksFn = void (*)(s16, bool);

    struct FactoryEntry {
        CreateFn create;
        SpawnPredicate predicate;
        HooksFn hooks;
    };

    template <typename T> static void InvokeRegisterHooks(s16 actorID, bool enabled) {
        if constexpr (HasRegisterHooks<T>::value) {
            T::RegisterHooks(actorID, enabled);
        }
    }

    template <typename T>
    static AbstractActorController* CreateController(Actor* actor, int networkID, int sceneKey, int roomIndex,
                                                     bool isLeader) {
        AbstractActorController* controller = new T(actor, networkID, sceneKey, roomIndex, isLeader);
        return controller;
    }

    const FactoryEntry* FindEntry(s16 actorID, s16 params) const {
        auto it = m_entries.find(actorID);
        if (it == m_entries.end()) {
            return nullptr;
        }
        const FactoryEntry* fallback = nullptr;
        for (const auto& entry : it->second) {
            if (entry.predicate == nullptr) {
                if (fallback == nullptr) {
                    fallback = &entry;
                }
                continue;
            }
            if (entry.predicate(params)) {
                return &entry;
            }
        }
        return fallback;
    }

    template <typename T> void Register(s16 actorID, SpawnPredicate predicate = nullptr) {
        m_entries[actorID].push_back({ &CreateController<T>, predicate, &InvokeRegisterHooks<T> });
    }

    ActorControllerFactory() {
        // Pretty self explanatory. You register the controller here which allows it to be synced online. Without this
        // one line You will only create a local copy and nobody else will see the syncronization. The template param is
        // the name of the controller class, and param1 is the the ActorID. You can also Make it only work for actors
        // that have a specific param. Look at the ones on the bottom to see how
        Register<DekuBabaController>(ACTOR_EN_DEKUBABA);
        Register<KareBabaController>(ACTOR_EN_KAREBABA);
        Register<GoroiwaController>(ACTOR_EN_GOROIWA);
        Register<LiftController>(ACTOR_OBJ_LIFT);
        Register<SkulltulaController>(ACTOR_EN_ST);
        Register<DogController>(ACTOR_EN_DOG);
        Register<MarketNpcController>(ACTOR_EN_HY);
        Register<BossGomaController>(ACTOR_BOSS_GOMA);
        Register<BgYdanHasiController>(ACTOR_BG_YDAN_HASI);
        Register<PushBlockController>(ACTOR_OBJ_OSHIHIKI);
        Register<CarpenterController>(ACTOR_EN_DAIKU_KAKARIKO);
        Register<TektiteController>(ACTOR_EN_TITE);
        Register<TorchController>(ACTOR_OBJ_SYOKUDAI);
        Register<BossDodongoController>(ACTOR_BOSS_DODONGO);
        Register<BgDdanJdController>(ACTOR_BG_DDAN_JD);
        Register<BeamosController>(ACTOR_EN_VM);
        Register<BabyDodongoController>(ACTOR_EN_DODOJR);
        Register<ArmosController>(ACTOR_EN_AM);
        Register<KeeseController>(ACTOR_EN_FIREFLY);
        Register<LizalfosController>(ACTOR_EN_ZF);
        Register<DodongoController>(ACTOR_EN_DODONGO);
        Register<TrapController>(ACTOR_EN_TRAP);
        Register<HeishiController>(ACTOR_EN_HEISHI1);
        Register<StalchildController>(ACTOR_EN_SKB);
        Register<NormalHorseController>(ACTOR_EN_HORSE_NORMAL);
        Register<BombController>(ACTOR_EN_BOM);
        Register<PotController>(ACTOR_OBJ_TSUBO);
        Register<BushController>(ACTOR_EN_KUSA);
        Register<RockController>(ACTOR_EN_ISHI);
        Register<WoodenCrateController>(ACTOR_OBJ_KIBAKO2);
        Register<CuccoController>(ACTOR_EN_NIW);
        Register<PoeController>(ACTOR_EN_POH);
        Register<AttackingCuccoController>(ACTOR_EN_ATTACK_NIW);
        Register<BombFlowerController>(ACTOR_EN_BOMBF);
        Register<GuayController>(ACTOR_EN_CROW);
        Register<WolfosController>(ACTOR_EN_WF);
        Register<StalfosController>(ACTOR_EN_TEST);
        Register<OctorokController>(ACTOR_EN_OKUTA);
        Register<ShopScrubController>(ACTOR_EN_DNS);
        Register<BiliController>(ACTOR_EN_BILI);
        Register<ShopnutsController>(ACTOR_EN_SHOPNUTS);
        Register<BubbleController>(ACTOR_EN_BUBBLE);
        Register<BdanObjectsController>(ACTOR_BG_BDAN_OBJECTS);
        Register<BigokutaController>(ACTOR_EN_BIGOKUTA);

        Register<ValiController>(ACTOR_EN_VALI);
        Register<EiyerController>(ACTOR_EN_EIYER);
        Register<BrobController>(ACTOR_EN_BROB);
        Register<RedeadController>(ACTOR_EN_RD);
        Register<PeahatController>(ACTOR_EN_PEEHAT);
        Register<SmallCrateController>(ACTOR_OBJ_KIBAKO);
        Register<LikeLikeController>(ACTOR_EN_RR);
        Register<DeadHandController>(ACTOR_EN_DH);
        Register<DeadHandArmController>(ACTOR_EN_DHA);
        Register<MilkCrateController>(ACTOR_BG_SPOT15_RRBOX);
        Register<GravestoneController>(ACTOR_BG_HAKA);
        Register<DampeController>(ACTOR_EN_TK);
        Register<GraveyardKidController>(ACTOR_EN_CS);
        Register<BowlWallController>(ACTOR_BG_BOWL_WALL);
        Register<ReebaController>(ACTOR_EN_REEBA);
        Register<MoblinController>(ACTOR_EN_MB);
        Register<MoriBigstController>(ACTOR_BG_MORI_BIGST);
        Register<MoriElevatorController>(ACTOR_BG_MORI_ELEVATOR);
        Register<PhantomFireController>(ACTOR_EN_FHG_FIRE);

        Register<HakaTrapController>(ACTOR_BG_HAKA_TRAP);
        Register<HidanSimaController>(ACTOR_BG_HIDAN_SIMA);
        Register<HidanRockController>(ACTOR_BG_HIDAN_ROCK);
        Register<FlyingTileController>(ACTOR_EN_YUKABYUN);
        Register<TorchSlugController>(ACTOR_EN_BW);
        Register<HidanCurtainController>(ACTOR_BG_HIDAN_CURTAIN);
        Register<MoriKaitenkabeController>(ACTOR_BG_MORI_KAITENKABE);
        Register<HidanFsliftController>(ACTOR_BG_HIDAN_FSLIFT);
        // Register<HidanFirewallController>(ACTOR_BG_HIDAN_FIREWALL);
        Register<HidanFwbigController>(ACTOR_BG_HIDAN_FWBIG);
        Register<HidanSyokuController>(ACTOR_BG_HIDAN_SYOKU);
        Register<FlareDancerController>(ACTOR_EN_FD);
        Register<FlareDancerCoreController>(ACTOR_EN_FW);
        Register<HidanDalmController>(ACTOR_BG_HIDAN_DALM);
        Register<HidanHamstepController>(ACTOR_BG_HIDAN_HAMSTEP);
        Register<HidanHrockController>(ACTOR_BG_HIDAN_HROCK);
        Register<HidanRsekizouController>(ACTOR_BG_HIDAN_RSEKIZOU);
        Register<ShellbladeController>(ACTOR_EN_NY);
        Register<RutoController>(ACTOR_EN_RU1,
                                 [](s16 params) { return gPlayState && gPlayState->sceneNum == SCENE_JABU_JABU; });

        Register<MizuMovebgController>(ACTOR_BG_MIZU_MOVEBG,
                                       [](s16 params) { return MizuMovebgController::IsNetworkedVariant(params); });
        Register<ClamController>(ACTOR_EN_SB);
        Register<StingerController>(ACTOR_EN_WEIYER);
        Register<MorphaController>(ACTOR_BOSS_MO);

        Register<HakaPlatformController>(ACTOR_BG_HAKA_MEGANEBG);
        Register<FlyingPotController>(ACTOR_EN_TUBO_TRAP);
        Register<ScytheTrapController>(ACTOR_BG_HAKA_SGAMI);
        Register<IceBlockController>(ACTOR_BG_ICE_OBJECTS);
        Register<FreezardController>(ACTOR_EN_FZ);
        Register<DarkLinkController>(ACTOR_EN_TORCH2);
        Register<SiofukiController>(ACTOR_EN_SIOFUKI);
        // Register<MizuWaterController>(ACTOR_BG_MIZU_WATER);
        Register<IcicleController>(ACTOR_BG_ICE_TURARA);
        Register<BlkobjController>(ACTOR_EN_BLKOBJ);
        Register<VolvagiaController>(ACTOR_BOSS_FD);
        Register<VolvagiaHoleController>(ACTOR_BOSS_FD2);
        Register<TruthSpinnerController>(ACTOR_BG_HAKA_GATE);
        Register<SkullJarController>(ACTOR_BG_HAKA_TUBO);
        Register<ShadowShipController>(ACTOR_BG_HAKA_SHIP);
        Register<BongoBongoController>(ACTOR_BOSS_SST);
        Register<Ge2Controller>(ACTOR_EN_GE2);
        Register<Zl3Controller>(ACTOR_EN_ZL3);
        Register<GanonController>(ACTOR_BOSS_GANON2);
        Register<GndIceblockController>(ACTOR_BG_GND_ICEBLOCK);
        Register<GanonOrganController>(ACTOR_EN_GANON_ORGAN);
        Register<BongoFloorController>(ACTOR_BG_SST_FLOOR);

        Register<GndFiremeiroController>(ACTOR_BG_GND_FIREMEIRO,
                                         [](s16 params) { return GndFiremeiroController::IsNetworkedVariant(params); });
        Register<HeavyBlockController>(ACTOR_BG_HEAVY_BLOCK,
                                       [](s16 params) { return HeavyBlockController::IsNetworkedVariant(params); });

        Register<GanonOtyukaController>(ACTOR_BG_GANON_OTYUKA,
                                        [](s16 params) { return GanonOtyukaController::IsNetworkedVariant(params); });

        Register<AnubiceController>(ACTOR_EN_ANUBICE);
        Register<AnubiceTagController>(ACTOR_EN_ANUBICE_TAG);
        
        Register<ObjLightswitchController>(ACTOR_OBJ_LIGHTSWITCH);

        Register<DoorKillerController>(ACTOR_DOOR_KILLER,
                                       [](s16 params) { return DoorKillerController::IsNetworkedVariant(params); });
        // Register<DaikuController>(ACTOR_EN_DAIKU);
        Register<GeldBController>(ACTOR_EN_GELDB);
        Register<JyaGoroiwaController>(ACTOR_BG_JYA_GOROIWA);
        Register<JyaCobraController>(ACTOR_BG_JYA_COBRA,
                                     [](s16 params) { return JyaCobraController::IsNetworkedVariant(params); });
        Register<IkController>(ACTOR_EN_IK);
        Register<JyaIronobjController>(ACTOR_BG_JYA_IRONOBJ);
        Register<JyaZurerukabeController>(ACTOR_BG_JYA_ZURERUKABE);
        Register<TwinrovaController>(ACTOR_BOSS_TW);
        Register<TrController>(ACTOR_EN_TR);
        Register<GanondorfController>(ACTOR_BOSS_GANON,
                                      [](s16 params) { return GanondorfController::IsNetworkedVariant(params); });

        Register<PondFishController>(ACTOR_FISHING,
                                     [](s16 params) { return PondFishController::IsNetworkedVariant(params); });

        Register<PhantomGanonController>(ACTOR_BOSS_GANONDROF,
                                         [](s16 params) { return PhantomGanonController::IsNetworkedVariant(params); });

        Register<PhantomHorseController>(ACTOR_EN_FHG,
                                         [](s16 params) { return PhantomHorseController::IsNetworkedVariant(params); });
        Register<PoeSistersController>(ACTOR_EN_PO_SISTERS,
                                       [](s16 params) { return PoeSistersController::IsNetworkedVariant(params); });
        Register<BombchuController>(ACTOR_EN_BOM_CHU);
        Register<MoriHashira4Controller>(ACTOR_BG_MORI_HASHIRA4);
        Register<PoePaintingController>(ACTOR_BG_PO_EVENT);

        Register<MoriRakkatenjoController>(ACTOR_BG_MORI_RAKKATENJO);
        Register<TalonController>(ACTOR_EN_TA, [](s16 params) { return TalonController::IsNetworkedVariant(params); });

        Register<FlyingSkullController>(ACTOR_EN_BB,
                                        [](s16 params) { return FlyingSkullController::IsNetworkedVariant(params); });
        Register<WallmasterController>(ACTOR_EN_WALLMAS);
        Register<FloormasterController>(ACTOR_EN_FLOORMAS);
        Register<RollingGoronController>(ACTOR_EN_GO2,
                                         [](s16 params) { return RollingGoronController::IsNetworkedVariant(params); });

        Register<BarinadeController>(ACTOR_BOSS_VA,
                                     [](s16 params) { return BarinadeController::IsNetworkedVariant(params); });

        Register<BaController>(ACTOR_EN_BA, [](s16 params) { return BaController::IsNetworkedVariant(params); });

        Register<TailpasaranController>(ACTOR_EN_TP,
                                        [](s16 params) { return TailpasaranController::IsNetworkedVariant(params); });
        Register<DekuScrubController>(ACTOR_EN_DEKUNUTS,
                                      [](s16 params) { return DekuScrubController::IsNetworkedVariant(params); });
        Register<NutsballController>(ACTOR_EN_NUTSBALL);

        Register<SkullWalltulaController>(ACTOR_EN_SW, [](s16 params) { return ((params & 0xE000) >> 13) == 0; });

        Register<EnGomaController>(ACTOR_EN_GOMA, [](s16 p) { return EnGomaController::IsNetworkedVariant(p); });
        Register<HintScrubController>(ACTOR_EN_HINTNUTS,
                                      [](s16 p) { return HintScrubController::IsNetworkedVariant(p); });
    }

    std::unordered_map<s16, std::vector<FactoryEntry>> m_entries;
};
} // namespace ZeldaOnline

#endif