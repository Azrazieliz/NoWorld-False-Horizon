#include "Content/OGSystemBindingComponent.h"
#include "World/OGWorldSimulationReceiver.h"
#include "Runtime/OGAdaptiveScoreComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Persistence/OGWorldStore.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
bool FailBinding(FString& Error,const TCHAR* Message) { Error=Message; return false; }
void DestroyReceiver(UOGDefinitionReceiverComponent* Receiver)
{
    if (!IsValid(Receiver)) return;
    Receiver->ReleaseDefinition();
    if (auto* Owner=Receiver->GetOwner()) Owner->RemoveInstanceComponent(Receiver);
    Receiver->DestroyComponent();
}
}
UOGDefinitionReceiverComponent::UOGDefinitionReceiverComponent()
{ PrimaryComponentTick.bCanEverTick=false; bAutoActivate=false; }
bool UOGDefinitionReceiverComponent::PrepareDefinition_Implementation(
    UDataAsset*,const FOGDefinitionBindingContext&,FString& Error)
{ return FailBinding(Error,TEXT("A subsystem receiver must implement its preparation contract.")); }
UOGSystemBindingDefinition::UOGSystemBindingDefinition() { System=TEXT("binding"); }
bool UOGSystemBindingDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("binding") || Bindings.IsEmpty())
        return FailBinding(Error,TEXT("A binding profile requires real subsystem bindings."));
    TSet<FName> Slots,Channels;
    for (const auto& B:Bindings)
    {
        if (B.Slot.IsNone() || Slots.Contains(B.Slot) || B.Definition.System.IsNone() ||
            !B.Definition.Id.IsValid() || B.Definition.MinimumVersion<1 || !B.ReceiverClass ||
            B.ReceiverClass->HasAnyClassFlags(CLASS_Abstract|CLASS_Deprecated|CLASS_NewerVersionExists) ||
            B.ReceiverClass->GetDefaultObject<UOGDefinitionReceiverComponent>()->AcceptedSystem!=B.Definition.System)
            return FailBinding(Error,TEXT("Binding slots require distinct names, valid definitions and a compatible concrete receiver."));
        Slots.Add(B.Slot);
        for (FName Channel:B.ReceiverClass->GetDefaultObject<UOGDefinitionReceiverComponent>()->ExclusiveChannels)
        {
            if (Channel.IsNone() || Channels.Contains(Channel))
                return FailBinding(Error,TEXT("Binding receivers cannot claim the same exclusive subsystem channel."));
            Channels.Add(Channel);
        }
    }
    return true;
}
bool UOGSystemBindingDefinition::ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const
{
    TSet<FName> ReplacedParts;
    for (const auto& B:Bindings)
    {
        auto* Asset=Resolve(B.Definition.System,B.Definition.Id);
        if (B.ReceiverClass->IsChildOf(UOGStaticMeshDefinitionReceiver::StaticClass()))
        {
            const auto* Mesh=Cast<UOGStaticMeshSystemDefinition>(Asset);
            if (!Mesh) return FailBinding(Error,TEXT("Native mesh receivers require a native mesh definition."));
            for (FName Part:Mesh->ReplaceComponentNames)
            {
                if (ReplacedParts.Contains(Part)) return FailBinding(Error,TEXT("Two mesh receivers cannot replace the same actor part."));
                ReplacedParts.Add(Part);
            }
        }
        if (B.ReceiverClass->IsChildOf(UOGWorldSimulationReceiver::StaticClass()) && !Cast<UOGWorldSimulationDefinition>(Asset))
            return FailBinding(Error,TEXT("Native World simulation receivers require a shared World simulation definition."));
        if (B.ReceiverClass->IsChildOf(UOGScoreDefinitionReceiver::StaticClass()) && !Cast<UOGAdaptiveScoreDefinition>(Asset))
            return FailBinding(Error,TEXT("Native score receivers require an adaptive score definition."));
    }
    return true;
}
void UOGSystemBindingDefinition::CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const
{ Super::CollectRequiredDefinitions(Out); for (const auto& B:Bindings) Out.Add(B.Definition); }
UOGSystemBindingComponent::UOGSystemBindingComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bStartWithTickEnabled=false;
    PrimaryComponentTick.TickInterval=1.f;
}
void UOGSystemBindingComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    if (!bAwaitingCanonicalRuntime || !CanonicalEntityId.IsValid()) { SetComponentTickEnabled(false); return; }
    auto* Game=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    auto* Core=Game?Game->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    auto* Store=Core?Core->GetWorldStore():nullptr;
    if (!Core || !Core->IsCoreReady() || Core->GetCanonicalWorldTick()<0 || !Store || !Store->IsOpen()) return;
    FString Error; if (!RefreshBinding(Error)) Status=Error;
}
void UOGSystemBindingComponent::RestoreWorldBodyFlags()
{
    if (!bHoldingWorldBody || !GetOwner()) return;
    GetOwner()->SetActorHiddenInGame(bWorldBodyHidden);
    GetOwner()->SetActorEnableCollision(bWorldBodyCollision);
    bHoldingWorldBody=false;
}
void UOGSystemBindingComponent::HandleCanonicalRuntimeReleasing()
{
    for (auto* Receiver:ActiveReceivers)
        if (auto* Body=Cast<UOGWorldSimulationReceiver>(Receiver))
        {
            if (Body->GetOriginalBodyFlags(bWorldBodyHidden,bWorldBodyCollision)) bHoldingWorldBody=true;
            Body->HoldSuppressedOnRelease();
        }
    ReleaseBinding();
    if (bHoldingWorldBody && GetOwner())
    { GetOwner()->SetActorHiddenInGame(true); GetOwner()->SetActorEnableCollision(false); }
    bAwaitingCanonicalRuntime=CanonicalEntityId.IsValid();
    SetComponentTickEnabled(bAwaitingCanonicalRuntime);
}
void UOGSystemBindingComponent::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Library=UOGDefinitionLibrary::Get())
        LibraryHandle=Library->OnDefinitionsChanged.AddUObject(this,&ThisClass::HandleLibraryChanged);
    if (CanonicalEntityId.IsValid())
    {
        FString Error;
        if (!RefreshBinding(Error))
        { Status=Error; bAwaitingCanonicalRuntime=true; SetComponentTickEnabled(true); }
    }
}
void UOGSystemBindingComponent::HandleLibraryChanged(int64)
{ if (CanonicalEntityId.IsValid() && !bBinding) { FString Error; if (!RefreshBinding(Error)) Status=Error; } }
bool UOGSystemBindingComponent::RefreshBinding(FString& Error)
{ return BindCanonicalEntity(CanonicalEntityId,Error); }
bool UOGSystemBindingComponent::BindCanonicalEntity(const FOGEntityId& Entity,FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || bBinding || !GetOwner() || !GetOwner()->GetRootComponent() || !Entity.IsValid())
        return FailBinding(Error,TEXT("Bind an existing canonical entity on a rooted actor without reentry."));
    TGuardValue<bool> Binding(bBinding,true);
    auto* Game=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    auto* Core=Game?Game->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    auto* Store=Core?Core->GetWorldStore():nullptr;
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen())
        return FailBinding(Error,TEXT("Canonical World state is unavailable."));
    bool Found=false; FString State; FOGDefinitionBindingContext Context; Context.EntityId=Entity;
    if (!Store->TryReadEntity(Entity,Found,Context.EntityKind,State,Context.EntityRevision,Error)) return false;
    TSharedPtr<FJsonObject> Json;
    if (!Found || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(State),Json) ||
        !Json || Json->HasField(TEXT("diagnosticFixture")))
    {
        if (CanonicalEntityId==Entity) ReleaseBinding();
        return FailBinding(Error,TEXT("Definition bindings require an actual non-diagnostic canonical entity."));
    }
    FString ProfileText;
    if (Json->HasField(TEXT("system_binding_id")) && !Json->TryGetStringField(TEXT("system_binding_id"),ProfileText))
        return FailBinding(Error,TEXT("Canonical system_binding_id must be an exact content ID string."));
    // An existing entity may legitimately have no optional subsystem composition.
    if (ProfileText.IsEmpty())
    {
        ReleaseBinding(); RestoreWorldBodyFlags(); CanonicalEntityId=Entity; Status.Reset(); return true;
    }
    const FOGContentId ProfileId(ProfileText);
    auto* Library=UOGDefinitionLibrary::Get();
    TStrongObjectPtr<UOGSystemBindingDefinition> Profile(Library?Cast<UOGSystemBindingDefinition>(Library->Resolve(TEXT("binding"),ProfileId)):nullptr);
    if (!ProfileId.IsValid() || !Profile.IsValid() || !Profile->ValidateDefinition(Error))
    { if (Error.IsEmpty()) Error=TEXT("Canonical binding profile is not installed."); return false; }
    if (CanonicalEntityId==Entity && BoundProfile==ProfileId && BoundLibraryRevision==Library->GetRevision()) return true;
    TArray<TStrongObjectPtr<UOGDefinitionReceiverComponent>> Staged;
    for (const auto& B:Profile->Bindings)
    {
        auto* Definition=Library->Resolve(B.Definition.System,B.Definition.Id);
        TStrongObjectPtr<UOGDefinitionReceiverComponent> Receiver(NewObject<UOGDefinitionReceiverComponent>(GetOwner(),B.ReceiverClass));
        if (Receiver.IsValid()) { Receiver->BoundDefinition=Definition; Receiver->BoundContext=Context; }
        if (!Receiver.IsValid() || !Definition || !Receiver->PrepareDefinition(Definition,Context,Error))
        {
            if (Receiver.IsValid()) Receiver->ReleaseDefinition();
            for (auto& Pending:Staged) Pending->ReleaseDefinition();
            return false;
        }
        Staged.Add(MoveTemp(Receiver));
    }
    // No active receiver is touched until every replacement can be installed.
    ReleaseBinding();
    UOGWorldSimulationReceiver* NextBody=nullptr;
    for (const auto& Receiver:Staged)
        if (auto* Body=Cast<UOGWorldSimulationReceiver>(Receiver.Get())) NextBody=Body;
    if (bHoldingWorldBody && NextBody)
        NextBody->AdoptOriginalBodyFlags(bWorldBodyHidden,bWorldBodyCollision);
    else RestoreWorldBodyFlags();
    CanonicalEntityId=Entity; BoundProfile=ProfileId; BoundLibraryRevision=Library->GetRevision();
    for (auto& Receiver:Staged)
    {
        auto* Component=Receiver.Get(); ActiveReceivers.Add(Component);
        GetOwner()->AddInstanceComponent(Component); Component->RegisterComponent(); Component->ActivateDefinition();
    }
    if (NextBody && !NextBody->IsBodyActive())
    {
        ReleaseBinding(); bAwaitingCanonicalRuntime=true; SetComponentTickEnabled(true);
        return FailBinding(Error,TEXT("Canonical World body activation did not acquire its physical claim."));
    }
    if (NextBody) bHoldingWorldBody=false; // The receiver retains flags until canonical rehydration succeeds.
    BoundCore=Core;
    Core->OnCanonicalRuntimeReleasing.AddUObject(this,&ThisClass::HandleCanonicalRuntimeReleasing);
    Status.Reset(); return true;
}
void UOGSystemBindingComponent::ReleaseBinding()
{
    bAwaitingCanonicalRuntime=false; SetComponentTickEnabled(false);
    if (BoundCore.IsValid()) BoundCore->OnCanonicalRuntimeReleasing.RemoveAll(this);
    BoundCore.Reset();
    for (int32 I=ActiveReceivers.Num()-1;I>=0;--I) DestroyReceiver(ActiveReceivers[I]);
    ActiveReceivers.Reset(); BoundProfile=FOGContentId(); BoundLibraryRevision=-1;
}
void UOGSystemBindingComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Library=UOGDefinitionLibrary::Get()) Library->OnDefinitionsChanged.Remove(LibraryHandle);
    ReleaseBinding(); RestoreWorldBodyFlags(); Super::EndPlay(Reason);
}
UOGStaticMeshSystemDefinition::UOGStaticMeshSystemDefinition() { System=TEXT("mesh"); }
bool UOGStaticMeshSystemDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("mesh") || Mesh.IsNull() || !Mesh.ToSoftObjectPath().GetAssetPathString().StartsWith(TEXT("/Game/")) ||
        !RelativeTransform.IsValid() || (CollisionEnabled!=ECollisionEnabled::NoCollision && CollisionProfile.IsNone()))
        return FailBinding(Error,TEXT("Mesh definition requires an installed mesh, valid transform and explicit enabled collision profile."));
    TSet<FName> Names;
    for (FName Name:ReplaceComponentNames)
    { if (Name.IsNone() || Names.Contains(Name)) return FailBinding(Error,TEXT("Invalid or repeated replacement component.")); Names.Add(Name); }
    for (const auto& Material:Materials)
        if (Material.IsNull() || !Material.ToSoftObjectPath().GetAssetPathString().StartsWith(TEXT("/Game/")))
            return FailBinding(Error,TEXT("Material overrides must reference actual installed game assets."));
    return true;
}
UOGStaticMeshDefinitionReceiver::UOGStaticMeshDefinitionReceiver() { AcceptedSystem=TEXT("mesh"); }
bool UOGStaticMeshDefinitionReceiver::PrepareDefinition_Implementation(
    UDataAsset* Asset,const FOGDefinitionBindingContext&,FString& Error)
{
    if (InstalledMesh || Prepared) return FailBinding(Error,TEXT("Mesh receiver already prepared."));
    auto* Definition=Cast<UOGStaticMeshSystemDefinition>(Asset);
    if (!Definition || !Definition->ValidateDefinition(Error) || !GetOwner() || !GetOwner()->GetRootComponent()) return false;
    auto* Mesh=Definition->Mesh.LoadSynchronous();
    if (!Mesh || Mesh->GetPathName()!=Definition->Mesh.ToSoftObjectPath().ToString())
        return FailBinding(Error,TEXT("Installed mesh is missing or redirected."));
    TArray<UMaterialInterface*> Materials;
    for (const auto& Ref:Definition->Materials)
    {
        auto* Material=Ref.LoadSynchronous();
        if (!Material || Material->GetPathName()!=Ref.ToSoftObjectPath().ToString())
            return FailBinding(Error,TEXT("Installed mesh material is missing or redirected."));
        Materials.Add(Material);
    }
    for (FName Name:Definition->ReplaceComponentNames)
    {
        TArray<UPrimitiveComponent*> Parts; GetOwner()->GetComponents(Parts);
        if (Parts.FilterByPredicate([Name](const auto* P){return P->GetFName()==Name;}).Num()!=1)
            return FailBinding(Error,TEXT("A declared mesh replacement part does not resolve uniquely."));
    }
    Prepared=Definition;
    InstalledMesh=NewObject<UStaticMeshComponent>(GetOwner());
    InstalledMesh->SetupAttachment(GetOwner()->GetRootComponent());
    InstalledMesh->SetStaticMesh(Mesh); InstalledMesh->SetRelativeTransform(Definition->RelativeTransform);
    for (int32 I=0;I<Materials.Num();++I) InstalledMesh->SetMaterial(I,Materials[I]);
    InstalledMesh->SetVisibility(false,false); InstalledMesh->SetHiddenInGame(true,false);
    InstalledMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // Component remains unregistered and invisible until the complete profile commits.
    return true;
}
void UOGStaticMeshDefinitionReceiver::ActivateDefinition_Implementation()
{
    if (!InstalledMesh || !Prepared || bActive) return;
    TArray<UPrimitiveComponent*> Parts; GetOwner()->GetComponents(Parts);
    for (auto* Part:Parts)
        if (Prepared->ReplaceComponentNames.Contains(Part->GetFName()))
        {
            FSavedPart Saved; Saved.Part=Part; Saved.Visible=Part->IsVisible(); Saved.Hidden=Part->bHiddenInGame;
            Saved.Collision=Part->GetCollisionEnabled(); Previous.Add(Saved);
            Part->SetVisibility(false,false); Part->SetHiddenInGame(true,false); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    GetOwner()->AddInstanceComponent(InstalledMesh); InstalledMesh->RegisterComponent();
    if (!Prepared->CollisionProfile.IsNone()) InstalledMesh->SetCollisionProfileName(Prepared->CollisionProfile);
    InstalledMesh->SetCollisionEnabled(Prepared->CollisionEnabled);
    InstalledMesh->SetVisibility(true,false); InstalledMesh->SetHiddenInGame(false,false); bActive=true;
}
void UOGStaticMeshDefinitionReceiver::ReleaseDefinition_Implementation()
{
    for (const auto& Saved:Previous) if (auto* Part=Saved.Part.Get())
    { Part->SetVisibility(Saved.Visible,false); Part->SetHiddenInGame(Saved.Hidden,false); Part->SetCollisionEnabled(Saved.Collision); }
    Previous.Reset();
    if (InstalledMesh)
    { if (auto* Owner=InstalledMesh->GetOwner()) Owner->RemoveInstanceComponent(InstalledMesh); InstalledMesh->DestroyComponent(); }
    InstalledMesh=nullptr; Prepared=nullptr; bActive=false;
}

UOGScoreDefinitionReceiver::UOGScoreDefinitionReceiver() { AcceptedSystem=TEXT("score"); ExclusiveChannels.Add(TEXT("audio.score")); }
bool UOGScoreDefinitionReceiver::PrepareDefinition_Implementation(
    UDataAsset* Asset,const FOGDefinitionBindingContext&,FString& Error)
{
    auto* Definition=Cast<UOGAdaptiveScoreDefinition>(Asset);
    if (InstalledScore || !Definition || !Definition->Validate(Error) || !GetOwner()) return false;
    if (!GetOwner()->FindComponentByClass<UOGSemanticAudioRuntime>())
        return FailBinding(Error,TEXT("A shared score needs the actor's actual semantic audio runtime."));
    TArray<UOGAdaptiveScoreComponent*> Existing; GetOwner()->GetComponents(Existing);
    for (auto* Component:Existing)
        if (!Component->ComponentTags.Contains(TEXT("OG.Definition.Score")))
            return FailBinding(Error,TEXT("An existing independent score director already owns this actor's music."));
    InstalledScore=NewObject<UOGAdaptiveScoreComponent>(GetOwner());
    InstalledScore->Score=Definition; InstalledScore->ComponentTags.Add(TEXT("OG.Definition.Score"));
    return true;
}
void UOGScoreDefinitionReceiver::ActivateDefinition_Implementation()
{
    if (!InstalledScore || InstalledScore->IsRegistered()) return;
    GetOwner()->AddInstanceComponent(InstalledScore); InstalledScore->RegisterComponent();
}
void UOGScoreDefinitionReceiver::ReleaseDefinition_Implementation()
{
    if (!InstalledScore) return;
    if (auto* Owner=InstalledScore->GetOwner()) Owner->RemoveInstanceComponent(InstalledScore);
    InstalledScore->DestroyComponent(); InstalledScore=nullptr;
}
