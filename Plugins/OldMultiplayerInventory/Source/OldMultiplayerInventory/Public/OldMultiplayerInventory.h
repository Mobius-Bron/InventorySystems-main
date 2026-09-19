#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** 模块日志分类 */
DECLARE_LOG_CATEGORY_EXTERN(LogOldMIS, Log, All);

/**
 * Multiplayer Inventory System 插件模块
 * 库存系统的主模块,负责所有 OldMIS_ 前缀类的注册
 */
class FOldMultiplayerInventoryModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
