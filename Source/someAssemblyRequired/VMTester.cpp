// Fill out your copyright notice in the Description page of Project Settings.


#include "VMTester.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogVMTester)

bool UVMTester::compareOutput(const TArray<int32>& actual, const TArray<int32>& expected, FString& outMessage)
{
	// 1) Number of values.
	if (actual.Num() != expected.Num()) {
		outMessage = FString::Printf(TEXT("Expected %d output value(s) but got %d."), expected.Num(), actual.Num());
		return false;
	}

	// 2) Order + 3) the values themselves, all in one pass since getPort
	// returns the port contents in the order they were written.
	for (int32 i = 0; i < expected.Num(); i++) {
		if (actual[i] != expected[i]) {
			outMessage = FString::Printf(TEXT("Value at position %d did not match. Expected %d but got %d."), i, expected[i], actual[i]);
			return false;
		}
	}

	outMessage = TEXT("Output matched expected values.");
	return true;
}

bool UVMTester::runTestCase(UVM* vm, const FString& program, const FVMTestCase& testCase, FVMTestResult& outResult)
{
	outResult = FVMTestResult();
	outResult.testName = testCase.testName;
	outResult.expectedOutput = testCase.expectedOutput;

	if (!vm) {
		outResult.passed = false;
		outResult.ranWithoutError = false;
		outResult.message = TEXT("No VM instance was provided to runTestCase!");
		return false;
	}

	// Every test case starts from a completely fresh machine state, per the
	// V0 spec: "The VM is reset to a fresh state between tests."
	vm->resetMachine(testCase.maxReg, testCase.maxPort, testCase.presetPorts);

	outResult.ranWithoutError = vm->runProgram(program);
	outResult.actualOutput = vm->getPort(testCase.outputPort);

	if (!outResult.ranWithoutError) {
		outResult.passed = false;
		outResult.message = TEXT("Program did not finish running (it raised an interrupt/error).");
		return false;
	}

	FString compareMessage;
	outResult.passed = compareOutput(outResult.actualOutput, testCase.expectedOutput, compareMessage);
	outResult.message = compareMessage;
	return outResult.passed;
}

bool UVMTester::runTestSuite(UVM* vm, const FString& program, const TArray<FVMTestCase>& testCases, TArray<FVMTestResult>& outResults)
{
	outResults.Reset();
	bool allPassed = true;

	// "The same player program is run independently against each test case."
	for (const FVMTestCase& testCase : testCases) {
		FVMTestResult result;
		bool passed = runTestCase(vm, program, testCase, result);
		outResults.Add(result);
		allPassed &= passed;
	}

	// "A solution succeeds only if all test cases succeed."
	return allPassed;
}

TArray<FVMTestCase> UVMTester::getDefaultAdditionTestSuite()
{
	TArray<FVMTestCase> suite;

	// V0 scope only needs to support the first addition-based challenge:
	// two values are read from the input port (P0) and their sum is
	// expected to show up as the single value on the output port (P1).
	suite.Add(FVMTestCase(TEXT("Test 1: 2 + 2"), 0, 1, { FportDataLoader(0, {2, 2}) }, 1, {4}));
	suite.Add(FVMTestCase(TEXT("Test 2: 5 + 7"), 0, 1, { FportDataLoader(0, {5, 7}) }, 1, {12}));
	suite.Add(FVMTestCase(TEXT("Test 3: -3 + 10"), 0, 1, { FportDataLoader(0, {-3, 10}) }, 1, {7}));
	suite.Add(FVMTestCase(TEXT("Test 4: 0 + 0"), 0, 1, { FportDataLoader(0, {0, 0}) }, 1, {0}));
	suite.Add(FVMTestCase(TEXT("Test 5: 100 + (-100)"), 0, 1, { FportDataLoader(0, {100, -100}) }, 1, {0}));
	// Extra test case, per Bob's Uncle's suggestion in team chat (9/8, 11:02 AM).
	suite.Add(FVMTestCase(TEXT("Test 6 (extra): 15 + (-20)"), 0, 1, { FportDataLoader(0, {15, -20}) }, 1, {-5}));

	return suite;
}

void UVMTester::testRunTester()
{
	UVM* vm = NewObject<UVM>(this);
	TArray<FVMTestCase> testSuite = getDefaultAdditionTestSuite();

	// A correct example program: read both values from P0, add them, and
	// push the sum onto P1.
	const FString correctProgram = TEXT("ADD P0 P0 P1");

	// An intentionally incorrect example program: only reads one value from
	// P0 and moves it straight to P1 without adding anything. This should
	// be recognized as failing at least one test case.
	const FString incorrectProgram = TEXT("MOV P0 P1");

	for (const auto& pass : { TPair<FString, FString>(TEXT("CORRECT program"), correctProgram), TPair<FString, FString>(TEXT("INCORRECT program"), incorrectProgram) }) {
		TArray<FVMTestResult> results;
		bool allPassed = runTestSuite(vm, pass.Value, testSuite, results);

		FString header = FString::Printf(TEXT("=== %s (\"%s\") -> %s ==="), *pass.Key, *pass.Value, allPassed ? TEXT("ALL TESTS PASSED") : TEXT("AT LEAST ONE TEST FAILED"));
		UE_LOG(LogVMTester, Log, TEXT("%s"), *header);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, allPassed ? FColor::Green : FColor::Red, header);

		for (const FVMTestResult& result : results) {
			FString line = FString::Printf(TEXT("  [%s] %s - %s"), result.passed ? TEXT("PASS") : TEXT("FAIL"), *result.testName, *result.message);
			UE_LOG(LogVMTester, Log, TEXT("%s"), *line);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, result.passed ? FColor::Green : FColor::Yellow, line);
		}
	}
}
