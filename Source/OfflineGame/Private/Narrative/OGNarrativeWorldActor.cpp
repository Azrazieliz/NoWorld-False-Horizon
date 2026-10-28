#include "Narrative/OGNarrativeWorldActor.h"
#include "Content/OGSystemBindingComponent.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Narrative/OGNarrativeSessionSubsystem.h"

AOGNarrativeWorldActor::AOGNarrativeWorldActor()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AuthoredNarrativeRoot"));
    SystemBindings=CreateDefaultSubobject<UOGSystemBindingComponent>(TEXT("SharedSystemBindings"));
}

bool AOGNarrativeWorldActor::CanInteract_Implementation(AActor* Interactor) const
{
    const UGameInstance* Game = GetGameInstance();
    const UOGNarrativeSessionSubsystem* Session = Game ? Game->GetSubsystem<UOGNarrativeSessionSubsystem>() : nullptr;
    FString Error;
    return Session && Session->CanOpenWorldSubject(*this, Interactor, Error);
}

FText AOGNarrativeWorldActor::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
    return InteractionPrompt;
}

void AOGNarrativeWorldActor::Interact_Implementation(AActor* Interactor)
{
    UGameInstance* Game = GetGameInstance();
    UOGNarrativeSessionSubsystem* Session = Game ? Game->GetSubsystem<UOGNarrativeSessionSubsystem>() : nullptr;
    FString Error;
    if (!Session || !Session->OpenWorldSubject(this, Interactor, Error))
        UE_LOG(LogTemp, Warning, TEXT("Narrative interaction unavailable: %s"), *Error);
}

void AOGNarrativeWorldActor::BeginPlay()
{
    Super::BeginPlay();
    if (CanonicalSubjectId.IsValid() && SystemBindings)
    {
        FString Error;
        if (!SystemBindings->BindCanonicalEntity(CanonicalSubjectId,Error))
            UE_LOG(LogTemp,Warning,TEXT("Canonical subject system binding unavailable: %s"),*Error);
    }
}
