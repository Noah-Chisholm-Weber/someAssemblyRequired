// Fill out your copyright notice in the Description page of Project Settings.


#include "VM.h"

DEFINE_LOG_CATEGORY(LogVM)

FRegexPattern UVM::lineCheckerPattern = FRegexPattern(R"(^([A-Z]+)(( +[RP]?\d+)+$))");
TMap<FString, FinstructionDefinition> UVM::instructionSet = TMap<FString, FinstructionDefinition>();

UVM::UVM() {
	registerInstruction(FinstructionDefinition(TEXT("MOV"), {EparameterType::addrOrValue, EparameterType::addrOrValue}, EopCode::mov));
	registerInstruction(FinstructionDefinition(TEXT("ADD"), { EparameterType::addrOrValue, EparameterType::addrOrValue, EparameterType::addrOrValue }, EopCode::add));
}

int UVM::readRegister(uint32 reg) {
	if (registers.IsValidIndex(reg)) return registers[reg];
	else raiseInterrupt(FString::Printf(TEXT("%d is not a valid register! This machine only has registers 0-%d"), reg, maxReg ));
	return -1;
}

int UVM::readPort(uint8 port) {
	TArray<int>* validPort = ports.Find(port);
	if (validPort) return validPort->Pop();
	else raiseInterrupt(FString::Printf(TEXT("%d is not a valid port! This machine only supports ports 0-%d"), port, maxPort));
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
	} else raiseInterrupt(FString::Printf(TEXT("%d is not a valid register! This machine only has registers 0-%d"), reg, maxReg));
}

