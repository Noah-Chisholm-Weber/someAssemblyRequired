// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VM.h"
#include "VMTester.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVMTester, Log, All);

// One test case: how to reset/preload the machine, and what the output port
// should look like after the (shared, static) program has run to completion.
USTRUCT(BlueprintType)
struct FVMTestCase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString testName;

	UPROPERTY(BlueprintReadWrite)
	int32 maxReg;

	UPROPERTY(BlueprintReadWrite)
	int32 maxPort;

	UPROPERTY(BlueprintReadWrite)
	TArray<FportDataLoader> presetPorts;

	UPROPERTY(BlueprintReadWrite)
	uint8 outputPort;

	UPROPERTY(BlueprintReadWrite)
	TArray<int32> expectedOutput;

	FVMTestCase()
	{
		testName = TEXT("");
		maxReg = 0;
		maxPort = 1;
		presetPorts = TArray<FportDataLoader>();
		outputPort = 1;
		expectedOutput = TArray<int32>();
	}

	FVMTestCase(FString _testName, int32 _maxReg, int32 _maxPort, TArray<FportDataLoader> _presetPorts, uint8 _outputPort, TArray<int32> _expectedOutput)
		: testName(_testName), maxReg(_maxReg), maxPort(_maxPort), presetPorts(_presetPorts), outputPort(_outputPort), expectedOutput(_expectedOutput)
	{

	}
};

// The result of running a single FVMTestCase against a program.
USTRUCT(BlueprintType)
struct FVMTestResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString testName;

	UPROPERTY(BlueprintReadWrite)
	bool passed;

	UPROPERTY(BlueprintReadWrite)
	bool ranWithoutError;

	UPROPERTY(BlueprintReadWrite)
	TArray<int32> actualOutput;

	UPROPERTY(BlueprintReadWrite)
	TArray<int32> expectedOutput;

	UPROPERTY(BlueprintReadWrite)
	FString message;

	FVMTestResult()
	{
		testName = TEXT("");
		passed = false;
		ranWithoutError = false;
		actualOutput = TArray<int32>();
		expectedOutput = TArray<int32>();
		message = TEXT("");
	}
};

// Evaluates a player/example program against one or more FVMTestCases using
// a UVM instance. Resets the machine fresh for every test case, as required
// by the V0 spec ("Program evaluation").
UCLASS(BlueprintType)
class SOMEASSEMBLYREQUIRED_API UVMTester : public UObject
{
	GENERATED_BODY()

public:
	// Runs one test case against the given program on the given VM and fills
	// out outResult. Returns the same value as outResult.passed for convenience.
	UFUNCTION(BlueprintCallable, Category = "Program Evaluation/Tests")
	bool runTestCase(UVM* vm, const FString& program, const FVMTestCase& testCase, FVMTestResult& outResult);

	// Runs every test case in testCases against the same program, resetting
	// the VM between each one. Returns true only if every test case passed.
	UFUNCTION(BlueprintCallable, Category = "Program Evaluation/Tests")
	bool runTestSuite(UVM* vm, const FString& program, const TArray<FVMTestCase>& testCases, TArray<FVMTestResult>& outResults);

	// Convenience wrapper: builds the default V0 addition-challenge test
	// suite (5 base cases + 1 extra, per team chat) so Blueprint doesn't have
	// to hand-build the test data.
	UFUNCTION(BlueprintCallable, Category = "Program Evaluation/Tests")
	static TArray<FVMTestCase> getDefaultAdditionTestSuite();

	// Editor-only demonstration, in the same spirit as UVM::testRunProgram.
	// Runs both a correct example program and an intentionally incorrect one
	// against the default test suite and logs/prints the results, so this
	// can be exercised without any Blueprint wiring at all.
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Program Evaluation/Tests")
	void testRunTester();

private:
	static bool compareOutput(const TArray<int32>& actual, const TArray<int32>& expected, FString& outMessage);
};
