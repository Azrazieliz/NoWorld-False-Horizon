#include "Runtime/OGGameCoreSubsystem.h"

namespace
{
bool ReservedRecoveryProvider(FName Id) { return Id==TEXT("core.native") || Id==TEXT("core.luminous"); }
}
bool UOGGameCoreSubsystem::RegisterCombatRecoveryProvider(FOGCombatRecoveryProvider Provider,FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || bSubmittingNativeAction || Provider.Id.IsNone() || ReservedRecoveryProvider(Provider.Id) ||
        Provider.Version<1 || Provider.MinimumReadableVersion<1 || Provider.MinimumReadableVersion>Provider.Version ||
        !Provider.Capture || !Provider.Restore || CombatRecoveryProviders.Contains(Provider.Id))
    { Error=TEXT("Native recovery registration requires a unique installed factory and explicit version range."); return false; }
    CombatRecoveryProviders.Add(Provider.Id,MoveTemp(Provider));
    if (PendingNativeRecoveryRuler.IsValid() && bCoreReady && !OwnedLuminousTurnBattle)
    {
        bool Restored=false; FString RecoveryError;
        if (!RestoreOwnedNativeEncounter(PendingNativeRecoveryRuler,Restored,RecoveryError))
            NativeEncounterRecoveryStatus=RecoveryError;
    }
    return true;
}
bool UOGGameCoreSubsystem::UnregisterCombatRecoveryProvider(FName Provider,FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || bSubmittingNativeAction || (OwnedLuminousTurnBattle && ActiveNativeRecoveryProvider==Provider))
    { Error=TEXT("An active encounter still owns this native recovery provider."); return false; }
    if (!CombatRecoveryProviders.Remove(Provider)) { Error=TEXT("Native recovery provider is not registered."); return false; }
    return true;
}
bool UOGGameCoreSubsystem::CaptureCombatRecoveryProvider(FName Provider,const FOGTurnBattle& Battle,
    const FOGEntityId& Ruler,int32& Version,FString& Payload,FString& Error) const
{
    Payload.Reset(); Version=1;
    if (ReservedRecoveryProvider(Provider)) return true;
    const auto* Registered=CombatRecoveryProviders.Find(Provider);
    if (!Registered || !WorldStore || !WorldStore->IsOpen())
    { Error=TEXT("The encounter's installed native recovery provider is unavailable."); return false; }
    const auto Factory=*Registered; Version=Factory.Version;
    if (!Factory.Capture(Battle,*WorldStore,Ruler,Payload,Error)) return false;
    if (Payload.Len()>1024*1024) { Error=TEXT("Native recovery provider payload exceeds its data budget."); return false; }
    return true;
}
bool UOGGameCoreSubsystem::RestoreCombatRecoveryProvider(FName Provider,int32 Version,const FString& Payload,
    const FOGEntityId& Ruler,FOGCombatRecoveryBindings& Bindings,FString& Error)
{
    if (!WorldStore || !WorldStore->IsOpen() || !Ruler.IsValid())
    { Error=TEXT("Native callback recovery requires the reopened canonical World."); return false; }
    if (ReservedRecoveryProvider(Provider))
    {
        if (Version!=1 || !Payload.IsEmpty())
        { Error=TEXT("Built-in native callback recovery version/data is inconsistent."); return false; }
        if (Provider==TEXT("core.luminous"))
            Bindings.RebindSpecialized=[this,Ruler](FOGTurnBattle& Battle,FString& Failure)
            { return BindOwnedLuminousCombatAuthority(Battle,Ruler,FOGLuminousCanonicalSources(),Failure); };
        return true;
    }
    const auto* Registered=CombatRecoveryProviders.Find(Provider);
    if (!Registered || Version<Registered->MinimumReadableVersion || Version>Registered->Version || Payload.Len()>1024*1024)
    { Error=TEXT("The saved encounter needs an installed recovery factory supporting its recorded version."); return false; }
    const auto Factory=*Registered;
    return Factory.Restore(*WorldStore,Ruler,Version,Payload,Bindings,Error);
}
