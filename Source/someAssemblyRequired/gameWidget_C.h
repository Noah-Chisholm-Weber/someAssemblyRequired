// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "gameWidget_C.generated.h"

class UVM;

/**
 * Main game UI widget.
 */
UCLASS()
class SOMEASSEMBLYREQUIRED_API UgameWidget_C : public UUserWidget
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	UPROPERTY()
	UVM* VM;

public:

	UFUNCTION(BlueprintCallable, Category = "VM")
	UVM* GetVM() const;

	UFUNCTION(BlueprintCallable, Category = "VM")
	FText PortValuesToText(const TArray<int32>& Values) const;
};