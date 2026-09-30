// Fill out your copyright notice in the Description page of Project Settings.


#include "VM.h"
#include "Kismet/GameplayStatics.h"
#include "Algo/LevenshteinDistance.h"

DEFINE_LOG_CATEGORY(LogVM)

FRegexPattern UVM::lineCheckerPattern = FRegexPattern(R"(^([A-Z]+)(( +([RP]\d+|-?\d+))+$))");
TMap<FString, FinstructionDefinition> UVM::instructionSet = TMap<FString, FinstructionDefinition>();

UVM::UVM() {

}

int UVM::readRegister(uint32 reg) {
	if (registers.IsValidIndex(reg)) return registers[reg];
	else raiseInterrupt(FString::Printf(TEXT("%d is not a valid register! This machine only has registers 0-%d"), reg, maxReg ));
	return -1;
}

int UVM::readPort(uint8 port) {
	FPort* validPort = ports.Find(port);
	if (validPort) {
		if (((uint8)validPort->rwFlags & (uint8)EReadWriteEnable::read) == 0) {
			raiseInterrupt(FString::Printf(TEXT("Attempting to read from a write only port, port number: %d!"), port));
			return -1;
		}
		stateChanged.Broadcast();
		if (validPort->myData.Num() > 0) return validPort->myData.Pop();
		raiseInterrupt(FString::Printf(TEXT("Attempting to read from an empty port, port number: %d!"), port));
		return -1;
	} else raiseInterrupt(FString::Printf(TEXT("%d is not a valid port! This machine only supports ports 0-%d"), port, maxPort));
	return -1;
}

int UVM::readOperand(FopperandValue op)
{
	switch (op.type)
	{
	case EopperandType::reg:
		return readRegister(op.value);
	case EopperandType::mem:
		UE_LOG(LogVM, Error, TEXT("Trying to read operand of type memory when memory has not been implemented!"));
		return -1;
	case EopperandType::port:
		return readPort(op.value);
	case EopperandType::value:
		return op.value;
	default:
		UE_LOG(LogVM, Error, TEXT("Trying to read operand of unknown type!"));
		return -1;
	}
}

void UVM::writeRegister(uint32 reg, int32 value) {
	if (registers.IsValidIndex(reg)) {
		registers[reg] = value;
		stateChanged.Broadcast();
	} else raiseInterrupt(FString::Printf(TEXT("%d is not a valid register! This machine only has registers 0-%d"), reg, maxReg));
}

void UVM::writePort(uint8 port, int32 value) {
	FPort* validPort = ports.Find(port);
	if (validPort) {
		if (((uint8)validPort->rwFlags & (uint8)EReadWriteEnable::write) == 0) {
			raiseInterrupt(FString::Printf(TEXT("Attempting to write to a read only port, port number: %d!"), port));
		}
		else {
			validPort->myData.Push(value);
			stateChanged.Broadcast();
		}
	} else raiseInterrupt(FString::Printf(TEXT("%d is not a valid port! This machine only supports ports 0-%d"), port, maxPort));
}

void UVM::writeValue(FopperandValue location, int32 value) {
	switch (location.type)
	{
	case EopperandType::reg:
		writeRegister(location.value, value);
		break;
	case EopperandType::mem:
		UE_LOG(LogVM, Error, TEXT("Trying to write value of type memory when memory has not been implemented!"));
		break;
	case EopperandType::port:
		writePort(location.value, value);
		break;
	case EopperandType::value:
		UE_LOG(LogVM, Warning, TEXT("Cannot write to a value!"));
		break;
	default:
		UE_LOG(LogVM, Error, TEXT("Trying to write value of unknown type!"));
		break;
	}
}

#define LOCTEXT_NAMESPACE "compileErrors"

