from pathlib import Path
R=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
H=R/"Source/OfflineGame/Public/Koikatsu/OGLuminousV3PlayableCharacter.h"
C=R/"Source/OfflineGame/Private/Koikatsu/OGLuminousV3PlayableCharacter.cpp"
h=H.read_text(encoding="utf-8-sig")
c=C.read_text(encoding="utf-8-sig")
def replace(s,a,b):
 n=s.count(a)
 if n!=1: raise ValueError(f"expected 1 match got {n}: {a[:80]}")
 return s.replace(a,b,1)
h=replace(h,'#include "World/OGStartingRegionPresentation.h"','#include "World/OGStartingRegionPresentation.h"\n#include "UI/OGGate12OwnedWorldCharacterBinding.h"')
h=replace(h,'class OFFLINEGAME_API AOGLuminousV3PlayableCharacter final : public AOGWorldPrototypeCharacter','class OFFLINEGAME_API AOGLuminousV3PlayableCharacter final : public AOGWorldPrototypeCharacter, public IOGGate12OwnedWorldCharacterBinding')
h=replace(h,'''    AOGLuminousV3PlayableCharacter();

    UPROPERTY(EditDefaultsOnly''','''    AOGLuminousV3PlayableCharacter();

    // Actual acquired Manifestation, verified in the canonical SQLite store;
    // never a QA owner, auto-granted character or interchangeable Version.
    virtual bool BindOwnedWorldCharacter_Implementation(
        const FOGEntityId& RulerId, const FOGContentId& IdentityId,
        const FOGContentId& VersionId, const FOGEntityId& ManifestationId,
        bool bSfwPresentation) override;

    UPROPERTY(EditDefaultsOnly''')
h=replace(h,'''    int32 ActivePartCount = 0;
    bool bLastAppliedPrivacy''','''    int32 ActivePartCount = 0;
    FOGEntityId BoundOwnedRuler;
    FOGEntityId BoundOwnedManifestation;
    FOGContentId BoundOwnedIdentity;
    FOGContentId BoundOwnedVersion;
    bool bNativeOwnedCharacterBound = false;
    bool bOwnedPresentationSfw = true;
    bool bLastAppliedPrivacy''')
c=replace(c,'''    const bool bPrivacy = GetSfwPresentation();
    const bool bQAAuthorized = IsIsolatedWardrobeQAContext();''','''    const bool bPrivacy = bNativeOwnedCharacterBound
        ? bOwnedPresentationSfw : GetSfwPresentation();
    const bool bQAAuthorized = IsIsolatedWardrobeQAContext();''')
c=replace(c,'''    if (CanonicalLuminousIdentityId.IsEmpty() || CanonicalLuminousVersionId.IsEmpty())
    {
        Error = TEXT("Gate11 Luminous canonical IdentityId/VersionId not yet assigned; refusing persistent wardrobe modification");
        return false;
    }
    const UGameInstance* GI =''','''    if (CanonicalLuminousIdentityId.IsEmpty() || CanonicalLuminousVersionId.IsEmpty())
    {
        Error = TEXT("No approved Luminous canonical Identity/Version IDs on this exact World actor.");
        return false;
    }
    if (bNativeOwnedCharacterBound)
    {
        if (BoundOwnedIdentity.ToString() != CanonicalLuminousIdentityId ||
            BoundOwnedVersion.ToString() != CanonicalLuminousVersionId)
        {
            Error = TEXT("Owned Luminous binding no longer matches this authored World pawn.");
            return false;
        }
        OutContext.EntityId = BoundOwnedManifestation;
        OutContext.ManifestationId = BoundOwnedManifestation;
        OutContext.OwnerEntityId = BoundOwnedRuler;
        return true;
    }
    const UGameInstance* GI =''')
hook='''void AOGLuminousV3PlayableCharacter::BeginPlay()
{'''
method='''bool AOGLuminousV3PlayableCharacter::BindOwnedWorldCharacter_Implementation(
    const FOGEntityId& RulerId, const FOGContentId& IdentityId,
    const FOGContentId& VersionId, const FOGEntityId& ManifestationId,
    bool bSfwPresentation)
{
    LastLuminousError.Reset();
    if (bNativeOwnedCharacterBound || !bLuminousReady ||
        !RulerId.IsValid() || !IdentityId.IsValid() || !VersionId.IsValid() ||
        !ManifestationId.IsValid() ||
        CanonicalLuminousIdentityId != IdentityId.ToString() ||
        CanonicalLuminousVersionId != VersionId.ToString())
    {
        LastLuminousError=TEXT("Exact accepted Luminous Identity/Version or native asset binding unavailable.");
        return false;
    }
    UGameInstance* GI=GetGameInstance();
    UOGGameCoreSubsystem* Core=GI ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    IOGWorldStore* Store=Core ? Core->GetWorldStore() : nullptr;
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen())
    {
        LastLuminousError=TEXT("Canonical ownership database unavailable.");
        return false;
    }
    FOGCharacterManifestationRecord Owned;
    bool bFound=false;
    if (!Store->TryReadCharacterManifestation(
            ManifestationId,bFound,Owned,LastLuminousError) ||
        !bFound || Owned.OwningRulerId!=RulerId ||
        Owned.IdentityId!=IdentityId || Owned.ActiveVersionId!=VersionId ||
        Owned.LifecycleState!=FName(TEXT("active")))
    {
        if (LastLuminousError.IsEmpty())
            LastLuminousError=TEXT("Luminous is not an active Manifestation of this Ruler.");
        return false;
    }
    BoundOwnedRuler=RulerId;
    BoundOwnedIdentity=IdentityId;
    BoundOwnedVersion=VersionId;
    BoundOwnedManifestation=ManifestationId;
    bNativeOwnedCharacterBound=true;
    bOwnedPresentationSfw=bSfwPresentation;
    CurrentWardrobe=EOGLuminousV3Wardrobe::Clothed;
    // Missing presentation state on a newly acquired Manifestation naturally
    // uses its source-authored CLOTHED default. Existing owned attire is read
    // from persistent state; no new state is written during possession.
    RestoreCanonicalWardrobe();
    if (!RefreshPrivacyRenderedAppearance())
    {
        bNativeOwnedCharacterBound=false;
        BoundOwnedRuler=FOGEntityId();
        BoundOwnedManifestation=FOGEntityId();
        BoundOwnedIdentity=FOGContentId();
        BoundOwnedVersion=FOGContentId();
        return false;
    }
    return true;
}

'''
if c.count(hook)!=1:raise RuntimeError("no BeginPlay match")
c=c.replace(hook,method+hook,1)
c=replace(c,'''    if (GetSfwPresentation() != bLastAppliedPrivacy ||
        IsIsolatedWardrobeQAContext() != bLastAppliedExplicitContext)''','''    const bool bEffectiveSfw = bNativeOwnedCharacterBound
        ? bOwnedPresentationSfw : GetSfwPresentation();
    if (bEffectiveSfw != bLastAppliedPrivacy ||
        IsIsolatedWardrobeQAContext() != bLastAppliedExplicitContext)''')
H.write_text(h,encoding="utf-8",newline="")
C.write_text(c,encoding="utf-8",newline="")
print("LUMINOUS_WORLD_ACTOR_OWNERSHIP_IMPLEMENTED_SOURCE_ONLY_NO_BUILD")
