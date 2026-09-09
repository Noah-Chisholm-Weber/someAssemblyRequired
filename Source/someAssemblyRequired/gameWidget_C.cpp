// Fill out your copyright notice in the Description page of Project Settings.

#include "gameWidget_C.h"
#include "VM.h"
#include "VMTester.h"

void UgameWidget_C::NativeConstruct()
{
    UE_LOG(LogTemp, Warning, TEXT("GAME WIDGET NATIVE CONSTRUCT CALLED"));

    VM = NewObject<UVM>(this);

    testSuite = NewObject<UVMTester>(this);

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

void UgameWidget_C::runTest(FString program) {
	TArray<FVMTestCase> suite = testSuite->getDefaultAdditionTestSuite();

	TArray<FVMTestResult> results;
	bool allPassed = testSuite->runTestSuite(VM, program, suite, results);

	FString header = FString::Printf(TEXT("=== CORRECT program (\"%s\") -> %s ==="), *program, allPassed ? TEXT("ALL TESTS PASSED") : TEXT("AT LEAST ONE TEST FAILED"));
	UE_LOG(LogVMTester, Log, TEXT("%s"), *header);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, allPassed ? FColor::Green : FColor::Red, header);

	for (const FVMTestResult& result : results) {
		FString line = FString::Printf(TEXT("  [%s] %s - %s"), result.passed ? TEXT("PASS") : TEXT("FAIL"), *result.testName, *result.message);
		UE_LOG(LogVMTester, Log, TEXT("%s"), *line);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, result.passed ? FColor::Green : FColor::Yellow, line);
	}
}