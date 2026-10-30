#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Interaction/OGSystemicInteractionRuntime.h"
#include "OGInteractionDefinition.generated.h"

/** An exact original animation role, with optional rig/copy/Version overrides. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGInteractionNativeTrack
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Role;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGContentId RigFamilyId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGEntityId ManifestationId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGContentId VersionId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Action;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Context;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SourceSceneRole;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FOGSharedMorphTrack> ExpressionTracks;
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGInteractionNativePlacement
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Role;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTransform RelativeToAnchor;
};

/** Sparse state changes, applied to a fresh canonical record on completion. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGInteractionNativeStateChange
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Role;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bChangeSkin = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGContentId SkinId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true)) FString OutfitMergeJson = TEXT("{}");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true)) FString VariantMergeJson = TEXT("{}");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bChangeContextProfile = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGContentId ContextProfileId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true)) FString ContextMergeJson = TEXT("{}");
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGInteractionNativeNode
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName NodeId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGContentId ActionId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.001")) double DurationSeconds = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> RequiredRoles;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> NextNodes;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SceneAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SceneContext;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FOGInteractionNativeTrack> Tracks;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FOGInteractionNativePlacement> Placements;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FOGInteractionNativeStateChange> StateChanges;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName EnvironmentAnchorTag;
};

/** Reusable native content. No NPC whitelist, universal reward, or permission tree.
 * Roles describe original source tracks; extra participants remain logical observers. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGInteractionDefinition : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGContentId GraphId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true)) FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SourceRevision;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bMaturePresentation = false;
    /** Copies the original sexual/adult source classification, not general violence. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bContainsAdultSource = false;
    /** One player role and any number of source roles. No global arity ceiling. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Roles;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName StartNode;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FOGInteractionNativeNode> Nodes;

    bool BuildGraph(FOGInteractionActionGraph& Out, FString& Error) const;
    bool BindRoles(const TArray<FOGInteractionParticipant>& Selection,
        TArray<FOGInteractionParticipant>& Out, FString& Error) const;
};

UCLASS(BlueprintType)
class OFFLINEGAME_API UOGInteractionCatalog : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TSoftObjectPtr<UOGInteractionDefinition>> Definitions;
};
