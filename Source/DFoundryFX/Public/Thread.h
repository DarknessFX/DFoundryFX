#pragma once
// Thread
#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "Misc/SingleThreadRunnable.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "Async/Async.h"

// Events -> GameMode and Viewport
#include "UnrealClient.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameViewportDelegates.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "ShaderPipelineCache.h"
#include "ShaderCodeArchive.h"
// External window
//#include "Framework/Application/SlateApplication.h"
//#include "Widgets/SWindow.h"

// ImGui
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformApplicationMisc.h"
#include "UObject/ConstructorHelpers.h"
#include "ImGui/imgui.h"

// Stats
#include "Module.h"
#include "StatData.h"

class DFOUNDRYFX_API FDFX_Thread : public FRunnable, FSingleThreadRunnable
{
public:  //Thread
  FDFX_Thread();
  virtual ~FDFX_Thread();

  void Wait(float Seconds);
  virtual FSingleThreadRunnable* GetSingleThreadInterface() override { return this; }
  virtual void Tick() override;

  virtual bool Init() override;
  virtual uint32 Run() override;
  virtual void Stop() override;
  virtual void Exit() override;

  void SetPaused(bool MakePaused);
  bool IsThreadPaused();
  bool IsThreadVerifiedSuspended();
  bool HasThreadStopped();

protected:
  bool bStopping = false;
  FThreadSafeBool bPaused;
  FThreadSafeBool bIsVerifiedSuspended;
  FThreadSafeBool bHasStopped;
  FRunnableThread* DFoundryFX_Thread = nullptr;


public:  // Events -> GameMode and Viewport
  AGameModeBase* GameMode;
  UGameViewportClient* GameViewport;
  FVector2D ViewportSize;
  UWorld* uWorld;
  UCanvas* uCanvas;
  APlayerController* PlayerController;

  FDelegateHandle hOnGameModeInitialized;
  void OnGameModeInitialized(AGameModeBase* GameMode);

  FDelegateHandle hOnWorldBeginPlay;
  void OnWorldBeginPlay();

  FDelegateHandle hOnViewportCreated;
  void OnViewportCreated();

  FDelegateHandle hOnHUDPostRender;
  void OnHUDPostRender(AHUD* Sender, UCanvas* Canvas);

  FDelegateHandle hOnViewportClose;
  bool OnViewportClose();

  FDelegateHandle hOnPipelineStateLogged;
  void OnPipelineStateLogged(FPipelineCacheFileFormatPSO& PipelineCacheFileFormatPSO);
  double ShaderLogTime = 0;

public:  // ImGui
  bool ImGui_ImplUE_Init();
  bool ImGui_ImplUE_CreateDeviceObjects();
  void ImGui_ImplUE_ProcessEvent();
  void ImGui_ImplUE_NewFrame();
  void ImGui_ImplUE_Render();
  void ImGui_ImplUE_RenderDrawLists();

  static const char* ImGui_ImplUE_GetClipboardText(void* user_data);
  static void ImGui_ImplUE_SetClipboardText(void* user_data, const char* text);

private:
  FORCEINLINE ImGuiIO& GetImGuiIO() const;
  ImGuiContext* m_ImGuiContext = nullptr;
  ImPlotContext* m_ImPlotContext = nullptr;

  uint64 m_ImGuiDiffTime;

  static ImGuiKey FKeyToImGuiKey(FName Keyname);

  bool ControllerInput();

  void RemoveDelegates();

  void ExternalWindow(bool IsExiting = false);
  TSharedPtr<SWindow> m_extWindow = nullptr;
  bool bExternalOpened = false;

};