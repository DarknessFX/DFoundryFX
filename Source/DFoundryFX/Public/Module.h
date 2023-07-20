#pragma once

#include "Thread.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDFoundryFX, Log, All);

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

private:
  TSharedPtr<FDFX_Thread> DFXThread;
};