#pragma once
#include "CoreMinimal.h"
#include "Content/OGContentId.h"
#include "Engine/DataAsset.h"
#include "Subsystems/EngineSubsystem.h"
#include "OGDefinitionLibrary.generated.h"

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGDefinitionReference
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName System;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FOGContentId Id;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MinimumVersion=1;
};
using FOGDefinitionResolver=TFunction<UDataAsset*(FName,const FOGContentId&)>;
/** Reusable data for any subsystem. Mutable entity/ownership state never lives here. */
UCLASS(Abstract,BlueprintType,Blueprintable)
class OFFLINEGAME_API UOGReusableDefinition : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName System;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId DefinitionId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGDefinitionReference> Dependencies;
    UFUNCTION(BlueprintNativeEvent,Category="Definitions")
    bool ValidateDefinition(FString& Error) const;
    virtual bool ValidateDefinition_Implementation(FString& Error) const;
    virtual void CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const { Out.Append(Dependencies); }
    virtual bool ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const { return true; }
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGDefinitionAssetEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName System;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId Id;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Version=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TSoftObjectPtr<UDataAsset> Asset;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGDefinitionReference> Dependencies;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGDefinitionBundleDependency
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId BundleId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MinimumVersion=1;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGDefinitionCatalogFragment
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName System;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(MultiLine=true)) FString Json;
};
/** Install once; all existing consumers resolve shared immutable definitions by system/ID.
 * References remain cook-visible. Registration never creates owned instances. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGDefinitionBundle : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId BundleId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Version=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGDefinitionBundleDependency> Dependencies;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGDefinitionAssetEntry> Assets;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGDefinitionCatalogFragment> Catalogs;
};

using FOGDefinitionCatalogValidator=TFunction<bool(const FString&,FString&)>;
DECLARE_MULTICAST_DELEGATE_OneParam(FOGDefinitionLibraryChanged,int64);

/** One definition library across Worlds, actors and UI. Canonical instances stay in WorldStore. */
UCLASS()
class OFFLINEGAME_API UOGDefinitionLibrary : public UEngineSubsystem
{
    GENERATED_BODY()
public:
    static UOGDefinitionLibrary* Get();
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    /** Discovers cooked bundles under /Game/NoWorld/Definitions once, without loading per entity. */
    void EnsureDiscovered();
    /** Call after an installed package has registered its cooked asset registry. */
    UFUNCTION(BlueprintCallable,Category="Definitions") void RefreshDiscovery();
    UFUNCTION(BlueprintCallable,Category="Definitions") bool InstallBundle(UOGDefinitionBundle* Bundle,FString& Error);
    UFUNCTION(BlueprintCallable,Category="Definitions") bool RemoveBundle(const FOGContentId& BundleId,FString& Error);
    UFUNCTION(BlueprintCallable,Category="Definitions") UDataAsset* Resolve(FName System,const FOGContentId& Id);
    void List(FName System,UClass* RequiredClass,TArray<UDataAsset*>& Out);
    int64 GetRevision() const { return Revision; }
    FOGDefinitionLibraryChanged OnDefinitionsChanged;
    /** New subsystem schemas register one adapter; new entries require no code changes. */
    bool RegisterCatalogSystem(FName System,const FString& RelativeBasePath,const FString& EmptyJson,
        const TArray<FString>& ArrayFields,FOGDefinitionCatalogValidator Validator,FString& Error,
        const TArray<FString>& MetadataFields={});
    static bool LoadCatalog(FName System,const FString& BaseJson,FString& Out,FString& Error);
    static bool MergeCatalogDocuments(const FString& Base,const TArray<FString>& Fragments,
        const TArray<FString>& ArrayFields,FString& Out,FString& Error,const TArray<FString>& MetadataFields={});
private:
    struct FCatalogAdapter
    {
        FString BasePath,EmptyJson;
        TArray<FString> Arrays,Metadata;
        FOGDefinitionCatalogValidator Validate;
    };
    struct FInstalledBundle
    {
        FOGContentId BundleId;
        int32 Version=0;
        UOGDefinitionBundle* Source=nullptr;
        TArray<FOGDefinitionBundleDependency> Dependencies;
        TArray<FOGDefinitionAssetEntry> Assets;
        TArray<FOGDefinitionCatalogFragment> Catalogs;
    };
    TArray<FInstalledBundle> Installed;
    struct FResolvedEntry { FOGDefinitionAssetEntry Entry; UDataAsset* Asset=nullptr; };
    UPROPERTY(Transient) TArray<TObjectPtr<UOGDefinitionBundle>> ActiveBundles;
    UPROPERTY(Transient) TArray<TObjectPtr<UDataAsset>> RetainedAssets;
    TMap<FString,FResolvedEntry> Entries;
    TMap<FName,FCatalogAdapter> CatalogAdapters;
    TMap<FName,TArray<FString>> CatalogFragments;
    bool bDiscovered=false;
    bool bDiscovering=false;
    bool bPublishing=false;
    int64 Revision=0;
    bool BuildCandidate(const TArray<FInstalledBundle>& Bundles,
        TMap<FString,FResolvedEntry>& OutEntries,TMap<FName,TArray<FString>>& OutCatalogs,
        TArray<UDataAsset*>& OutAssets,FString& Error) const;
    bool Publish(const TArray<FInstalledBundle>& Bundles,FString& Error);
};
