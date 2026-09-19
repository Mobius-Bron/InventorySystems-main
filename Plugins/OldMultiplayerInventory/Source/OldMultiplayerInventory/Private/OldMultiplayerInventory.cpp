#include "OldMultiplayerInventory.h"

#define LOCTEXT_NAMESPACE "FOldMultiplayerInventoryModule"

DEFINE_LOG_CATEGORY(LogOldMIS);

void FOldMultiplayerInventoryModule::StartupModule()
{
}

void FOldMultiplayerInventoryModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOldMultiplayerInventoryModule, OldMultiplayerInventory)
