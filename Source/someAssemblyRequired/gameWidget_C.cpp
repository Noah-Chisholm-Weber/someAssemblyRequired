// Fill out your copyright notice in the Description page of Project Settings.

#include "gameWidget_C.h"
#include "VM.h"

void UgameWidget_C::NativeConstruct()
{
    UE_LOG(LogTemp, Warning, TEXT("GAME WIDGET NATIVE CONSTRUCT CALLED"));

    VM = NewObject<UVM>(this);

    //THESE TWO LINES CAN BE COMMENTED OUT WHEN IMPLEMENTING THE TEST CASES, THIS IS JUST SAMPLE DATA INJECTED INTO THE VM
    TArray<FportDataLoader> preLoadedPorts;
    preLoadedPorts.Add(FportDataLoader(0, { 2, 2 }));

    VM->resetMachine(0, 1, preLoadedPorts);

    UE_LOG(LogTemp, Warning, TEXT("P0 initialized with %d values"), VM->getPort(0).Num());

    Super::NativeConstruct();
}

UVM* UgameWidget_C::GetVM() const
{
	return VM;
}

FText UgameWidget_C::PortValuesToText(const TArray<int32>& Values) const
{
    FString Result = TEXT("[");

    for (int32 i = 0; i < Values.Num(); i++)
    {
        Result += FString::FromInt(Values[i]);

        if (i < Values.Num() - 1)
        {
            Result += TEXT(", ");
        }
    }

    Result += TEXT("]");

    return FText::FromString(Result);
}

