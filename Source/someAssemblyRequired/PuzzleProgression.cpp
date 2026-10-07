// Fill out your copyright notice in the Description page of Project Settings.


#include "PuzzleProgression.h"
#include "VM.h"
#include "Algo/Reverse.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY(LogPuzzles)

// Variables in a Blueprint struct are saved as name_index_guid, so this takes the name part.
static FProperty* findField(const UScriptStruct* rowStruct, const FString& fieldName)
{
	for (TFieldIterator<FProperty> it(rowStruct); it; ++it) {
		FString name = it->GetName();
		if (name == fieldName) return *it;

		FString head, head2;
		if (name.Split(TEXT("_"), &head, nullptr, ESearchCase::CaseSensitive, ESearchDir::FromEnd)
			&& head.Split(TEXT("_"), &head2, nullptr, ESearchCase::CaseSensitive, ESearchDir::FromEnd)
			&& head2 == fieldName) return *it;
	}
	return nullptr;
}

static bool readInt(const UScriptStruct* rowStruct, const uint8* rowData, const FString& fieldName, int32& out)
{
	FIntProperty* prop = CastField<FIntProperty>(findField(rowStruct, fieldName));
	if (!prop) return false;
	out = prop->GetPropertyValue_InContainer(rowData);
	return true;
}

static bool readText(const UScriptStruct* rowStruct, const uint8* rowData, const FString& fieldName, FText& out)
{
	FTextProperty* prop = CastField<FTextProperty>(findField(rowStruct, fieldName));
	if (!prop) return false;
	out = prop->GetPropertyValue_InContainer(rowData);
	return true;
}

static bool readName(const UScriptStruct* rowStruct, const uint8* rowData, const FString& fieldName, FName& out)
{
	FNameProperty* prop = CastField<FNameProperty>(findField(rowStruct, fieldName));
	if (!prop) return false;
	out = prop->GetPropertyValue_InContainer(rowData);
	return true;
}

static bool readNameArray(const UScriptStruct* rowStruct, const uint8* rowData, const FString& fieldName, TArray<FName>& out)
{
	FArrayProperty* prop = CastField<FArrayProperty>(findField(rowStruct, fieldName));
	if (!prop || !CastField<FNameProperty>(prop->Inner)) return false;
	out = *prop->ContainerPtrToValuePtr<TArray<FName>>(rowData);
	return true;
}

static bool readTestCases(const UScriptStruct* rowStruct, const uint8* rowData, const FString& fieldName, TArray<FVMTestCase>& out)
{
	FArrayProperty* prop = CastField<FArrayProperty>(findField(rowStruct, fieldName));
	if (!prop) return false;
	FStructProperty* inner = CastField<FStructProperty>(prop->Inner);
	if (!inner || inner->Struct != FVMTestCase::StaticStruct()) return false;
	out = *prop->ContainerPtrToValuePtr<TArray<FVMTestCase>>(rowData);
	return true;
}

void UPuzzleProgression::loadPuzzles(UDataTable* table)
{
	puzzles.Reset();
	orderedIds.Reset();

	if (!table || !table->GetRowStruct()) {
		UE_LOG(LogPuzzles, Error, TEXT("loadPuzzles was given a null table!"));
		return;
	}

	const UScriptStruct* rowStruct = table->GetRowStruct();

	for (const FName& rowName : table->GetRowNames()) {
		const uint8* rowData = table->FindRowUnchecked(rowName);
		if (!rowData) continue;

		FPuzzleInfo info;
		info.puzzleId = rowName;

		// These two are needed to order and run a puzzle, so the row is skipped without them.
		if (!readInt(rowStruct, rowData, TEXT("puzzleNumber"), info.puzzleNumber)
			|| !readTestCases(rowStruct, rowData, TEXT("testCases"), info.testCases)) {
			UE_LOG(LogPuzzles, Error, TEXT("Row %s is missing puzzleNumber or testCases, skipping it."), *rowName.ToString());
			continue;
		}

		readText(rowStruct, rowData, TEXT("testName"), info.testName);
		readText(rowStruct, rowData, TEXT("objective"), info.objective);
		readInt(rowStruct, rowData, TEXT("instructionLimit"), info.instructionLimit);
		readInt(rowStruct, rowData, TEXT("parSteps"), info.parSteps);
		readName(rowStruct, rowData, TEXT("nextChallenge"), info.nextChallenge);

		if (!readNameArray(rowStruct, rowData, TEXT("unlockedCommands"), info.unlockedCommands)) {
			UE_LOG(LogPuzzles, Warning, TEXT("Row %s has no unlockedCommands, so it unlocks nothing."), *rowName.ToString());
		}

		puzzles.Add(rowName, info);
	}

	puzzles.GetKeys(orderedIds);
	orderedIds.Sort([this](const FName& a, const FName& b) {
		return puzzles[a].puzzleNumber < puzzles[b].puzzleNumber;
		});
}

