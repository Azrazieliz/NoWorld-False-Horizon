#if WITH_DEV_AUTOMATION_TESTS
#include "Animation/OGSharedReferenceMotion.h"
#include "Misc/AutomationTest.h"
#include "Misc/Compression.h"
#include "Misc/Crc.h"
#include "Engine/SkeletalMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedReferenceSamplingTest,
    "OfflineGame.Animation.SharedReference.IndependentRigSampling",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGSharedReferenceSamplingTest::RunTest(const FString& Parameters)
{
    FOGReferenceMotionClip Clip; Clip.Rate=2; Clip.Duration=1; Clip.Frames=3;
    FOGReferenceMotionTrack Track; Track.Bone=TEXT("authored_source_joint");
    Track.Rotations={FQuat::Identity,FRotator(0,90,0).Quaternion(),FRotator(0,180,0).Quaternion()};
    Track.Translations={FVector::ZeroVector,FVector(2,0,0),FVector(4,0,0)};
    Track.Scales={FVector::OneVector,FVector(2),FVector(3)};
    Clip.Tracks.Add(Track);
    const auto Sample=Clip.Sample(0,.25);
    TestTrue(TEXT("Authored rotation interpolates by source phase"),
        Sample.GetRotation().Equals(FRotator(0,45,0).Quaternion(),.0001));
    TestTrue(TEXT("Translation is retained, not discarded"),Sample.GetTranslation().Equals(FVector(1,0,0)));
    TestTrue(TEXT("Scale is retained, not discarded"),Sample.GetScale3D().Equals(FVector(1.5)));
    TestTrue(TEXT("Source end clamps to original final sample"),Clip.Sample(0,100).GetTranslation().Equals(FVector(4,0,0)));
    Clip.Tracks[0].Translations.Reset(); Clip.Tracks[0].Scales.Reset();
    const auto RotationOnly=Clip.Sample(0,.25);
    TestTrue(TEXT("Missing source translation preserves native reference offset"),RotationOnly.GetTranslation().IsZero());
    TestTrue(TEXT("Missing source scale preserves native morphology"),RotationOnly.GetScale3D().Equals(FVector::OneVector));
    Clip.Duration=.9f;
    TestTrue(TEXT("Short original final interval reaches the actual final sample"),
        Clip.Sample(0,.9).GetRotation().Equals(Track.Rotations.Last(),.0001));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedReferenceCodecTest,
    "OfflineGame.Animation.SharedReference.IntegrityAndSourceIdentity",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGSharedReferenceCodecTest::RunTest(const FString& Parameters)
{
    const FString Json=TEXT("{\"schema\":\"noworld.shared_reference_motion.v1\",\"id\":\"test.source\",\"rate\":30,\"duration\":0,\"frames\":1,\"tracks\":[{\"bone\":\"source_root\",\"aliases\":[\"native_root\"],\"rotations\":[[0,0,0,1]]}]}");
    FTCHARToUTF8 Utf8(*Json);
    const int32 Size=Utf8.Length();
    int64 Compressed=FCompression::CompressMemoryBound(NAME_Zlib,Size);
    TArray<uint8> Data; Data.SetNumUninitialized(16+static_cast<int32>(Compressed));
    const uint8 Magic[]={'N','W','M','R','J','0','0','1'};
    FMemory::Memcpy(Data.GetData(),Magic,8);
    auto Write=[&Data](int32 At,uint32 Value) { for (int32 I=0;I<4;++I) Data[At+I]=uint8(Value>>(8*I)); };
    Write(8,Size); Write(12,FCrc::MemCrc32(Utf8.Get(),Size));
    if (!TestTrue(TEXT("Compress native runtime fixture"),FCompression::CompressMemory(
        NAME_Zlib,Data.GetData()+16,Compressed,Utf8.Get(),Size))) return false;
    Data.SetNum(16+static_cast<int32>(Compressed));
    FString Error; auto Clip=FOGSharedReferenceLibrary::Decode(Data,Error);
    if (!TestTrue(TEXT("Native reader accepts shared source format"),Clip.IsValid())) return false;
    TestEqual(TEXT("Source identity does not depend on a character"),Clip->Id,FName(TEXT("test.source")));
    TestEqual(TEXT("Exact native alias survives import"),Clip->Tracks[0].Aliases[0],FName(TEXT("native_root")));
    TestTrue(TEXT("Static source pose is valid"),Clip->Sample(0,100).Equals(FTransform::Identity));
    Data[12]^=1;
    TestFalse(TEXT("Corrupt payload cannot drive any character"),FOGSharedReferenceLibrary::Decode(Data,Error).IsValid());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedReferenceRigTest,
    "OfflineGame.Animation.SharedReference.CompleteUnambiguousRigAdapter",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGSharedReferenceRigTest::RunTest(const FString& Parameters)
{
    USkeletalMesh* Mesh=NewObject<USkeletalMesh>();
    FOGReferenceRigAdapter Rig; Rig.Mesh=Mesh;
    Rig.Bones.Add(TEXT("source_joint"),0); Rig.Bones.Add(TEXT("native_joint"),1);
    Rig.ReferencePose={FTransform::Identity,
        FTransform(FRotator(0,30,0).Quaternion(),FVector(0,10,0),FVector(2))};
    FOGReferenceMotionClip Clip; Clip.Rate=30; Clip.Duration=0; Clip.Frames=1;
    FOGReferenceMotionTrack Track; Track.Bone=TEXT("source_joint");
    Track.Aliases.Add(TEXT("native_joint"));
    Track.Rotations.Add(FRotator(0,60,0).Quaternion()); Clip.Tracks.Add(Track);
    TArray<int32> Mapping; FString Error; TMap<FName,FName> Aliases;
    TestFalse(TEXT("Two matching real bones never select by alias array order"),
        Rig.Map(Clip,Aliases,Mapping,Error));
    Aliases.Add(TEXT("source_joint"),TEXT("native_joint"));
    if (!TestTrue(TEXT("One rig-level alias resolves the original source"),
        Rig.Map(Clip,Aliases,Mapping,Error))) return false;
    TArray<FTransform> Pose;
    if (!TestTrue(TEXT("Shared source evaluates on connected native rig"),
        Rig.Evaluate(Clip,Mapping,0,Pose))) return false;
    TestTrue(TEXT("Native reference orientation composes with original delta"),
        Pose[1].GetRotation().Equals(FRotator(0,90,0).Quaternion(),.0001));
    TestTrue(TEXT("Missing translation preserves actual native morphology"),
        Pose[1].GetTranslation().Equals(FVector(0,10,0)));
    TestTrue(TEXT("Missing scale preserves actual native reference scale"),
        Pose[1].GetScale3D().Equals(FVector(2)));
    TestTrue(TEXT("Unanimated native bone retains its reference pose"),
        Pose[0].Equals(Rig.ReferencePose[0]));
    Clip.Tracks[0].Bone=TEXT("missing_source");
    Clip.Tracks[0].Aliases.Reset();
    TestFalse(TEXT("Difficult missing tracks are reported, never silently dropped"),
        Rig.Map(Clip,Aliases,Mapping,Error));
    return true;
}

#endif
