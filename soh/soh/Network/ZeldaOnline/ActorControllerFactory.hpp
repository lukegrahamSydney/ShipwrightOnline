#ifndef ACTORCONTROLLERFACTORYH
#define ACTORCONTROLLERFACTORYH

#include <unordered_map>
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

namespace ZeldaOnline {
class ActorControllerFactory {
  public:
    using SpawnPredicate = bool (*)(s16 params);
    using CreateFn = AbstractActorController* (*)(Actor*, int, int, int, bool);

    static ActorControllerFactory& Instance() {
        static ActorControllerFactory sInstance;
        return sInstance;
    }

    bool IsNetworked(s16 actorID, s16 params) const {
        auto it = m_entries.find(actorID);
        if (it == m_entries.end()) {
            return false;
        }
        return it->second.predicate == nullptr || it->second.predicate(params);
    }

    AbstractActorController* Create(s16 actorID, Actor* actor, int networkID, int sceneKey, int roomIndex,
                                    bool isLeader) const {
        auto it = m_entries.find(actorID);
        if (it == m_entries.end()) {
            return nullptr;
        }
        return it->second.create(actor, networkID, sceneKey, roomIndex, isLeader);
    }

  private:
    struct FactoryEntry {
        CreateFn create;
        SpawnPredicate predicate;
    };

    template <typename T>
    static AbstractActorController* CreateController(Actor* actor, int networkID, int sceneKey, int roomIndex,
                                                     bool isLeader) {
        AbstractActorController* controller = new T(actor, networkID, sceneKey, roomIndex, isLeader);
        return controller;
    }

    template <typename T> void Register(s16 actorID, SpawnPredicate predicate = nullptr) {
        m_entries[actorID] = { &CreateController<T>, predicate };
    }

    ActorControllerFactory() {
        //Pretty self explanatory. You register the controller here which allows it to be synced online. Without this one line
        //You will only create a local copy and nobody else will see the syncronization.
        //The template param is the name of the controller class, and param1 is the the ActorID. You can also 
        //Make it only work for actors that have a specific param. Look at the ones on the bottom to see how
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
        Register<RutoController>(ACTOR_EN_RU1);
        Register<ValiController>(ACTOR_EN_VALI);
        Register<EiyerController>(ACTOR_EN_EIYER);
        Register<BrobController>(ACTOR_EN_BROB);
        Register<RedeadController>(ACTOR_EN_RD);
        Register<PeahatController>(ACTOR_EN_PEEHAT);
        Register<SmallCrateController>(ACTOR_OBJ_KIBAKO);
        Register<LikeLikeController>(ACTOR_EN_RR);
        Register<DeadHandController>(ACTOR_EN_DH);
        Register<DeadHandArmController>(ACTOR_EN_DHA);

        Register<FlyingSkullController>(ACTOR_EN_BB,
                                        [](s16 params) { return FlyingSkullController::IsNetworkedVariant(params); });
        Register<WallmasterController>(ACTOR_EN_WALLMAS);
        Register<FloormasterController>(ACTOR_EN_FLOORMAS);
        Register<RollingGoronController>(ACTOR_EN_GO2,
                                         [](s16 params) { return RollingGoronController::IsNetworkedVariant(params); });

        Register<BarinadeController>(ACTOR_BOSS_VA,
                                     [](s16 params) { return BarinadeController::IsNetworkedVariant(params); });

        Register<BaController>(ACTOR_EN_BA, [](s16 params) { return BaController::IsNetworkedVariant(params); });

        Register<TailpasaranController>(ACTOR_EN_TP, [](s16 params) { return TailpasaranController::IsNetworkedVariant(params); });
        Register<DekuScrubController>(ACTOR_EN_DEKUNUTS,
                                      [](s16 params) { return DekuScrubController::IsNetworkedVariant(params); });
        Register<NutsballController>(ACTOR_EN_NUTSBALL);

        Register<SkullWalltulaController>(ACTOR_EN_SW, [](s16 params) { return ((params & 0xE000) >> 13) == 0; });

        Register<EnGomaController>(ACTOR_EN_GOMA, [](s16 p) { return EnGomaController::IsNetworkedVariant(p); });
        Register<HintScrubController>(ACTOR_EN_HINTNUTS,
                                      [](s16 p) { return HintScrubController::IsNetworkedVariant(p); });
    }

    std::unordered_map<s16, FactoryEntry> m_entries;
};
}

#endif
