#pragma once
#include "CoreMinimal.h"
#include "Content/OGDefinitionLibrary.h"
#include "Core/OGEntityId.h"
#include "Persistence/OGWorldStore.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "OGSystemBindingComponent.generated.h"

class UOGGameCoreSubsystem;
class UOGAdaptiveScoreComponent;
class UOGAdaptiveScoreDefinition;
class UStaticMesh;
class UMaterialInterface;
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGDefinitionBindingContext
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FOGEntityId EntityId;
    UPROPERTY(BlueprintReadOnly) FName EntityKind;
    UPROPERTY(BlueprintReadOnly) int64 EntityRevision=0;
};
/** One receiver implementation per subsystem; definitions can be shared by any number of real entities. */
UCLASS(Abstract,Blueprintable,ClassGroup=(OfflineGame))
class OFFLINEGAME_API UOGDefinitionReceiverComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOGDefinitionReceiverComponent();
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) FName AcceptedSystem;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TArray<FName> ExclusiveChannels;
    UPROPERTY(Transient,BlueprintReadOnly) TObjectPtr<UDataAsset> BoundDefinition;
    UPROPERTY(Transient,BlueprintReadOnly) FOGDefinitionBindingContext BoundContext;
    UFUNCTION(BlueprintNativeEvent,Category="Definitions")
    bool PrepareDefinition(UDataAsset* Definition,const FOGDefinitionBindingContext& Context,FString& Error);
    virtual bool PrepareDefinition_Implementation(UDataAsset*,const FOGDefinitionBindingContext&,FString& Error);
    /** Called only after every receiver has prepared. No failure or canonical writes in this phase. */
    UFUNCTION(BlueprintNativeEvent,Category="Definitions") void ActivateDefinition();
    virtual void ActivateDefinition_Implementation() {}
    UFUNCTION(BlueprintNativeEvent,Category="Definitions") void ReleaseDefinition();
    virtual void ReleaseDefinition_Implementation() {}
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGSystemBindingEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Slot;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGDefinitionReference Definition;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TSubclassOf<UOGDefinitionReceiverComponent> ReceiverClass;
};
/** Reusable composition: new objects choose a profile; they do not get a new subsystem implementation. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGSystemBindingDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGSystemBindingDefinition();
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGSystemBindingEntry> Bindings;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
    virtual void CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const override;
    virtual bool ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const override;
};

UCLASS(ClassGroup=(OfflineGame),meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGSystemBindingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOGSystemBindingComponent();
    /** Optional placed-actor identity. Bind only after the real canonical entity exists. */
    UPROPERTY(EditInstanceOnly,BlueprintReadOnly) FOGEntityId CanonicalEntityId;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) FString Status;
    UFUNCTION(BlueprintCallable,Category="Definitions")
    bool BindCanonicalEntity(const FOGEntityId& EntityId,FString& Error);
    UFUNCTION(BlueprintCallable,Category="Definitions") bool RefreshBinding(FString& Error);
    UFUNCTION(BlueprintCallable,Category="Definitions") void ReleaseBinding();
protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float, ELevelTick, FActorComponentTickFunction*) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(Transient) TArray<TObjectPtr<UOGDefinitionReceiverComponent>> ActiveReceivers;
    TWeakObjectPtr<UOGGameCoreSubsystem> BoundCore;
    friend struct FOGSystemBindingEventTestAccess;
    FDelegateHandle LibraryHandle,EntityFeedHandle;
    TSharedPtr<FOGWorldStoreCommittedEvents> CanonicalEntityFeed;
    FOGEntityId ObservedEntity;
    FName BoundEntityKind;
    bool bAutoRefreshEnabled=false;
    bool bRefreshRequested=false;
    uint64 BindingGeneration=0;
    FOGContentId BoundProfile;
    int64 BoundLibraryRevision=-1;
    bool bBinding=false;
    bool bAwaitingCanonicalRuntime=false;
    bool bHoldingWorldBody=false;
    bool bWorldBodyHidden=false, bWorldBodyCollision=false;
    void QueueRefresh();
    void ObserveCanonicalEntity(UOGGameCoreSubsystem& Core,const FOGEntityId& Entity);
    void StopCanonicalObservation();
    void HandleEntityCommitted(const TArray<FOGWorldEvent>& Events);
    void ReleaseActiveReceivers();
    void RetainWorldBodyFlags();
    void RestoreWorldBodyFlags();
    void HandleCanonicalRuntimeReleasing();
    void HandleLibraryChanged(int64 Revision);
};

UCLASS(BlueprintType)
class OFFLINEGAME_API UOGStaticMeshSystemDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGStaticMeshSystemDefinition();
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TSoftObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<TSoftObjectPtr<UMaterialInterface>> Materials;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FTransform RelativeTransform;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName CollisionProfile;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TEnumAsByte<ECollisionEnabled::Type> CollisionEnabled=ECollisionEnabled::NoCollision;
    /** Explicit replacement parts only; unrelated actor components remain untouched. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FName> ReplaceComponentNames;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
};
UCLASS(ClassGroup=(OfflineGame))
class OFFLINEGAME_API UOGStaticMeshDefinitionReceiver : public UOGDefinitionReceiverComponent
{
    GENERATED_BODY()
public:
    UOGStaticMeshDefinitionReceiver();
    virtual bool PrepareDefinition_Implementation(UDataAsset*,const FOGDefinitionBindingContext&,FString&) override;
    virtual void ActivateDefinition_Implementation() override;
    virtual void ReleaseDefinition_Implementation() override;
private:
    struct FSavedPart { TWeakObjectPtr<UPrimitiveComponent> Part; bool Visible=false,Hidden=false; ECollisionEnabled::Type Collision=ECollisionEnabled::NoCollision; };
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> InstalledMesh;
    UPROPERTY(Transient) TObjectPtr<UOGStaticMeshSystemDefinition> Prepared;
    TArray<FSavedPart> Previous;
    bool bActive=false;
};

/** Native score director receiver: one shared score, no duplicate audio assets. */
UCLASS(ClassGroup=(OfflineGame))
class OFFLINEGAME_API UOGScoreDefinitionReceiver : public UOGDefinitionReceiverComponent
{
    GENERATED_BODY()
public:
    UOGScoreDefinitionReceiver();
    virtual bool PrepareDefinition_Implementation(UDataAsset*,const FOGDefinitionBindingContext&,FString&) override;
    virtual void ActivateDefinition_Implementation() override;
    virtual void ReleaseDefinition_Implementation() override;
private:
    UPROPERTY(Transient) TObjectPtr<UOGAdaptiveScoreComponent> InstalledScore;
};
