#pragma once
// Thread
#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "Misc/SingleThreadRunnable.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "Async/Async.h"

// UE Events
#include "UnrealClient.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameViewportDelegates.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "ShaderPipelineCache.h"
#include "ShaderCodeArchive.h"

// UE Others
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformApplicationMisc.h"
#include "UObject/ConstructorHelpers.h"

// ImGui
#include "ImGui/imgui.h"

// Stats
#include "DFoundryFX_Module.h"
#include "DFoundryFX_StatData.h"

class DFOUNDRYFX_API FDFX_Thread : public FRunnable, FSingleThreadRunnable {
public:  //Thread
  FDFX_Thread();
  virtual ~FDFX_Thread();

  // FRunnable interface
  virtual bool Init() override;
  virtual uint32 Run() override;
  virtual void Stop() override;
  virtual void Exit() override;

  // FSingleThreadRunnable interface
  virtual FSingleThreadRunnable* GetSingleThreadInterface() override { return this; }
  virtual void Tick() override;

  // Thread control
  void Wait(float Seconds);
  void SetPaused(bool bMakePaused);
  bool IsPaused() const;
  bool IsVerifiedSuspended() const;
  bool HasStopped() const;
  bool IsThreadPaused() const;
  bool IsThreadVerifiedSuspended() const;
  bool HasThreadStopped() const;

  // ImGui
  bool ImGui_ImplUE_Init();
  bool ImGui_ImplUE_CreateDeviceObjects();
  void ImGui_ImplUE_ProcessEvent();
  void ImGui_ImplUE_NewFrame();
  void ImGui_ImplUE_Render();
  void ImGui_ImplUE_RenderDrawLists();
  static const char* ImGui_ImplUE_GetClipboardText(void* user_data);
  static void ImGui_ImplUE_SetClipboardText(void* user_data, const char* text);

  
protected:
  FRunnableThread* DFoundryFX_Thread = nullptr;

  bool bStopping = false;
  static FThreadSafeBool bThreadPaused;
  static FThreadSafeBool bIsVerifiedSuspended;
  static FThreadSafeBool bHasStopped;

  // UE context
  TWeakObjectPtr<AGameModeBase> GameMode;
  TWeakObjectPtr<UGameViewportClient> GameViewport;
  TWeakObjectPtr<UWorld> uWorld;
  TWeakObjectPtr<UCanvas> uCanvas;
  TWeakObjectPtr<APlayerController> PlayerController;
  FVector2D ViewportSize;

  // Delegates
  FDelegateHandle hOnGameModeInitialized;
  void OnGameModeInitialized(AGameModeBase* GameMode);

  FDelegateHandle hOnWorldBeginPlay;
  void OnWorldBeginPlay();

  FDelegateHandle hOnViewportCreated;
  void OnViewportCreated();

  FDelegateHandle hOnViewportResized;
  void OnViewportResized(FViewport* Viewport, uint32 /*Unused*/);
  
  FDelegateHandle hOnHUDPostRender;
  void OnHUDPostRender(AHUD* Sender, UCanvas* Canvas);

  FDelegateHandle hOnViewportClose;
  bool OnViewportClose();

  FDelegateHandle hOnPipelineStateLogged;
  void OnPipelineStateLogged(const FPipelineCacheFileFormatPSO& PipelineCacheFileFormatPSO);
  double ShaderLogTime = 0;

private:
  // ImGui state
  FORCEINLINE ImGuiIO& GetImGuiIO() const;
  ImGuiContext* m_ImGuiContext = nullptr;
  ImPlotContext* m_ImPlotContext = nullptr;
  static ImGuiKey FKeyToImGuiKey(FName Keyname);
  uint64 m_ImGuiDiffTime;

  // Events
  bool HandleControllerInput();
  void RemoveDelegates();
};