#include "MultiplayerInventory.h"

#define LOCTEXT_NAMESPACE "FMultiplayerInventoryModule"

DEFINE_LOG_CATEGORY(LogMIS);

void FMultiplayerInventoryModule::StartupModule()
{
}

void FMultiplayerInventoryModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMultiplayerInventoryModule, MultiplayerInventory)