void UVM::writePort(uint8 port, int32 value) {
	TArray<int>* validPort = ports.Find(port);
	if (validPort) {
		validPort->Push(value);
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

bool UVM::verifyLine(FString line, FcompiledInstruction& compiledInstruction) {
	FRegexMatcher matcher(lineCheckerPattern, line);
	if (!matcher.FindNext()) return false;
	FString base = matcher.GetCaptureGroup(1);
	FinstructionDefinition* validDef = instructionSet.Find(base);
	if (!validDef) return false;
	compiledInstruction.opCode = validDef->opCode;
	FString paramString = matcher.GetCaptureGroup(2);
	TArray<FString> params;
	paramString.ParseIntoArray(params, TEXT(" "));
	size_t counter = 0;
	FString curParam;
	TCHAR* end;
	char firstChar;
	if (params.Num() != validDef->params.Num()) {
		if (params.Num() < validDef->params.Num()) {
			UE_LOG(LogVM, Error, TEXT("There were too few params for the %s command when verifing a %s! Expected: %s"), *base, *line, *validDef->toString());
		}
		else {
			UE_LOG(LogVM, Error, TEXT("There were too many params for the %s command when verifing a %s! Expected: %s"), *base, *line, *validDef->toString());
		}
		return false;
	}
	for (const EparameterType& type : validDef->params) {
		curParam = params[counter];
		switch (type)
		{
		case EparameterType::addr:
			firstChar = curParam[0];
			if (firstChar != 'R' && firstChar != 'P') {
				UE_LOG(LogVM, Warning, TEXT("Could not find 'R' or 'P' when parsing an address only field! Line: %s"), *line);
				return false;
			}
			switch (counter) {
				case 0: 
					compiledInstruction.op1 = FopperandValue(firstChar == 'R' ? EopperandType::reg : EopperandType::port, FCString::Strtoi(*curParam + 1, &end, 10));
					break;
				case 1:
					compiledInstruction.op2 = FopperandValue(firstChar == 'R' ? EopperandType::reg : EopperandType::port, FCString::Strtoi(*curParam + 1, &end, 10));
					break;
				case 2:
					compiledInstruction.op3 = FopperandValue(firstChar == 'R' ? EopperandType::reg : EopperandType::port, FCString::Strtoi(*curParam + 1, &end, 10));
					break;
				default:
					UE_LOG(LogVM, Error, TEXT("Too many operands for %s!"), *base);
					return false;
			}
			if (*end != '\0') {
				UE_LOG(LogVM, Warning, TEXT("Could not parse an int for the register id after address type identifier for address only parameter! Line: %s"), *line);
				return false;
			}
			break;
		case EparameterType::value:
			switch (counter) {
			case 0:
				compiledInstruction.op1 = FopperandValue(EopperandType::value, FCString::Strtoi(*curParam, &end, 10));
				break;
			case 1:
				compiledInstruction.op2 = FopperandValue(EopperandType::value, FCString::Strtoi(*curParam, &end, 10));
				break;
			case 2:
				compiledInstruction.op3 = FopperandValue(EopperandType::value, FCString::Strtoi(*curParam, &end, 10));
				break;
			default:
				UE_LOG(LogVM, Error, TEXT("Too many operands for %s!"), *base);
				return false;
			}
			if (*end != '\0') {
				UE_LOG(LogVM, Warning, TEXT("Could not parse an int for value only parameter! Line: %s"), *line);
				return false;
			}
			break;
		case EparameterType::label:
			UE_LOG(LogVM, Error, TEXT("Label not implemlented yet!"));
			return false;
		case EparameterType::addrOrValue:
			firstChar = curParam[0];
			if (firstChar != 'R' && firstChar != 'P') {
				switch (counter) {
				case 0:
					compiledInstruction.op1 = FopperandValue(EopperandType::value, FCString::Strtoi(*curParam, &end, 10));
					break;
				case 1:
					compiledInstruction.op2 = FopperandValue(EopperandType::value, FCString::Strtoi(*curParam, &end, 10));
					break;
				case 2:
					compiledInstruction.op3 = FopperandValue(EopperandType::value, FCString::Strtoi(*curParam, &end, 10));
					break;
				default:
					UE_LOG(LogVM, Error, TEXT("Too many operands for %s!"), *base);
					return false;
				}
				if (*end != '\0') {
					UE_LOG(LogVM, Warning, TEXT("Could not parse an int for address or value parameter when missing address type identifier! Line: %s"), *line);
					return false;
				}
			} else switch (counter) {
			case 0:
				compiledInstruction.op1 = FopperandValue(firstChar == 'R' ? EopperandType::reg : EopperandType::port, FCString::Strtoi(*curParam + 1, &end, 10));
				break;
			case 1:
				compiledInstruction.op2 = FopperandValue(firstChar == 'R' ? EopperandType::reg : EopperandType::port, FCString::Strtoi(*curParam + 1, &end, 10));
				break;
			case 2:
				compiledInstruction.op3 = FopperandValue(firstChar == 'R' ? EopperandType::reg : EopperandType::port, FCString::Strtoi(*curParam + 1, &end, 10));
				break;
			default:
				UE_LOG(LogVM, Error, TEXT("Too many operands for %s!"), *base);
				return false;
			}
			if (*end != '\0') {
				UE_LOG(LogVM, Warning, TEXT("Could not parse an int for the register id after address type identifier for address or value parameter! Line: %s"), *line);
				return false;
			}
			break;
		default:
			UE_LOG(LogVM, Error, TEXT("Unhandled EparameterType in verifyLine!"));
			break;
		}
		counter++;
	}
	return true;
}

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
	instructions.SetNumZeroed(program.ParseIntoArrayLines(lines));
	uint32 counter = 0;
	for (FString line : lines) {
		if (!verifyLine(line, instructions[counter++])) return false;
	}
	return true;
}

TArray<int32> UVM::getPort(uint8 port)
{
	TArray<int32>* validPort = ports.Find(port);
	if (validPort) return *validPort;
	else UE_LOG(LogVM, Error, TEXT("Could not find port %d"), port);
	return TArray<int32>();
}

int32 UVM::getRegister(int32 reg)
{
	return readRegister(reg);
}

bool UVM::runProgram(FString program)
{
	TArray<FcompiledInstruction> instructions;
	if (!compileProgram(program, instructions)) return false;
	while (instructions.IsValidIndex(pc)) {
		if (interrupt) {
			return false;
		}
		else {
			if (!executeInstruction(instructions[pc])) return false;
		}
		pc++;
	}
	return true;
}

void UVM::resetMachine(int32 _maxReg, int32 _maxPort, TArray<FportDataLoader> preLoadedPorts)
{
	maxReg = _maxReg;
	maxPort = _maxPort;
	registers = TArray<int32>();
	registers.SetNumZeroed(maxReg + 1);
	ports.Reset();
	for (uint8 i = 0; i < _maxPort + 1; i++) ports.Add(i, TArray<int32>());
	for (const FportDataLoader& data : preLoadedPorts) ports[data.portNumber] = data.data;
	interrupt = false;
	pc = 0;
}

void UVM::testRunProgram() {
	TArray<FportDataLoader> preLoadedPorts;
	preLoadedPorts.Add(FportDataLoader(0, {2,2}));
	resetMachine(0, 1, preLoadedPorts);
	runProgram("ADD P0 P0 P1");
}
