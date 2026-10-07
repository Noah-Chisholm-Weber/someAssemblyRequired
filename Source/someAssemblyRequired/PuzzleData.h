// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VMTester.h"
#include "PuzzleData.generated.h"

// One puzzle, read from a row of challenges_DT (row struct challenge_S).
// The row name is the puzzle id (challenge1, challenge2, ...).
USTRUCT(BlueprintType)
struct FPuzzleInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FName puzzleId;

	// Puzzles unlock in order of this value, lowest first.
	UPROPERTY(BlueprintReadWrite)
	int32 puzzleNumber;

	UPROPERTY(BlueprintReadWrite)
	FText testName;

	UPROPERTY(BlueprintReadWrite)
	FText objective;

	UPROPERTY(BlueprintReadWrite)
	int32 instructionLimit;

	UPROPERTY(BlueprintReadWrite)
	int32 parSteps;

	UPROPERTY(BlueprintReadWrite)
	FName nextChallenge;

	// Register count is set per test case with maxReg. Preset port values are listed in the order the program reads them.
	UPROPERTY(BlueprintReadWrite)
	TArray<FVMTestCase> testCases;

	// Commands unlocked the first time this puzzle is completed. LABELS is listed here too even though it isn't an instruction.
	UPROPERTY(BlueprintReadWrite)
	TArray<FName> unlockedCommands;

	FPuzzleInfo() {
		puzzleId = NAME_None;
		puzzleNumber = 0;
		testName = FText();
		objective = FText();
		instructionLimit = 0;
		parSteps = 0;
		nextChallenge = NAME_None;
		testCases = TArray<FVMTestCase>();
		unlockedCommands = TArray<FName>();
	}
};