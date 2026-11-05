#include "Narrative/OGNarrativeWorldActor.h"
#include "Content/OGSystemBindingComponent.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Narrative/OGNarrativeSessionSubsystem.h"

namespace
{
bool CanInteractWithNarrativeSubject(const AActor& Subject, AActor* Interactor)
{
    const UGameInstance* Game = Subject.GetGameInstance();
    const auto* Session = Game ? Game->GetSubsystem<UOGNarrativeSessionSubsystem>() : nullptr;
    FString Error;
    return Session && Session->CanOpenWorldSubject(Subject, Interactor, Error);
}

void InteractWithNarrativeSubject(AActor& Subject, AActor* Interactor)
{
    UGameInstance* Game = Subject.GetGameInstance();
    auto* Session = Game ? Game->GetSubsystem<UOGNarrativeSessionSubsystem>() : nullptr;
    FString Error;
    if (!Session || !Session->OpenWorldSubject(&Subject, Interactor, Error))
        UE_LOG(LogTemp, Warning, TEXT("Narrative interaction unavailable: %s"), *Error);
}
}

AOGNarrativeWorldActor::AOGNarrativeWorldActor()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AuthoredNarrativeRoot"));
    SystemBindings=CreateDefaultSubobject<UOGSystemBindingComponent>(TEXT("SharedSystemBindings"));
}

bool AOGNarrativeWorldActor::CanInteract_Implementation(AActor* Interactor) const
{
    return CanInteractWithNarrativeSubject(*this, Interactor);
}

FText AOGNarrativeWorldActor::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
    return InteractionPrompt;
}

void AOGNarrativeWorldActor::Interact_Implementation(AActor* Interactor)
{
    InteractWithNarrativeSubject(*this, Interactor);
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

AOGNarrativeWorldCharacter::AOGNarrativeWorldCharacter()
{
    SystemBindings = CreateDefaultSubobject<UOGSystemBindingComponent>(TEXT("SharedSystemBindings"));
}

bool AOGNarrativeWorldCharacter::CanInteract_Implementation(AActor* Interactor) const
{
    return CanInteractWithNarrativeSubject(*this, Interactor);
}

FText AOGNarrativeWorldCharacter::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
    return InteractionPrompt;
}

void AOGNarrativeWorldCharacter::Interact_Implementation(AActor* Interactor)
{
    InteractWithNarrativeSubject(*this, Interactor);
}
