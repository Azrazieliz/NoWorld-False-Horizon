#pragma once

#include "CoreMinimal.h"
#include "Animation/OGSharedMotionLibrary.h"

class USkeletalMeshComponent;
class USkeletalMesh;

/** A source track shared by every connected character; no target Skeleton or wardrobe. */
struct OFFLINEGAME_API FOGReferenceMotionTrack
{
    FName Bone;
    FName CalibrationBone; // Original decoder's measured native target identity.
    TArray<FName> Aliases;
    TArray<FQuat> Rotations;
    TArray<FVector> Translations;
    TArray<FVector> Scales;
};
struct OFFLINEGAME_API FOGReferenceMotionClip
{
    FName Id;
    float Rate = 0;
    float Duration = 0;
    int32 Frames = 0;
    TArray<FOGReferenceMotionTrack> Tracks;
    int64 ResidentBytes = 0;
    /** Sample original reference-local deltas. Missing channels preserve the target reference pose. */
    FTransform Sample(int32 Track, double Seconds) const;
};

/** Shared canonical component-space reference orientations from the actual calibration Skeleton. */
struct OFFLINEGAME_API FOGReferenceCalibrationBasis
{
    TMap<FName, int32> Bones;
    TArray<int32> Parents;
    TArray<FQuat> ComponentRotations;
};
struct OFFLINEGAME_API FOGReferenceBoneConnection
{
    int32 Bone = INDEX_NONE;
    FQuat RotationBasis = FQuat::Identity;
    FQuat TranslationBasis = FQuat::Identity;
    bool bScaleAxesMatch = true;
};

/** One adapter per actual character part/rig. Clips never contain character indices. */
struct OFFLINEGAME_API FOGReferenceRigAdapter
{
    TWeakObjectPtr<USkeletalMesh> Mesh;
    TMap<FName, int32> Bones;
    TArray<FTransform> ReferencePose;
    TArray<int32> Parents;
    TArray<FQuat> ComponentRotations;
    mutable TSharedPtr<const FOGReferenceCalibrationBasis, ESPMode::ThreadSafe> Calibration;
    bool Bind(USkeletalMeshComponent* Part, FString& Error);
    bool Map(const FOGReferenceMotionClip& Clip, const TMap<FName, FName>& Aliases,
        TArray<FOGReferenceBoneConnection>& OutConnections, FString& Error) const;
    bool Evaluate(const FOGReferenceMotionClip& Clip, const TArray<FOGReferenceBoneConnection>& Connections,
        double Seconds, TArray<FTransform>& OutPose) const;
};

/** Offline shared source registry, lazy bounded cache and semantic routing.
 * Source files are loaded once globally, independently of actor/Version/outfit. */
class OFFLINEGAME_API FOGSharedReferenceLibrary
{
public:
    static bool ValidateManifestJson(const FString& Json,FString& Error,bool RequireInstalledPackets=false);
    /** Installer hook after original source packets or their mounted catalog become available. */
    static void RefreshInstalledSources();
    static TSharedPtr<const FOGReferenceMotionClip, ESPMode::ThreadSafe> Load(FName Id, FString& Error);
    static TSharedPtr<const FOGReferenceMotionClip, ESPMode::ThreadSafe> Decode(
        const TArray<uint8>& Bytes, FString& Error);
    static TArray<FOGSharedMotionEntry> Find(FName Action, FName Context, FName Rig,
        FName Anatomy, const TArray<FName>& Capabilities, int32 Count, FName Role);
    static TArray<FName> GetClipIds();
    static FName CanonicalId(FName ExistingSourceId);
    static void TrimCache(int64 BudgetBytes = 128ll * 1024 * 1024);
};
