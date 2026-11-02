from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
p=r/"Private/Combat/OGTurnBattleLuminousAlterV1.cpp"
s=p.read_text(encoding="utf-8-sig")
a="""    // The source's exact self-sacrificial standing buff never makes Alter
    // a substitute for Base's healer or player Ruler.
    if(bA3)
    {
        Own.MartyrStanceExpires=Current+2;
        Own.bMartyrNextFinisherPending=true;
        return true;
    }

    TSet<FOGEntityId> Eligible;"""
b="""    // A3 is an owner action too: its previously inflicted, registered
    // Severing Wounds must still tick this turn. Returning early here used
    // to skip source P2 and P1, incorrectly extending enemy wound timers.
    TSet<FOGEntityId> Eligible;"""
assert s.count(a)==1;s=s.replace(a,b,1)
a="""    FOGDeterministicRng Rng(static_cast<uint64>(
        HashCombine(GetTypeHash(State.BattleId.ToString()),
                    GetTypeHash(Action.ActionId.ToString()))));"""
b="""    // Only after genuine wound-start damage/siphon has resolved can A3
    // establish its 2-turn Martyr Crown; it causes no free sword attack.
    if(bA3)
    {
        Own.MartyrStanceExpires=Current+2;
        Own.bMartyrNextFinisherPending=true;
        return true;
    }
    FOGDeterministicRng Rng(static_cast<uint64>(
        HashCombine(GetTypeHash(State.BattleId.ToString()),
                    GetTypeHash(Action.ActionId.ToString()))));"""
assert s.count(a)==1;s=s.replace(a,b,1)
p.write_text(s,encoding="utf8",newline="")
print("A3 now ticks pre-existing registered Severing Wounds on owner's action; stance then applies without fabricated damage")
