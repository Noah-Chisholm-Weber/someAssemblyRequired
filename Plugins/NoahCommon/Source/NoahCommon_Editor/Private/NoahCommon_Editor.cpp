// Copyright Epic Games, Inc. All Rights Reserved.

#include "NoahCommon_Editor.h"

#define LOCTEXT_NAMESPACE "FNoahCommon_EditorModule"

void FNoahCommon_EditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FNoahCommon_EditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNoahCommon_EditorModule, NoahCommon_Editor)