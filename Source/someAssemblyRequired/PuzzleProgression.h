// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PuzzleData.h"
#include "PuzzleProgression.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPuzzles, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPuzzleCompleted, FName, puzzleId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCommandUnlocked, FString, commandName);

// Keeps track of which puzzles are done and which commands the player has unlocked.
// It is a game instance subsystem so the progress stays when the level changes.
UCLASS()
class SOMEASSEMBLYREQUIRED_API UPuzzleProgression : public UGameInstanceSubsystem
{
	GENERATED_BODY()

private:
	UPROPERTY()
	UVMTester* tester;

	TMap<FName, FPuzzleInfo> puzzles;
	TArray<FName> orderedIds;
	TSet<FName> completed;
	TSet<FString> unlockedNames;

public:
	// Reads every row of challenges_DT. The table uses a Blueprint struct, so the fields are found by name.
	UFUNCTION(BlueprintCallable, Category = "Puzzles")
	void loadPuzzles(UDataTable* table);

	UFUNCTION(BlueprintCallable, Category = "Puzzles")
	TArray<FName> getPuzzleOrder();

	UFUNCTION(BlueprintCallable, Category = "Puzzles")
	bool getPuzzle(FName puzzleId, FPuzzleInfo& outPuzzle);

	UFUNCTION(BlueprintCallable, Category = "Puzzles")
	bool isPuzzleUnlocked(FName puzzleId);

	UFUNCTION(BlueprintCallable, Category = "Puzzles")
	bool isPuzzleCompleted(FName puzzleId);

	// Returns NAME_None once every puzzle is completed.
	UFUNCTION(BlueprintCallable, Category = "Puzzles")
	FName getCurrentPuzzleId();

	// Runs every test case of the puzzle against the program, completes the puzzle if they all pass.
	UFUNCTION(BlueprintCallable, Category = "Puzzles")
	bool runPuzzle(UVM* vm, FName puzzleId, const FString& program, TArray<FVMTestResult>& results);

	UFUNCTION(BlueprintCallable, Category = "Puzzles")
	void completePuzzle(FName puzzleId);

	UFUNCTION(BlueprintCallable, Category = "Puzzles")
	bool isCommandUnlocked(const FString& commandName);

	// Call after the commands have been registered with the VM.
	UFUNCTION(BlueprintCallable, Category = "Puzzles")
	void reapplyUnlocksToVM();

	UFUNCTION(BlueprintCallable, Category = "Puzzles")
	void resetProgress();

	UPROPERTY(BlueprintAssignable, Category = "Puzzles")
	FPuzzleCompleted puzzleCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Puzzles")
	FCommandUnlocked commandUnlocked;
};