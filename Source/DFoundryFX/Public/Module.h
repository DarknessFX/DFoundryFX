#pragma once

#include "Modules/ModuleManager.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Thread.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDFoundryFX, Log, All);
DECLARE_STATS_GROUP(TEXT("DFoundryFX"), STATGROUP_DFoundryFX, STATCAT_Advanced);

//MODULE
class DFOUNDRYFX_API FDFX_Module : public IModuleInterface
{
public:
  static inline FDFX_Module& Get()
  {
    return FModuleManager::LoadModuleChecked<FDFX_Module>("DFoundryFX");
  }
  static inline bool IsAvailable()
  {
    return FModuleManager::Get().IsModuleLoaded("DFoundryFX");
  }

  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
  virtual bool IsGameModule() const override { return true; }

  static inline UMaterialInterface* MasterMaterial = nullptr;
  static inline UMaterialInstanceDynamic* MaterialInstance = nullptr;
  static inline UTexture2D* FontTexture = nullptr;
  static inline bool FontTexture_Updated = false;
};