TArray<FName> UPuzzleProgression::getPuzzleOrder()
{
	return orderedIds;
}

bool UPuzzleProgression::getPuzzle(FName puzzleId, FPuzzleInfo& outPuzzle)
{
	const FPuzzleInfo* validPuzzle = puzzles.Find(puzzleId);
	if (!validPuzzle) return false;

	outPuzzle = *validPuzzle;
	return true;
}

bool UPuzzleProgression::isPuzzleUnlocked(FName puzzleId)
{
	const int32 index = orderedIds.IndexOfByKey(puzzleId);
	if (index == INDEX_NONE) return false;

	// The first puzzle is always open, every other one needs the one before it done.
	return index == 0 || completed.Contains(orderedIds[index - 1]);
}

bool UPuzzleProgression::isPuzzleCompleted(FName puzzleId)
{
	return completed.Contains(puzzleId);
}

FName UPuzzleProgression::getCurrentPuzzleId()
{
	for (const FName& id : orderedIds) {
		if (!completed.Contains(id)) return id;
	}
	return NAME_None;
}

bool UPuzzleProgression::runPuzzle(UVM* vm, FName puzzleId, const FString& program, TArray<FVMTestResult>& results)
{
	results.Reset();

	if (!isPuzzleUnlocked(puzzleId)) {
		UE_LOG(LogPuzzles, Warning, TEXT("Puzzle %s is locked or does not exist!"), *puzzleId.ToString());
		return false;
	}

	FPuzzleInfo puzzle;
	if (!getPuzzle(puzzleId, puzzle)) return false;

	// A puzzle with no test cases would pass without running anything.
	if (puzzle.testCases.Num() == 0) {
		UE_LOG(LogPuzzles, Warning, TEXT("Puzzle %s has no test cases yet."), *puzzleId.ToString());
		return false;
	}

	if (!tester) tester = NewObject<UVMTester>(this);

	// readPort takes values from the end of the array, so flip the inputs to make the first value listed the first one read.
	TArray<FVMTestCase> cases = puzzle.testCases;
	for (FVMTestCase& testCase : cases) {
		for (FportDataLoader& loader : testCase.presetPorts) Algo::Reverse(loader.data.myData);
	}

	bool allPassed = tester->runTestSuite(vm, program, cases, results);
	if (allPassed) completePuzzle(puzzleId);
	return allPassed;
}

void UPuzzleProgression::completePuzzle(FName puzzleId)
{
	if (completed.Contains(puzzleId)) return;

	FPuzzleInfo puzzle;
	if (!getPuzzle(puzzleId, puzzle)) return;

	completed.Add(puzzleId);

	for (const FName& command : puzzle.unlockedCommands) {
		const FString name = command.ToString().ToUpper();
		if (unlockedNames.Contains(name)) continue;

		unlockedNames.Add(name);
		UVM::setCommandUnlocked(name, true);
		commandUnlocked.Broadcast(name);
	}

	puzzleCompleted.Broadcast(puzzleId);
}

bool UPuzzleProgression::isCommandUnlocked(const FString& commandName)
{
	return unlockedNames.Contains(commandName.ToUpper());
}

void UPuzzleProgression::reapplyUnlocksToVM()
{
	for (const FString& name : unlockedNames) UVM::setCommandUnlocked(name, true);
}

void UPuzzleProgression::resetProgress()
{
	// This doesn't lock the commands again in the VM.
	completed.Reset();
	unlockedNames.Reset();
}