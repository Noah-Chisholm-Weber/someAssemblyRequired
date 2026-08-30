// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "interactionHandler_C.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable )
class INTERACTION_API UinteractionHandler_C : public UActorComponent
{
	GENERATED_BODY()
public:	
	// Sets default values for this component's properties
	UinteractionHandler_C();

	UPROPERTY(EditAnywhere, Category = "Parameters", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SceneComponent"))
	FComponentReference rayCastOriginatorRef;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Default")
	TObjectPtr<USceneComponent> rayCastOriginator = nullptr;

private:
    void ResolveTransformComponent();

public:
	UPROPERTY(BlueprintReadWrite, Category = "Default")
	APawn* parentPawn;

	UPROPERTY(BlueprintReadOnly, Category = "Default")
	bool foundParent = false;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void OnRegister() override;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
