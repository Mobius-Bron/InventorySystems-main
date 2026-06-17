// Copyright AmberAeolian. All Rights Reserved.

#include "DebugSystem.h"

#define LOCTEXT_NAMESPACE "FDebugSystemModule"

void FDebugSystemModule::StartupModule()
{
	// CVars are registered statically in DS_DebugFunctionLibrary.cpp via TAutoConsoleVariable
	// No additional initialization required
}

void FDebugSystemModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDebugSystemModule, DebugSystem)