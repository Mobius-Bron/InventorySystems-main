#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** 模块日志分类 */
DECLARE_LOG_CATEGORY_EXTERN(LogMIS, Log, All);

/**
 * Multiplayer Inventory System 插件模块
 * 库存系统的主模块,负责所有 MIS_ 前缀类的注册
 */
class FMultiplayerInventoryModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
