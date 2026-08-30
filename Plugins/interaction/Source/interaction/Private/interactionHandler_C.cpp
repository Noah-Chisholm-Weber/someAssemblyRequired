// Fill out your copyright notice in the Description page of Project Settings.

#include "interactionHandler_C.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Camera/CameraComponent.h"

// Sets default values for this component's properties
UinteractionHandler_C::UinteractionHandler_C()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UinteractionHandler_C::OnRegister()
{
	Super::OnRegister();

	ResolveTransformComponent();
}


// Called when the game starts
void UinteractionHandler_C::BeginPlay()
{
	ResolveTransformComponent();

	auto* owner = Cast<APawn>(GetOwner());
	if (owner) {
		parentPawn = owner;
		foundParent = true;
	}
	else {
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Interaction Handler \"%s\" is not parented to an instance of Pawn! The parent owner of this handler is \"%s\""),
			*UKismetSystemLibrary::GetDisplayName(this),
			*UKismetSystemLibrary::GetDisplayName(GetOwner())
		);	
	}
	Super::BeginPlay();
}


// Called every frame
void UinteractionHandler_C::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UinteractionHandler_C::ResolveTransformComponent()
{
	rayCastOriginator = nullptr;

	AActor* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	if (rayCastOriginatorRef.ComponentProperty.IsNone()) {
		if (UCameraComponent* playerCamera = Cast<UCameraComponent>(owner->GetComponentByClass(UCameraComponent::StaticClass()))) {
			rayCastOriginator = playerCamera;
			return;
		}
	}

	rayCastOriginator = Cast<USceneComponent>(
		rayCastOriginatorRef.GetComponent(owner)
	);

	if (!rayCastOriginator)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Interaction Handler \"%s\" has no valid transform component reference on owner \"%s\"."),
			*GetNameSafe(this),
			*GetNameSafe(owner)
		);
	}
}