bool UVM::verifyLine(FString line, FcompiledInstruction& compiledInstruction, int32 lineNumber) {
	line = line.ToUpper();

	if (line.TrimStartAndEnd().IsEmpty()) {
		compiledInstruction.opCode = EopCode::empty;
		return true;
	}

	FRegexMatcher matcher(lineCheckerPattern, line);

	if (!matcher.FindNext()) {
		errorEvent.Broadcast(FCompileError(LOCTEXT("regexError", "Does not match the expected command format."), lineNumber));
		return false;
	}

	FString base = matcher.GetCaptureGroup(1).ToUpper();
	FinstructionDefinition* validDef = instructionSet.Find(base);

	if (!validDef) {
		int32 bestDistance = MAX_int32;
		FString bestMatch;

		for (const auto& pair : instructionSet) {
			const FString& command = pair.Key;
			const int32 distance = Algo::LevenshteinDistance(base, command);

			if (distance < bestDistance) {
				bestDistance = distance;
				bestMatch = command;
			}
		}

		if (bestDistance <= 2) errorEvent.Broadcast(FCompileError(FText::FormatNamed(LOCTEXT("instructionDoesNotExistWSuggestion", "{instBase} does not exist! Did you mean {suggestion}?"), TEXT("instBase"), FText::FromString(base), TEXT("suggestion"), FText::FromString(bestMatch)), lineNumber));
		else errorEvent.Broadcast(FCompileError(FText::FormatNamed(LOCTEXT("instructionDoesNotExist", "{instBase} does not exist!"), TEXT("instBase"), FText::FromString(base)), lineNumber));

		return false;
	}

	if (!validDef->isUnlocked) {
		UE_LOG(LogVM, Warning, TEXT("%s base was marked as locked!"), *base);
		return false;
	}

	compiledInstruction.opCode = validDef->opCode;

	FString paramString = matcher.GetCaptureGroup(2);
	TArray<FString> params;
	paramString.ParseIntoArray(params, TEXT(" "));

	int32 counter = 0;
	FString curParam;
	TCHAR* end;
	TCHAR firstChar;

	if (params.Num() != validDef->params.Num()) {
		if (params.Num() < validDef->params.Num()) {
			errorEvent.Broadcast(FCompileError(FText::FormatNamed(LOCTEXT("tooFewParams", "There were too few parameters for the {base} command! Expected: {expected}"), TEXT("base"), FText::FromString(base), TEXT("expected"), FText::FromString(validDef->toString())), lineNumber));

			UE_LOG(LogVM, Error, TEXT("There were too few params for the %s command when verifying a %s! Expected: %s"), *base, *line, *validDef->toString());
		} else {
			errorEvent.Broadcast(FCompileError(FText::FormatNamed(LOCTEXT("tooManyParams", "There were too many parameters for the {base} command! Expected: {expected}" ), TEXT("base"), FText::FromString(base), TEXT("expected"), FText::FromString(validDef->toString())), lineNumber));

			UE_LOG(LogVM, Error, TEXT("There were too many params for the %s command when verifying a %s! Expected: %s"), *base, *line, *validDef->toString());
		}

		return false;
	}

	static const FTextFormat notAnAddrFormat(
		LOCTEXT(
			"notAnAddr",
			"The command {base} expects an address for parameter number {paramNum}. "
			"{got} is not a recognized address! Expected 'R' or 'P' followed by an integer. Example: R0"
		)
	);

	static const FTextFormat notAValueFormat(
		LOCTEXT(
			"notAValue",
			"The command {base} expects a value for parameter number {paramNum}. "
			"{got} is not an integer! Expected a whole value between -2,147,483,648 and 2,147,483,647. Example: 69"
		)
	);

	static const FTextFormat notAnAddrOrValueValueFormat(
		LOCTEXT(
			"notAnAddrOrValueValue",
			"The command {base} expects either a value or an address for parameter number {paramNum}. "
			"{got} is not an integer! Expected a whole value between -2,147,483,648 and 2,147,483,647. Example: 69"
		)
	);

	static const FTextFormat notAnAddrOrValueAddrFormat(
		LOCTEXT(
			"notAnAddrOrValueAddr",
			"The command {base} expects either a value or an address for parameter number {paramNum}. "
			"{got} is not a recognized address! Expected 'R' or 'P' followed by an integer. Example: R0"
		)
	);

	static const FTextFormat portOutOfBoundsFormat(
		LOCTEXT(
			"portOutOfBounds",
			"When parsing the port for parameter number {paramNum}. "
			"P{got} is out of bounds, this machine has ports 0 through {max}."
		)
	);

	static const FTextFormat regOutOfBoundsFormat(
		LOCTEXT(
			"regOutOfBounds",
			"When parsing the register for parameter number {paramNum}. "
			"R{got} is out of bounds, this machine has registers 0 through {max}."
		)
	);

	static const FTextFormat addrNegFormat(
		LOCTEXT(
			"addrNeg",
			"When parsing the address for parameter number {paramNum}. "
			"The address id cannot be negative."
		)
	);

	for (const Fparameter& expectedParam : validDef->params) {
		curParam = params[counter];
		const EparameterType type = expectedParam.type;

		switch (type) {
		case EparameterType::addr: {
			firstChar = curParam[0];

			if (firstChar != 'R' && firstChar != 'P') {
				errorEvent.Broadcast(FCompileError(FText::FormatNamed(notAnAddrFormat, TEXT("base"), FText::FromString(base), TEXT("paramNum"), counter + 1, TEXT("got"), FText::FromString(curParam)), lineNumber));

				UE_LOG(LogVM, Warning, TEXT("Could not find 'R' or 'P' when parsing an address only field! Line: %s"), *line);

				return false;
			}

			const TCHAR* numberStart = *curParam + 1;
			const int64 parsedAddress = FCString::Strtoi64(numberStart, &end, 10);

			if (end == numberStart || *end != '\0' || parsedAddress < MIN_int32 || parsedAddress > MAX_int32) {
				errorEvent.Broadcast(FCompileError(FText::FormatNamed(notAnAddrFormat, TEXT("base"), FText::FromString(base), TEXT("paramNum"), counter + 1, TEXT("got"), FText::FromString(curParam)), lineNumber));

				UE_LOG(LogVM, Warning, TEXT("Could not parse a valid int32 for the address ID! Line: %s"), *line);

				return false;
			}

			if (parsedAddress < 0) {
				errorEvent.Broadcast(FCompileError(FText::FormatNamed(addrNegFormat, TEXT("paramNum"), counter + 1), lineNumber));
				return false;
			}

			const uint32 address = static_cast<uint32>(parsedAddress);
			if (firstChar == 'R') {
				if (address > maxReg) {
					errorEvent.Broadcast(FCompileError(FText::FormatNamed(regOutOfBoundsFormat, TEXT("paramNum"), counter + 1, TEXT("got"), address, TEXT("max"), maxReg), lineNumber));
					return false;
				}
			}
			else {
				if (address > maxPort) {
					errorEvent.Broadcast(FCompileError(FText::FormatNamed(portOutOfBoundsFormat, TEXT("paramNum"), counter + 1, TEXT("got"), address, TEXT("max"), maxPort), lineNumber));
					return false;
				}
			}

			if (!compiledInstruction.addParam(FopperandValue(firstChar, address))) return false;

			break;
		}

		case EparameterType::value: {
			const TCHAR* numberStart = *curParam;

			const int64 parsedValue = FCString::Strtoi64(numberStart, &end, 10);

			if (end == numberStart || *end != '\0' || parsedValue < MIN_int32 || parsedValue > MAX_int32) {
				errorEvent.Broadcast(FCompileError(FText::FormatNamed(notAValueFormat, TEXT("base"), FText::FromString(base), TEXT("paramNum"), counter + 1, TEXT("got"), FText::FromString(curParam)), lineNumber));

				UE_LOG(LogVM, Warning, TEXT("Could not parse a valid int32 for value only parameter! Line: %s"), *line);

				return false;
			}

			const int32 value = static_cast<int32>(parsedValue);

			if (!compiledInstruction.addParam(FopperandValue(EopperandType::value, value))) return false;

			break;
		}

		case EparameterType::label: {
			UE_LOG(LogVM, Error, TEXT("Label not implemented yet!"));
			return false;
		}

		case EparameterType::addrOrValue: {
			firstChar = curParam[0];

			if (firstChar != 'R' && firstChar != 'P') {
				const TCHAR* numberStart = *curParam;
				const int64 parsedValue = FCString::Strtoi64(numberStart, &end, 10);

				if (end == numberStart || *end != '\0' || parsedValue < MIN_int32 || parsedValue > MAX_int32) {
					errorEvent.Broadcast(FCompileError(FText::FormatNamed(notAnAddrOrValueValueFormat, TEXT("base"), FText::FromString(base), TEXT("paramNum"), counter + 1, TEXT("got"), FText::FromString(curParam)), lineNumber));
					UE_LOG(LogVM, Warning, TEXT("Could not parse a valid int32 for address or value parameter! Line: %s"), *line);
					return false;
				}

				const int32 value = static_cast<int32>(parsedValue);

				if (!compiledInstruction.addParam(FopperandValue(EopperandType::value, value))) return false;
			} else {
				const TCHAR* numberStart = *curParam + 1;
				const int64 parsedAddress = FCString::Strtoi64(numberStart, &end, 10);

				if (end == numberStart || *end != '\0' || parsedAddress < MIN_int32 || parsedAddress > MAX_int32) {
					errorEvent.Broadcast(FCompileError(FText::FormatNamed(notAnAddrOrValueAddrFormat, TEXT("base"), FText::FromString(base), TEXT("paramNum"), counter + 1, TEXT("got"), FText::FromString(curParam)), lineNumber));
					UE_LOG(LogVM, Warning, TEXT("Could not parse a valid int32 for the address ID in address or value parameter! Line: %s"), *line);
					return false;
				}

				if (parsedAddress < 0) {
					errorEvent.Broadcast(FCompileError(FText::FormatNamed(addrNegFormat, TEXT("paramNum"), counter + 1), lineNumber));
					return false;
				}

				const uint32 address = static_cast<uint32>(parsedAddress);
				if (firstChar == 'R') {
					if (address > maxReg) {
						errorEvent.Broadcast(FCompileError(FText::FormatNamed(regOutOfBoundsFormat, TEXT("paramNum"), counter + 1, TEXT("got"), address, TEXT("max"), maxReg), lineNumber));
						return false;
					}
				}
				else {
					if (address > maxPort) {
						errorEvent.Broadcast(FCompileError(FText::FormatNamed(portOutOfBoundsFormat, TEXT("paramNum"), counter + 1, TEXT("got"), address, TEXT("max"), maxPort), lineNumber));
						return false;
					}
				}

				if (!compiledInstruction.addParam(FopperandValue(firstChar, address))) return false;
			}

			break;
		}

		default: {
			UE_LOG(LogVM, Error, TEXT("Unhandled EparameterType in verifyLine!"));
			return false;
		}
		}

		counter++;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

bool UVM::executeInstruction(FcompiledInstruction& instruction)
{
	switch (instruction.opCode)
	{
	case EopCode::add:
		writeValue(instruction.op3, readOperand(instruction.op1) + readOperand(instruction.op2));
		break;
	case EopCode::mov:
		writeValue(instruction.op2, readOperand(instruction.op1));
		break;
	default:
		UE_LOG(LogVM, Error, TEXT("Unhandled command!"));
		return false;
	}
	return true;
}

void UVM::registerInstruction(FinstructionDefinition newInstruction) {
	FinstructionDefinition* validDef = instructionSet.Find(newInstruction.base);
	if (validDef) {
		UE_LOG(LogVM, Error, TEXT("Could not add \'%s\' to instruction set because base \'%s\' is already there!"), *newInstruction.toString(), *validDef->toString());
	}
	else {
		instructionSet.Add(newInstruction.base, newInstruction);
	}
}

void UVM::raiseInterrupt(const FString& debugMessage) {
	UE_LOG(LogVM, Warning, TEXT("%s"), *debugMessage);
	interrupt = true;
}

bool UVM::compileProgram(FString program, TArray<FcompiledInstruction>& instructions)
{
	TArray<FString> lines;
	instructions.Reset();
	instructions.SetNum(program.ParseIntoArrayLines(lines));
	uint32 counter = 0;
	for (FString line : lines) {
		if (!verifyLine(line, instructions[counter], counter)) return false;
		counter++;
	}
	return true;
}

TArray<int32> UVM::getPort(uint8 port)
{
	FPort* validPort = ports.Find(port);
	if (validPort) return validPort->myData;
	else UE_LOG(LogVM, Error, TEXT("Could not find port %d"), port);
	return TArray<int32>();
}

FPort UVM::getPortFull(uint8 port)
{
	FPort* validPort = ports.Find(port);
	if (validPort) return *validPort;
	else UE_LOG(LogVM, Error, TEXT("Could not find port %d"), port);
	return FPort();
}

int32 UVM::getRegister(int32 reg)
{
	return readRegister(reg);
}

bool UVM::runProgram(FString program)
{
	if (!compileProgram(program, curProgram)) return false;
	runningProgram = true;
	ranWithoutErrors = true;
	pc = 0;
	unPauseProgram();
	return true;
}

void UVM::unPauseProgram() {
	interrupt = false;
	GetWorld()->GetTimerManager().SetTimer(stepTimer, this, &UVM::programRunner, runSpeed);
}

void UVM::programRunner() {
	if (!interrupt) {
		stepProgram();
		if (curProgram.IsValidIndex(pc)) {
			GetWorld()->GetTimerManager().SetTimer(stepTimer, this, &UVM::programRunner, runSpeed);
		}
		else {
			stopProgram();
		}
	}
}

bool UVM::stepProgram() {
	if (!runningProgram) return false;
	if (!curProgram.IsValidIndex(pc)) {
		stopProgram();
		return false;
	}
	while (curProgram[pc].opCode == EopCode::empty) pc++;
	if (!executeInstruction(curProgram[pc++])) {
		ranWithoutErrors = false;
		return false;
	}
	stepCount++;
	stateChanged.Broadcast();
	return true;
}

void UVM::pauseProgram() {
	interrupt = true;
	stepTimer.Invalidate();
}

void UVM::stopProgram() {
	if (isStopped()) return;
	interrupt = true;
	runningProgram = false;
	stepTimer.Invalidate();
	pc = 0;
	FProgramResults results;
	results.ports = ports;
	results.ranWithoutErrors = ranWithoutErrors;
	results.stepsTaken = stepCount;
	stepCount = 0;
	programEnded.Broadcast(results);
}

const bool UVM::isPaused() {
	return !stepTimer.IsValid() && !isStopped();
}

const bool UVM::isStopped() {
	return !runningProgram;
}

const int UVM::getStepCount()
{
	return stepCount;
}

const int UVM::getPC()
{
	return pc;
}

void UVM::getAllCommandNames(TArray<FString>& commandNames, bool filterLocked)
{
	if (!filterLocked) instructionSet.GetKeys(commandNames);
	else {
		for (const auto& pair : instructionSet) {
			const FString& key = pair.Key;
			const FinstructionDefinition& value = pair.Value;
			if (value.isUnlocked) commandNames.Add(key);
		}
	}
}

void UVM::getAllCommands(TArray<FinstructionDefinition>& commandDatas, bool filterLocked)
{
	if (!filterLocked) instructionSet.GenerateValueArray(commandDatas);
	else {
		for (const auto& pair : instructionSet) {
			const FString& key = pair.Key;
			const FinstructionDefinition& value = pair.Value;
			if (value.isUnlocked) commandDatas.Add(value);
		}
	}
}

FinstructionDefinition UVM::getCommand(FString name)
{
	return instructionSet[name];
}

void UVM::resetMachine(int32 _maxReg, int32 _maxPort, TArray<FportDataLoader> preLoadedPorts)
{
	maxReg = _maxReg;
	maxPort = _maxPort;
	registers = TArray<int32>();
	registers.SetNumZeroed(maxReg + 1);
	ports.Reset();
	for (uint8 i = 0; i < _maxPort + 1; i++) ports.Add(i, FPort());
	for (const FportDataLoader& data : preLoadedPorts) ports[data.portNumber] = data.data;
	interrupt = false;
	pc = 0;
	stepCount = 0;
	ranWithoutErrors = true;
	runningProgram = false;
	stateChanged.Broadcast();
}

void UVM::testRunProgram() {
	TArray<FportDataLoader> preLoadedPorts;
	preLoadedPorts.Add(FportDataLoader(0, FPort({2,2}, (uint8)EReadWriteEnable::readWrite)));
	resetMachine(0, 1, preLoadedPorts);
	runProgram("ADD P0 P0 P1");
}

#undef LOCTEXT_NAMESPACE