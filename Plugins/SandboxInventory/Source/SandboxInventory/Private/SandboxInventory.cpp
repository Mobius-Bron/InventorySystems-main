// Copyright Epic Games, Inc. All Rights Reserved.

#include "SandboxInventory.h"

#define LOCTEXT_NAMESPACE "FSandboxInventoryModule"

void FSandboxInventoryModule::StartupModule()
{
	// This code will execute after your module is loaded into memory
}

void FSandboxInventoryModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSandboxInventoryModule, SandboxInventory)