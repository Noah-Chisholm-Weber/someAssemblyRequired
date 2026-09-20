// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Internationalization/Regex.h"
#include "VM.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVM, Log, All);

UENUM()
enum class EparameterType
{
	addr,
	value,
	label,
	addrOrValue
};

UENUM()
enum class EopCode
{
	add,
	mov,
};

USTRUCT()
struct FinstructionDefinition
{
	GENERATED_BODY()

	FString base;
	TArray<EparameterType> params;
	EopCode opCode;

	FinstructionDefinition() {
		base = TEXT("");
		params = TArray<EparameterType>();
		opCode = (EopCode) - 1;
	}

	FinstructionDefinition(FString _base, TArray<EparameterType> _params, EopCode _opCode) : base(_base), params(_params), opCode(_opCode) {

	}

	FString toString() const {
		FString toReturn = base + " ";
		for (const EparameterType& param : params) {
			switch (param)
			{
			case EparameterType::addr:
				toReturn += "{P|R}<id>";
				break;
			case EparameterType::value:
				toReturn += "<int>";
				break;
			case EparameterType::label:
				toReturn += "label";
				break;
			case EparameterType::addrOrValue:
				toReturn += "{P|R}<id> | <int>";
				break;
			default:
				break;
			}
		}
		return toReturn;
	}
};

UENUM()
enum class EopperandType
{
	reg,
	mem,
	port,
	value
};

USTRUCT(BlueprintType)
struct FopperandValue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	EopperandType type;

	UPROPERTY(BlueprintReadWrite)
	int32 value;

	FopperandValue() {
		type = EopperandType::value;
		value = -1;
	}

	FopperandValue(EopperandType _type, int32 _value) : type(_type), value(_value) {

	}
};

USTRUCT(BlueprintType)
struct FcompiledInstruction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	EopCode opCode;
	
	UPROPERTY(BlueprintReadWrite)
	FopperandValue op1;
	
	UPROPERTY(BlueprintReadWrite)
	FopperandValue op2;

	UPROPERTY(BlueprintReadWrite)
	FopperandValue op3;
};

UENUM(BlueprintType)
enum class EReadWriteEnable : uint8
{
	None = 0x0,
	read = 0x1,
	write = 0x2,
	readWrite = 0x3
};

USTRUCT(BlueprintType)
struct FPort
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TArray<int32> myData;

	UPROPERTY(BlueprintReadWrite)
	uint8 rwFlags;

	bool operator==(const FPort& Other) const
	{
		return rwFlags == Other.rwFlags && myData == Other.myData;
	}

	FPort(TArray<int32> _myData, uint8 _rwFlags) : myData(_myData), rwFlags(_rwFlags) {

	}

	FPort() {
		rwFlags = 0;
	}
};

FORCEINLINE uint32 GetTypeHash(const FPort& port)
{
	// Combine hashes using Unreal's built-in HashCombine function
	uint32 Hash = GetTypeHash(port.rwFlags);
	Hash = HashCombine(Hash, GetTypeHash(port.myData));
	return Hash;
}

USTRUCT(BlueprintType)
struct FportDataLoader
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	uint8 portNumber;

	UPROPERTY(BlueprintReadWrite)
	FPort data;

	FportDataLoader() {
		portNumber = 0;
		data = FPort();
	}

	FportDataLoader(uint8 _portNumber, FPort _data) : portNumber(_portNumber), data(_data) {

	}

	FportDataLoader(uint8 _portNumber, TArray<int32> _data) : portNumber(_portNumber), data(_data, (uint8)EReadWriteEnable::readWrite) {

	}

	bool operator==(const FportDataLoader& Other) const
	{
		return portNumber == Other.portNumber && data == Other.data;
	}
};

// 2. Declare the Custom Hash Function globally (Must match this exact signature)
FORCEINLINE uint32 GetTypeHash(const FportDataLoader& dataLoader)
{
	// Combine hashes using Unreal's built-in HashCombine function
	uint32 Hash = GetTypeHash(dataLoader.portNumber);
	Hash = HashCombine(Hash, GetTypeHash(dataLoader.data));
	return Hash;
}
USTRUCT(BlueprintType)
struct FProgramResults
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TMap<uint8, FPort> ports;

	UPROPERTY(BlueprintReadWrite)
	bool ranWithoutErrors;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProgramEnded, FProgramResults, results);

UCLASS(BlueprintType)
class SOMEASSEMBLYREQUIRED_API UVM : public UObject
{
	GENERATED_BODY()

public:
	UVM();

private:
	static TMap<FString, FinstructionDefinition> instructionSet;
	static FRegexPattern lineCheckerPattern;
	
	uint32 maxReg;
	uint8 maxPort;
	TArray<int32> registers;
	TMap<uint8, FPort> ports;
	bool interrupt = false;
	uint32 pc = 0;
	TArray<FcompiledInstruction> curProgram;
	float runSpeed = 0.75f;
	FTimerHandle stepTimer;
	bool runningProgram = false;
	bool ranWithoutErrors = true;

	int readRegister(uint32 reg);
	int readPort(uint8 port);
	int readOperand(FopperandValue op);

	void writeRegister(uint32 reg, int32 value);
	void writePort(uint8 port, int32 value);
	void writeValue(FopperandValue location, int32 value);

	bool verifyLine(FString line, FcompiledInstruction& instruction);
	bool executeInstruction(FcompiledInstruction& instruction);

	static void registerInstruction(FinstructionDefinition newInstruction);

	void raiseInterrupt(const FString& debugMessage);

public:
	UFUNCTION(BlueprintCallable, Category = "Program Evaluation")
	bool compileProgram(FString program, TArray<FcompiledInstruction>& instructions);

	UFUNCTION(BlueprintCallable, Category = "Program Evaluation")
	TArray<int32> getPort(uint8 port);

	UFUNCTION(BlueprintCallable, Category = "Program Evaluation")
	FPort getPortFull(uint8 port);

	UFUNCTION(BlueprintCallable, Category = "Program Debugging")
	int32 getRegister(int32 reg);

	UFUNCTION(BlueprintCallable, Category = "Program Execution")
	bool runProgram(FString program);

	UFUNCTION(BlueprintCallable, Category = "Program Execution")
	void resetMachine(int32 _maxReg, int32 _maxPort, TArray<FportDataLoader> preLoadedPorts);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Program Execution/Tests")
	void testRunProgram();

	UFUNCTION(BlueprintCallable, Category = "Program Execution")
	void unPauseProgram();

	UFUNCTION(BlueprintCallable, Category = "Program Execution")
	bool stepProgram();

	UFUNCTION(BlueprintCallable, Category = "Program Execution")
	void pauseProgram();

	UPROPERTY(BlueprintAssignable, Category = "Program Execution")
	FProgramEnded programEnded;
};
