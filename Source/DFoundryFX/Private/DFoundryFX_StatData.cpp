#include "DFoundryFX_StatData.h"
#include "DFoundryFX_Module.h"
#include <type_traits>

DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatLoadDefault"), STAT_StatLoadDefault, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatUpdate"), STAT_StatUpdate, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatMainWin"), STAT_StatMainWin, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatPlotThread"), STAT_StatPlotThread, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatPlotFrame"), STAT_StatPlotFrame, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatPlotFPS"), STAT_StatPlotFPS, STATGROUP_DFoundryFX);

UGameViewportClient* FDFX_StatData::ViewportClient = nullptr;
FVector2D FDFX_StatData::ViewportSize = FVector2D::ZeroVector;

bool FDFX_StatData::bMainWindowExpanded = true;
bool FDFX_StatData::bShowPlots = true;
bool FDFX_StatData::bSortPlots = true;
bool FDFX_StatData::bShowDebugTab = true;

float FDFX_StatData::GlobalStatHistoryDuration = 3.0f;

FDFX_StatData::FPlotConfig FDFX_StatData::ThreadPlotConfig;
FDFX_StatData::FPlotConfig FDFX_StatData::FramePlotConfig;
FDFX_StatData::FPlotConfig FDFX_StatData::FPSPlotConfig;

FDFX_StatData::FThreadPlotStyle FDFX_StatData::ThreadPlotStyles[7];

double FDFX_StatData::CurrentTime = 0.0;
double FDFX_StatData::LastTime = 0.0;
double FDFX_StatData::DeltaTime = 0.0;
int32 FDFX_StatData::FrameCount = 0;
float FDFX_StatData::FrameTime = 0.0f;
int32 FDFX_StatData::FPS = 0;
float FDFX_StatData::GameThreadTime = 0.0f;
float FDFX_StatData::RenderThreadTime = 0.0f;
float FDFX_StatData::GPUFrameTime = 0.0f;
float FDFX_StatData::RHIThreadTime = 0.0f;
float FDFX_StatData::SwapBufferTime = 0.0f;
float FDFX_StatData::InputLatencyTime = 0.0f;
float FDFX_StatData::ImGuiThreadTime = 0.0f;

double FDFX_StatData::ImPlotFrameCount = 0.0;
FDFX_StatData::FHistoryBuffer FDFX_StatData::TimeHistory;
FDFX_StatData::FHistoryBuffer FDFX_StatData::FrameCountHistory;
FDFX_StatData::FHistoryBuffer FDFX_StatData::FrameTimeHistory;
FDFX_StatData::FHistoryBuffer FDFX_StatData::FPSHistory;
FDFX_StatData::FHistoryBuffer FDFX_StatData::GameThreadTimeHistory;
FDFX_StatData::FHistoryBuffer FDFX_StatData::RenderThreadTimeHistory;
FDFX_StatData::FHistoryBuffer FDFX_StatData::GPUFrameTimeHistory;
FDFX_StatData::FHistoryBuffer FDFX_StatData::RHIThreadTimeHistory;
FDFX_StatData::FHistoryBuffer FDFX_StatData::SwapBufferTimeHistory;
FDFX_StatData::FHistoryBuffer FDFX_StatData::InputLatencyTimeHistory;
FDFX_StatData::FHistoryBuffer FDFX_StatData::ImGuiThreadTimeHistory;

TArray<FDFX_StatData::FShaderCompileLog> FDFX_StatData::ShaderCompileLogs;

TArray<FDFX_StatData::FContextInfoEntry> FDFX_StatData::Engine_MemoryEntries;
TArray<FDFX_StatData::FContextInfoEntry> FDFX_StatData::Engine_ViewportEntries;
TArray<FDFX_StatData::FContextInfoEntry> FDFX_StatData::Engine_GEngineEntries;
TArray<FDFX_StatData::FContextInfoEntry> FDFX_StatData::Engine_RHIEntries;
TArray<FDFX_StatData::FContextInfoEntry> FDFX_StatData::Engine_PlatformEntries;

void FDFX_StatData::RunDFoundryFX(uint64 ImGuiThreadTimeMs) {
  //ViewportClient->GetViewportSize(ViewportSize);
  ImGuiThreadTime = 0.9f * ImGuiThreadTime + 0.1f * (static_cast<float>(ImGuiThreadTimeMs) * FPlatformTime::GetSecondsPerCycle64());

  {
    SCOPE_CYCLE_COUNTER(STAT_StatUpdate);
    UpdateStats();
  }
  {
    SCOPE_CYCLE_COUNTER(STAT_StatMainWin);
    RenderMainWindow();
  }

  if (bShowPlots) {
    if (ThreadPlotConfig.bVisible) {
      {
        SCOPE_CYCLE_COUNTER(STAT_StatPlotThread);
        LoadThreadPlotConfig();
      }
    }
    if (FramePlotConfig.bVisible) {
      {
        SCOPE_CYCLE_COUNTER(STAT_StatPlotFrame);
        LoadFramePlotConfig();
      }
    }
    if (FPSPlotConfig.bVisible) {
      {
        SCOPE_CYCLE_COUNTER(STAT_StatPlotFPS);
        LoadFPSPlotConfig();
      }
    }
  }
}

void FDFX_StatData::RenderMainWindow() {
  char WindowTitle[] = "DFoundryFX";
  ImGui::Begin(WindowTitle, NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
  if (ImGui::IsWindowCollapsed())  {
    bMainWindowExpanded = false;
    const int Offset = 64;
    const int txtSizeX = ImGui::CalcTextSize(WindowTitle).x;
    ImGui::SetWindowPos(ImVec2(ViewportSize.X - txtSizeX - Offset, 0));
    ImGui::SetWindowSize(ImVec2(txtSizeX + Offset, 0));

    const ImRect titleBarRect = ImGui::GetCurrentWindow()->TitleBarRect();
    ImGui::PushClipRect(titleBarRect.Min, titleBarRect.Max, false);
    ImGui::SetCursorPos(ImVec2(txtSizeX + Offset / 2, 0.0f));
    ImGui::Text(" "); ImGui::SameLine();
    ImGui::Checkbox("##DisplayGraphs", &bShowPlots);
    ImGui::PopClipRect();
  } else {
    bMainWindowExpanded = true;
    ImGui::SetWindowPos(ImVec2(ViewportSize.X - (ViewportSize.X / 4), 0));
    ImGui::SetWindowSize(ImVec2(ViewportSize.X / 4, ViewportSize.Y));
    ImGui::BeginTabBar("##MainWindowTabBar");
    if (ImGui::BeginTabItem("Engine")) {
      RenderEngineTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Shaders")) {
      RenderShadersTab();
      ImGui::EndTabItem();
    }
  #if WITH_EDITOR
    if (ImGui::BeginTabItem("STAT")) {
      RenderStatTab();
      ImGui::EndTabItem();
    }
  #endif
    if (ImGui::BeginTabItem("Settings")) {
      RenderSettingsTab();
      ImGui::EndTabItem();
    }
    if (bShowDebugTab) {
      if (ImGui::BeginTabItem("Debug")) {
        RenderDebugTab();
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();    
  }
  ImGui::End();
}

void FDFX_StatData::UpdateStats() {
  if (FApp::IsBenchmarking() || FApp::UseFixedTimeStep()) {
    CurrentTime = FPlatformTime::Seconds();
    if (LastTime == 0.0) {
      LastTime = CurrentTime;
    }
    DeltaTime = CurrentTime - LastTime;
    LastTime = CurrentTime;
  } else {
    CurrentTime = FApp::GetCurrentTime();
    LastTime = FApp::GetLastTime();
    DeltaTime = CurrentTime - LastTime;
  }

  FrameCount = GFrameCounter;
  FrameTime = 0.9f * FrameTime + 0.1f * (static_cast<float>(DeltaTime * 1000.0));
  FPS = static_cast<int32>(FMath::RoundToInt(1000.0f / FrameTime));
  GameThreadTime = 0.9f * GameThreadTime + 0.1f * FPlatformTime::ToMilliseconds(GGameThreadTime);
  RenderThreadTime = 0.9f * RenderThreadTime + 0.1f * FPlatformTime::ToMilliseconds(GRenderThreadTime);
  GPUFrameTime = 0.9f * GPUFrameTime + 0.1f * FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
  RHIThreadTime = 0.9f * RHIThreadTime + 0.1f * FPlatformTime::ToMilliseconds(GRHIThreadTime); //  GWorkingRHIThreadTime, GRHIThreadTime display some crazy values on Shipping builds, changed to GWorkingRHIThreadTime.
  SwapBufferTime = 0.9f * SwapBufferTime + 0.1f * FPlatformTime::ToMilliseconds(GSwapBufferTime);
  InputLatencyTime = 0.9f * InputLatencyTime + 0.1f * FPlatformTime::ToMilliseconds(GInputLatencyTimer.DeltaTime);

  // Save data for ImPlot
  TimeHistory.Add(CurrentTime);
  FrameCountHistory.Add(ImPlotFrameCount);
  FrameTimeHistory.Add(FrameTime);
  FPSHistory.Add(static_cast<double>(FPS));
  GameThreadTimeHistory.Add(GameThreadTime);
  RenderThreadTimeHistory.Add(RenderThreadTime);
  GPUFrameTimeHistory.Add(GPUFrameTime);
  RHIThreadTimeHistory.Add(RHIThreadTime);
  SwapBufferTimeHistory.Add(SwapBufferTime);
  InputLatencyTimeHistory.Add(InputLatencyTime);
  ImGuiThreadTimeHistory.Add(ImGuiThreadTime);

  ImPlotFrameCount++;
}

void FDFX_StatData::RenderEngineTab() {
  UWorld* World = ViewportClient->GetWorld();
  IConsoleManager& ConsoleManager = IConsoleManager::Get();

  if (ImGui::CollapsingHeader("Viewport Settings")) {
    static int32 ViewportSizeArray[2] = { static_cast<int32>(ViewportSize.X), static_cast<int32>(ViewportSize.Y) };
    static int32 MaxFPS = ConsoleManager.FindConsoleVariable(TEXT("t.MaxFPS"))->GetInt();
    static bool bVSync = ConsoleManager.FindConsoleVariable(TEXT("r.VSync"))->GetBool();
    static int32 ScreenPercentage = ConsoleManager.FindConsoleVariable(TEXT("r.ScreenPercentage"))->GetInt();
    static int32 ResolutionQuality = ConsoleManager.FindConsoleVariable(TEXT("sg.ResolutionQuality"))->GetInt();
    static int32 ViewDistanceScale = ConsoleManager.FindConsoleVariable(TEXT("r.ViewDistanceScale"))->GetInt();
    static int32 PostProcessQuality = ConsoleManager.FindConsoleVariable(TEXT("sg.PostProcessQuality"))->GetInt();
    static int32 ShadowQuality = ConsoleManager.FindConsoleVariable(TEXT("sg.ShadowQuality"))->GetInt();
    static int32 TextureQuality = ConsoleManager.FindConsoleVariable(TEXT("sg.TextureQuality"))->GetInt();
    static int32 EffectsQuality = ConsoleManager.FindConsoleVariable(TEXT("sg.EffectsQuality"))->GetInt();
    static int32 DetailMode = ConsoleManager.FindConsoleVariable(TEXT("r.DetailMode"))->GetInt();
    static int32 SkeletalMeshLODBias = ConsoleManager.FindConsoleVariable(TEXT("r.SkeletalMeshLODBias"))->GetInt();
    static bool bFullscreen = ViewportClient->Viewport->IsFullscreen();
    static bool bFullscreenExclusive = (ConsoleManager.FindConsoleVariable(TEXT("r.FullscreenMode"))->GetInt() == 1 ? false : true);

    ImGui::BeginTable("##tblViewSettingsBase", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextColumn();
    ImGui::InputInt2("Size", ViewportSizeArray);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS1")) {
      if (bFullscreen) {
        ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%if"), ViewportSizeArray[0], ViewportSizeArray[1]));
      } else {
        ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%iw"), ViewportSizeArray[0], ViewportSizeArray[1]));
      }
    }

    ImGui::TableNextColumn();
    if (ImGui::Checkbox("Fullscreen", &bFullscreen)) {
      if (bFullscreen != ViewportClient->Viewport->IsFullscreen()) {
        if (bFullscreen) {
          ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%if"), ViewportSizeArray[0], ViewportSizeArray[1]));
        } else {
          ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%iw"), ViewportSizeArray[0], ViewportSizeArray[1]));
        }
      }
    }
    ImGui::TableNextColumn();

    ImGui::TableNextColumn();
    if (ImGui::Checkbox("FullscreenExclusive", &bFullscreenExclusive)) {
      if (bFullscreenExclusive != (ConsoleManager.FindConsoleVariable(TEXT("r.FullscreenMode"))->GetInt() == 1 ? false : true)) {
        if (bFullscreenExclusive) {
          ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.FullscreenMode 2")));
          bFullscreenExclusive = true;
        } else {
          ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.FullscreenMode 1")));
          bFullscreenExclusive = false;
        }
      }
    }
    ImGui::TableNextColumn();

    ImGui::TableNextColumn();
    if (ImGui::Checkbox("VSync", &bVSync)) {
      if (bVSync != ConsoleManager.FindConsoleVariable(TEXT("r.VSync"))->GetBool()) {
        if (bVSync) {
          ViewportClient->ConsoleCommand("r.VSync 1");
        } else {
          ViewportClient->ConsoleCommand("r.VSync 0");
        }
      }
    }
    ImGui::TableNextColumn();

    ImGui::TableNextColumn();
    ImGui::InputInt("MaxFPS", &MaxFPS, 1, 5);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS2")) {
      ViewportClient->ConsoleCommand(FString::Printf(TEXT("t.MaxFPS %i"), MaxFPS));
    }
    ImGui::TableNextColumn();
    ImGui::InputInt("ScreenPercent", &ScreenPercentage, 1, 5);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS3")) {
      ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.ScreenPercentage %i"), ScreenPercentage));
    }
    ImGui::TableNextColumn();
    ImGui::InputInt("Res.Quality", &ResolutionQuality, 1, 5);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS4")) {
      ViewportClient->ConsoleCommand(FString::Printf(TEXT("sg.ResolutionQuality %i"), ResolutionQuality));
    }
    ImGui::TableNextColumn();
    ImGui::InputInt("ViewDistance", &ViewDistanceScale, 1, 5);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS5")) {
      ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.ViewDistanceScale %i"), ViewDistanceScale));
    }
    ImGui::TableNextColumn();
    ImGui::InputInt("PP Quality", &PostProcessQuality, 1, 5);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS6")) {
      ViewportClient->ConsoleCommand(FString::Printf(TEXT("sg.PostProcessQuality %i"), PostProcessQuality));
    }
    ImGui::TableNextColumn();
    ImGui::InputInt("Shadow Qual.", &ShadowQuality, 1, 5);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS7")) {
      ViewportClient->ConsoleCommand(FString::Printf(TEXT("sg.ShadowQuality %i"), ShadowQuality));
    }
    ImGui::TableNextColumn();
    ImGui::InputInt("Texture Qual.", &TextureQuality, 1, 5);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS8")) {
      ViewportClient->ConsoleCommand(FString::Printf(TEXT("sg.TextureQuality %i"), TextureQuality));
    }
    ImGui::TableNextColumn();
    ImGui::InputInt("Effects Qual.", &EffectsQuality, 1, 5);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS9")) {
      ViewportClient->ConsoleCommand(FString::Printf(TEXT("sg.EffectsQuality %i"), EffectsQuality));
    }
    ImGui::TableNextColumn();
    ImGui::InputInt("Detail Mode", &DetailMode, 1, 5);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS10")) {
      ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.DetailMode %i"), DetailMode));
    }
    ImGui::TableNextColumn();
    ImGui::InputInt("Skeletal LOD", &SkeletalMeshLODBias, 1, 5);
    ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS11")) {
      ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.SkeletalMeshLODBias %i"), SkeletalMeshLODBias));
    }

    ImGui::EndTable();
  }

  if (ImGui::CollapsingHeader("Memory Stats")) {
    FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
    RenderInfoHelper(TEXT("Mem"), GetMemoryString(MemoryStats.UsedPhysical));
    RenderInfoHelper(TEXT("MemPeak"), GetMemoryString(MemoryStats.PeakUsedPhysical));
    RenderInfoHelper(TEXT("MemAvail"), GetMemoryString(MemoryStats.AvailablePhysical));
    RenderInfoHelper(TEXT("MemTotal"), GetMemoryString(MemoryStats.TotalPhysical));
    RenderInfoHelper(TEXT("VMem"), GetMemoryString(MemoryStats.UsedVirtual));
    RenderInfoHelper(TEXT("VMemPeak"), GetMemoryString(MemoryStats.PeakUsedVirtual));
    RenderInfoHelper(TEXT("VMemAvail"), GetMemoryString(MemoryStats.AvailableVirtual));
    RenderInfoHelper(TEXT("VMemTotal"), GetMemoryString(MemoryStats.TotalVirtual));
  }

  if (ImGui::CollapsingHeader("Viewport Context")) {
    ImGui::BeginDisabled();
    RenderInfoHelper(TEXT("bIsPlayInEditorViewport"), ViewportClient->bIsPlayInEditorViewport);
    RenderInfoHelper(TEXT("GetDPIScale"), ViewportClient->GetDPIScale());
    RenderInfoHelper(TEXT("GetDPIDerivedResolutionFraction"), ViewportClient->GetDPIDerivedResolutionFraction());
    RenderInfoHelper(TEXT("IsCursorVisible"), ViewportClient->Viewport->IsCursorVisible());
    RenderInfoHelper(TEXT("IsForegroundWindow"), ViewportClient->Viewport->IsForegroundWindow());
    RenderInfoHelper(TEXT("IsExclusiveFullscreen"), ViewportClient->Viewport->IsExclusiveFullscreen());
    RenderInfoHelper(TEXT("IsFullscreen"), ViewportClient->Viewport->IsFullscreen());
    RenderInfoHelper(TEXT("IsGameRenderingEnabled"), ViewportClient->Viewport->IsGameRenderingEnabled());
    RenderInfoHelper(TEXT("IsHDRViewport"), ViewportClient->Viewport->IsHDRViewport());
    RenderInfoHelper(TEXT("IsKeyboardAvailable"), ViewportClient->Viewport->IsKeyboardAvailable(0));
    RenderInfoHelper(TEXT("IsMouseAvailable"), ViewportClient->Viewport->IsMouseAvailable(0));
    RenderInfoHelper(TEXT("IsPenActive"), ViewportClient->Viewport->IsPenActive());
    RenderInfoHelper(TEXT("IsPlayInEditorViewport"), ViewportClient->Viewport->IsPlayInEditorViewport());
    RenderInfoHelper(TEXT("IsSlateViewport"), ViewportClient->Viewport->IsSlateViewport());
    RenderInfoHelper(TEXT("IsSoftwareCursorVisible"), ViewportClient->Viewport->IsSoftwareCursorVisible());
    RenderInfoHelper(TEXT("IsStereoRenderingAllowed"), ViewportClient->Viewport->IsStereoRenderingAllowed());
    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("GEngine Context")) {
    ImGui::BeginDisabled();

    RenderInfoHelper(TEXT("IsEditor"), GEngine->IsEditor());
    RenderInfoHelper(TEXT("IsAllowedFramerateSmoothing"), GEngine->IsAllowedFramerateSmoothing());
    RenderInfoHelper(TEXT("bForceDisableFrameRateSmoothing"), GEngine->bForceDisableFrameRateSmoothing);
    RenderInfoHelper(TEXT("bSmoothFrameRate"), GEngine->bSmoothFrameRate);
    RenderInfoHelper(TEXT("MinDesiredFrameRate"), GEngine->MinDesiredFrameRate);
    RenderInfoHelper(TEXT("bUseFixedFrameRate"), GEngine->bUseFixedFrameRate);
    RenderInfoHelper(TEXT("FixedFrameRate"), GEngine->FixedFrameRate);
    RenderInfoHelper(TEXT("bCanBlueprintsTickByDefault"), GEngine->bCanBlueprintsTickByDefault);
    RenderInfoHelper(TEXT("IsControllerIdUsingPlatformUserId"), GEngine->IsControllerIdUsingPlatformUserId());
    RenderInfoHelper(TEXT("IsStereoscopic3D"), GEngine->IsStereoscopic3D(ViewportClient->Viewport));
    RenderInfoHelper(TEXT("IsVanillaProduct"), GEngine->IsVanillaProduct());
    RenderInfoHelper(TEXT("HasMultipleLocalPlayers"), GEngine->HasMultipleLocalPlayers(World));

    RenderInfoHelper(TEXT("AreEditorAnalyticsEnabled"), GEngine->AreEditorAnalyticsEnabled());
    RenderInfoHelper(TEXT("bAllowMultiThreadedAnimationUpdate"), GEngine->bAllowMultiThreadedAnimationUpdate);
    RenderInfoHelper(TEXT("bDisableAILogging"), GEngine->bDisableAILogging);
    RenderInfoHelper(TEXT("bEnableOnScreenDebugMessages"), GEngine->bEnableOnScreenDebugMessages);
    RenderInfoHelper(TEXT("bEnableOnScreenDebugMessagesDisplay"), GEngine->bEnableOnScreenDebugMessagesDisplay);
    RenderInfoHelper(TEXT("bEnableEditorPSysRealtimeLOD"), GEngine->bEnableEditorPSysRealtimeLOD);
    RenderInfoHelper(TEXT("bEnableVisualLogRecordingOnStart"), GEngine->bEnableVisualLogRecordingOnStart);
    RenderInfoHelper(TEXT("bGenerateDefaultTimecode"), GEngine->bGenerateDefaultTimecode);
    RenderInfoHelper(TEXT("bIsInitialized"), GEngine->bIsInitialized);
    RenderInfoHelper(TEXT("bLockReadOnlyLevels"), GEngine->bLockReadOnlyLevels);
    RenderInfoHelper(TEXT("bOptimizeAnimBlueprintMemberVariableAccess"), GEngine->bOptimizeAnimBlueprintMemberVariableAccess);
    RenderInfoHelper(TEXT("bPauseOnLossOfFocus"), GEngine->bPauseOnLossOfFocus);
    RenderInfoHelper(TEXT("bRenderLightMapDensityGrayscale"), GEngine->bRenderLightMapDensityGrayscale);
    RenderInfoHelper(TEXT("bShouldGenerateLowQualityLightmaps_DEPRECATED"), GEngine->bShouldGenerateLowQualityLightmaps_DEPRECATED);
    RenderInfoHelper(TEXT("BSPSelectionHighlightIntensity"), GEngine->BSPSelectionHighlightIntensity);
    RenderInfoHelper(TEXT("bStartedLoadMapMovie"), GEngine->bStartedLoadMapMovie);
    RenderInfoHelper(TEXT("bSubtitlesEnabled"), GEngine->bSubtitlesEnabled);
    RenderInfoHelper(TEXT("bSubtitlesForcedOff"), GEngine->bSubtitlesForcedOff);
    RenderInfoHelper(TEXT("bSuppressMapWarnings"), GEngine->bSuppressMapWarnings);
    RenderInfoHelper(TEXT("DisplayGamma"), GEngine->DisplayGamma);
    RenderInfoHelper(TEXT("IsAutosaving"), GEngine->IsAutosaving());
    RenderInfoHelper(TEXT("MaximumLoopIterationCount"), GEngine->MaximumLoopIterationCount);
    RenderInfoHelper(TEXT("MaxLightMapDensity"), GEngine->MaxLightMapDensity);
    RenderInfoHelper(TEXT("MaxOcclusionPixelsFraction"), GEngine->MaxOcclusionPixelsFraction);
    RenderInfoHelper(TEXT("MaxParticleResize"), GEngine->MaxParticleResize);
    RenderInfoHelper(TEXT("MaxParticleResizeWarn"), GEngine->MaxParticleResizeWarn);
    RenderInfoHelper(TEXT("MaxPixelShaderAdditiveComplexityCount"), GEngine->MaxPixelShaderAdditiveComplexityCount);
    RenderInfoHelper(TEXT("UseSkeletalMeshMinLODPerQualityLevels"), GEngine->UseSkeletalMeshMinLODPerQualityLevels);
    RenderInfoHelper(TEXT("UseStaticMeshMinLODPerQualityLevels"), GEngine->UseStaticMeshMinLODPerQualityLevels);

    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("RHI Context")) {
    ImGui::BeginDisabled();

    RenderInfoHelper(TEXT("GRHIAdapterDriverDate"), GRHIAdapterDriverDate);
    RenderInfoHelper(TEXT("GRHIAdapterDriverOnDenyList"), GRHIAdapterDriverOnDenyList);
    RenderInfoHelper(TEXT("GRHIAdapterInternalDriverVersion"), GRHIAdapterInternalDriverVersion);
    RenderInfoHelper(TEXT("GRHIAdapterName"), GRHIAdapterName);
    RenderInfoHelper(TEXT("GRHIAdapterUserDriverVersion"), GRHIAdapterUserDriverVersion);
    RenderInfoHelper(TEXT("GRHIAttachmentVariableRateShadingEnabled"), GRHISupportsAttachmentVariableRateShading);
    RenderInfoHelper(TEXT("GRHIDeviceId"), GRHIDeviceId);
    RenderInfoHelper(TEXT("GRHIDeviceIsAMDPreGCNArchitecture"), GRHIDeviceIsAMDPreGCNArchitecture);
    RenderInfoHelper(TEXT("GRHIDeviceIsIntegrated"), GRHIDeviceIsIntegrated);
    RenderInfoHelper(TEXT("GRHIDeviceRevision"), GRHIDeviceRevision);
    RenderInfoHelper(TEXT("GRHIForceNoDeletionLatencyForStreamingTextures"), GRHIForceNoDeletionLatencyForStreamingTextures);
    RenderInfoHelper(TEXT("GRHIIsHDREnabled"), GRHIIsHDREnabled);
    RenderInfoHelper(TEXT("GRHILazyShaderCodeLoading"), GRHILazyShaderCodeLoading);
    RenderInfoHelper(TEXT("GRHIMinimumWaveSize"), GRHIMinimumWaveSize);
    RenderInfoHelper(TEXT("GRHIMaximumWaveSize"), GRHIMaximumWaveSize);
    RenderInfoHelper(TEXT("GRHINeedsExtraDeletionLatency"), GRHINeedsExtraDeletionLatency);
    RenderInfoHelper(TEXT("GRHINeedsUnatlasedCSMDepthsWorkaround"), GRHINeedsUnatlasedCSMDepthsWorkaround);
    RenderInfoHelper(TEXT("GRHIPersistentThreadGroupCount"), GRHIPersistentThreadGroupCount);
    RenderInfoHelper(TEXT("GRHIPresentCounter"), GRHIPresentCounter);
    RenderInfoHelper(TEXT("GRHIRayTracingAccelerationStructureAlignment"), GRHIRayTracingAccelerationStructureAlignment);
    RenderInfoHelper(TEXT("GRHIRayTracingScratchBufferAlignment"), GRHIRayTracingScratchBufferAlignment);
    RenderInfoHelper(TEXT("GRHIRequiresRenderTargetForPixelShaderUAVs"), GRHIRequiresRenderTargetForPixelShaderUAVs);
    RenderInfoHelper(TEXT("GRHISupportsArrayIndexFromAnyShader"), GRHISupportsArrayIndexFromAnyShader);
    RenderInfoHelper(TEXT("GRHISupportsAsyncPipelinePrecompile"), GRHISupportsAsyncPipelinePrecompile);
    RenderInfoHelper(TEXT("GRHISupportsAsyncTextureCreation"), GRHISupportsAsyncTextureCreation);
    RenderInfoHelper(TEXT("GRHISupportsAtomicUInt64"), GRHISupportsAtomicUInt64);
    RenderInfoHelper(TEXT("GRHISupportsAttachmentVariableRateShading"), GRHISupportsAttachmentVariableRateShading);
    RenderInfoHelper(TEXT("GRHISupportsBackBufferWithCustomDepthStencil"), GRHISupportsBackBufferWithCustomDepthStencil);
    RenderInfoHelper(TEXT("GRHISupportsBaseVertexIndex"), GRHISupportsBaseVertexIndex);
    RenderInfoHelper(TEXT("GRHISupportsComplexVariableRateShadingCombinerOps"), GRHISupportsComplexVariableRateShadingCombinerOps);
    RenderInfoHelper(TEXT("GRHISupportsConservativeRasterization"), GRHISupportsConservativeRasterization);
    RenderInfoHelper(TEXT("GRHISupportsDepthUAV"), GRHISupportsDepthUAV);
    RenderInfoHelper(TEXT("GRHISupportsDirectGPUMemoryLock"), GRHISupportsDirectGPUMemoryLock);
    RenderInfoHelper(TEXT("GRHISupportsDrawIndirect"), GRHISupportsDrawIndirect);
    RenderInfoHelper(TEXT("GRHISupportsDX12AtomicUInt64"), GRHISupportsDX12AtomicUInt64);
    RenderInfoHelper(TEXT("GRHISupportsDynamicResolution"), GRHISupportsDynamicResolution);
    RenderInfoHelper(TEXT("GRHISupportsEfficientUploadOnResourceCreation"), GRHISupportsEfficientUploadOnResourceCreation);
    RenderInfoHelper(TEXT("GRHISupportsExactOcclusionQueries"), GRHISupportsExactOcclusionQueries);
    RenderInfoHelper(TEXT("GRHISupportsExplicitFMask"), GRHISupportsExplicitFMask);
    RenderInfoHelper(TEXT("GRHISupportsExplicitHTile"), GRHISupportsExplicitHTile);
    RenderInfoHelper(TEXT("GRHISupportsFirstInstance"), GRHISupportsFirstInstance);
    RenderInfoHelper(TEXT("GRHISupportsFrameCyclesBubblesRemoval"), GRHISupportsFrameCyclesBubblesRemoval);
    RenderInfoHelper(TEXT("GRHISupportsGPUTimestampBubblesRemoval"), GRHISupportsGPUTimestampBubblesRemoval);
    RenderInfoHelper(TEXT("GRHISupportsHDROutput"), GRHISupportsHDROutput);
    RenderInfoHelper(TEXT("GRHISupportsInlineRayTracing"), GRHISupportsInlineRayTracing);
    RenderInfoHelper(TEXT("GRHISupportsLargerVariableRateShadingSizes"), GRHISupportsLargerVariableRateShadingSizes);
    RenderInfoHelper(TEXT("GRHISupportsLateVariableRateShadingUpdate"), GRHISupportsLateVariableRateShadingUpdate);
    RenderInfoHelper(TEXT("GRHISupportsLazyShaderCodeLoading"), GRHISupportsLazyShaderCodeLoading);
    RenderInfoHelper(TEXT("GRHISupportsMapWriteNoOverwrite"), GRHISupportsMapWriteNoOverwrite);
    RenderInfoHelper(TEXT("GRHISupportsMeshShadersTier0"), GRHISupportsMeshShadersTier0);
    RenderInfoHelper(TEXT("GRHISupportsMeshShadersTier1"), GRHISupportsMeshShadersTier1);
    RenderInfoHelper(TEXT("GRHISupportsMSAADepthSampleAccess"), GRHISupportsMSAADepthSampleAccess);
    RenderInfoHelper(TEXT("GRHISupportsMultithreadedResources"), GRHISupportsMultithreadedResources);
    RenderInfoHelper(TEXT("GRHISupportsMultithreadedShaderCreation"), GRHISupportsMultithreadedShaderCreation);
    RenderInfoHelper(TEXT("GRHISupportsMultithreading"), GRHISupportsMultithreading);
    RenderInfoHelper(TEXT("GRHISupportsParallelRHIExecute"), GRHISupportsParallelRHIExecute);
    RenderInfoHelper(TEXT("GRHISupportsPipelineFileCache"), GRHISupportsPipelineFileCache);
    RenderInfoHelper(TEXT("GRHISupportsPipelineStateSortKey"), GRHISupportsPipelineStateSortKey);
    RenderInfoHelper(TEXT("GRHISupportsPipelineVariableRateShading"), GRHISupportsPipelineVariableRateShading);
    RenderInfoHelper(TEXT("GRHISupportsPixelShaderUAVs"), GRHISupportsPixelShaderUAVs);
    RenderInfoHelper(TEXT("GRHISupportsPrimitiveShaders"), GRHISupportsPrimitiveShaders);
    RenderInfoHelper(TEXT("GRHISupportsQuadTopology"), GRHISupportsQuadTopology);
    RenderInfoHelper(TEXT("GRHISupportsRawViewsForAnyBuffer"), GRHISupportsRawViewsForAnyBuffer);
    RenderInfoHelper(TEXT("GRHISupportsRayTracing"), GRHISupportsRayTracing);
    RenderInfoHelper(TEXT("GRHISupportsRayTracingAMDHitToken"), GRHISupportsRayTracingAMDHitToken);
    RenderInfoHelper(TEXT("GRHISupportsRayTracingAsyncBuildAccelerationStructure"), GRHISupportsRayTracingAsyncBuildAccelerationStructure);
    RenderInfoHelper(TEXT("GRHISupportsRayTracingDispatchIndirect"), GRHISupportsRayTracingDispatchIndirect);
    RenderInfoHelper(TEXT("GRHISupportsRayTracingPSOAdditions"), GRHISupportsRayTracingPSOAdditions);
    RenderInfoHelper(TEXT("GRHISupportsRayTracingShaders"), GRHISupportsRayTracingShaders);
    RenderInfoHelper(TEXT("GRHISupportsRectTopology"), GRHISupportsRectTopology);
    RenderInfoHelper(TEXT("GRHISupportsResummarizeHTile"), GRHISupportsResummarizeHTile);
    RenderInfoHelper(TEXT("GRHISupportsRHIOnTaskThread"), GRHISupportsRHIOnTaskThread);
    RenderInfoHelper(TEXT("GRHISupportsRHIThread"), GRHISupportsRHIThread);
    RenderInfoHelper(TEXT("GRHISupportsRWTextureBuffers"), GRHISupportsRWTextureBuffers);
    RenderInfoHelper(TEXT("GRHISupportsSeparateDepthStencilCopyAccess"), GRHISupportsSeparateDepthStencilCopyAccess);
    RenderInfoHelper(TEXT("GRHISupportsShaderTimestamp"), GRHISupportsShaderTimestamp);
    RenderInfoHelper(TEXT("GRHISupportsStencilRefFromPixelShader"), GRHISupportsStencilRefFromPixelShader);
    RenderInfoHelper(TEXT("GRHISupportsTextureStreaming"), GRHISupportsTextureStreaming);
    RenderInfoHelper(TEXT("GRHISupportsUAVFormatAliasing"), GRHISupportsUAVFormatAliasing);
    RenderInfoHelper(TEXT("GRHISupportsUpdateFromBufferTexture"), GRHISupportsUpdateFromBufferTexture);
    RenderInfoHelper(TEXT("GRHISupportsVariableRateShadingAttachmentArrayTextures"), GRHISupportsVariableRateShadingAttachmentArrayTextures);
    RenderInfoHelper(TEXT("GRHISupportsWaveOperations"), GRHISupportsWaveOperations);
    RenderInfoHelper(TEXT("GRHIThreadNeedsKicking"), GRHIThreadNeedsKicking);
    RenderInfoHelper(TEXT("GRHIThreadTime"), GRHIThreadTime);
    RenderInfoHelper(TEXT("GRHIValidationEnabled"), GRHIValidationEnabled);
    //RenderInfoHelper(TEXT("GRHIVariableRateShadingEnabled"), GVRSImageManager.IsAttachmentVRSEnabled());
    RenderInfoHelper(TEXT("GRHIVendorId"), GRHIVendorId);

    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("Platform Context")) {
    ImGui::BeginDisabled();
    RenderInfoHelper(TEXT("AllowAudioThread"), FGenericPlatformMisc::AllowAudioThread());
    RenderInfoHelper(TEXT("AllowLocalCaching"), FGenericPlatformMisc::AllowLocalCaching());
    RenderInfoHelper(TEXT("AllowThreadHeartBeat"), FGenericPlatformMisc::AllowThreadHeartBeat());
    RenderInfoHelper(TEXT("CloudDir"), FGenericPlatformMisc::CloudDir());
    RenderInfoHelper(TEXT("DesktopTouchScreen"), FGenericPlatformMisc::DesktopTouchScreen());
    RenderInfoHelper(TEXT("EngineDir"), FGenericPlatformMisc::EngineDir());
    RenderInfoHelper(TEXT("FullscreenSameAsWindowedFullscreen"), FGenericPlatformMisc::FullscreenSameAsWindowedFullscreen());
    RenderInfoHelper(TEXT("GamePersistentDownloadDir"), FGenericPlatformMisc::GamePersistentDownloadDir());
    RenderInfoHelper(TEXT("GameTemporaryDownloadDir"), FGenericPlatformMisc::GameTemporaryDownloadDir());
    RenderInfoHelper(TEXT("GetBatteryLevel"), FGenericPlatformMisc::GetBatteryLevel());
    RenderInfoHelper(TEXT("GetBrightness"), FGenericPlatformMisc::GetBrightness());
    RenderInfoHelper(TEXT("GetCPUBrand"), FGenericPlatformMisc::GetCPUBrand());
    RenderInfoHelper(TEXT("GetCPUChipset"), FGenericPlatformMisc::GetCPUChipset());
    RenderInfoHelper(TEXT("GetCPUInfo"), FGenericPlatformMisc::GetCPUInfo());
    RenderInfoHelper(TEXT("GetCPUVendor"), FGenericPlatformMisc::GetCPUVendor());
    RenderInfoHelper(TEXT("GetDefaultDeviceProfileName"), FGenericPlatformMisc::GetDefaultDeviceProfileName());
    RenderInfoHelper(TEXT("GetDefaultLanguage"), FGenericPlatformMisc::GetDefaultLanguage());
    RenderInfoHelper(TEXT("GetDefaultLocale"), FGenericPlatformMisc::GetDefaultLocale());
    RenderInfoHelper(TEXT("GetDefaultPathSeparator"), FGenericPlatformMisc::GetDefaultPathSeparator());
    RenderInfoHelper(TEXT("GetDeviceId"), FGenericPlatformMisc::GetDeviceId());
    RenderInfoHelper(TEXT("GetDeviceMakeAndModel"), FGenericPlatformMisc::GetDeviceMakeAndModel());
    RenderInfoHelper(TEXT("GetDeviceTemperatureLevel"), FGenericPlatformMisc::GetDeviceTemperatureLevel());
    RenderInfoHelper(TEXT("GetDeviceVolume"), FGenericPlatformMisc::GetDeviceVolume());
    RenderInfoHelper(TEXT("GetEngineMode"), FGenericPlatformMisc::GetEngineMode());
    RenderInfoHelper(TEXT("GetEpicAccountId"), FGenericPlatformMisc::GetEpicAccountId());
    RenderInfoHelper(TEXT("GetFileManagerName"), FGenericPlatformMisc::GetFileManagerName());
    RenderInfoHelper(TEXT("GetLastError"), FGenericPlatformMisc::GetLastError());
    RenderInfoHelper(TEXT("GetLocalCurrencyCode"), FGenericPlatformMisc::GetLocalCurrencyCode());
    RenderInfoHelper(TEXT("GetLocalCurrencySymbol"), FGenericPlatformMisc::GetLocalCurrencySymbol());
    RenderInfoHelper(TEXT("GetLoginId"), FGenericPlatformMisc::GetLoginId());
    RenderInfoHelper(TEXT("GetMaxPathLength"), FGenericPlatformMisc::GetMaxPathLength());
    RenderInfoHelper(TEXT("GetMaxRefreshRate"), FGenericPlatformMisc::GetMaxRefreshRate());
    RenderInfoHelper(TEXT("GetMaxSupportedRefreshRate"), FGenericPlatformMisc::GetMaxSupportedRefreshRate());
    RenderInfoHelper(TEXT("GetMaxSyncInterval"), FGenericPlatformMisc::GetMaxSyncInterval());
    RenderInfoHelper(TEXT("GetMobilePropagateAlphaSetting"), FGenericPlatformMisc::GetMobilePropagateAlphaSetting());
    RenderInfoHelper(TEXT("GetNullRHIShaderFormat"), FGenericPlatformMisc::GetNullRHIShaderFormat());
    RenderInfoHelper(TEXT("GetOperatingSystemId"), FGenericPlatformMisc::GetOperatingSystemId());
    RenderInfoHelper(TEXT("GetOSVersion"), FGenericPlatformMisc::GetOSVersion());
    RenderInfoHelper(TEXT("GetPathVarDelimiter"), FGenericPlatformMisc::GetPathVarDelimiter());
    RenderInfoHelper(TEXT("GetPlatformFeaturesModuleName"), FGenericPlatformMisc::GetPlatformFeaturesModuleName());
    RenderInfoHelper(TEXT("GetPrimaryGPUBrand"), FGenericPlatformMisc::GetPrimaryGPUBrand());
    RenderInfoHelper(TEXT("GetTimeZoneId"), FGenericPlatformMisc::GetTimeZoneId());
    RenderInfoHelper(TEXT("GetUBTPlatform"), FGenericPlatformMisc::GetUBTPlatform());
    RenderInfoHelper(TEXT("GetUniqueAdvertisingId"), FGenericPlatformMisc::GetUniqueAdvertisingId());
    RenderInfoHelper(TEXT("GetUseVirtualJoysticks"), FGenericPlatformMisc::GetUseVirtualJoysticks());
    RenderInfoHelper(TEXT("GetVolumeButtonsHandledBySystem"), FGenericPlatformMisc::GetVolumeButtonsHandledBySystem());
    RenderInfoHelper(TEXT("HasActiveWiFiConnection"), FGenericPlatformMisc::HasActiveWiFiConnection());
    RenderInfoHelper(TEXT("HasMemoryWarningHandler"), FGenericPlatformMisc::HasMemoryWarningHandler());
    RenderInfoHelper(TEXT("HasNonoptionalCPUFeatures"), FGenericPlatformMisc::HasNonoptionalCPUFeatures());
    RenderInfoHelper(TEXT("HasProjectPersistentDownloadDir"), FGenericPlatformMisc::HasProjectPersistentDownloadDir());
    RenderInfoHelper(TEXT("HasSeparateChannelForDebugOutput"), FGenericPlatformMisc::HasSeparateChannelForDebugOutput());
    RenderInfoHelper(TEXT("HasVariableHardware"), FGenericPlatformMisc::HasVariableHardware());
    RenderInfoHelper(TEXT("Is64bitOperatingSystem"), FGenericPlatformMisc::Is64bitOperatingSystem());
    RenderInfoHelper(TEXT("IsDebuggerPresent"), FGenericPlatformMisc::IsDebuggerPresent());
    RenderInfoHelper(TEXT("IsEnsureAllowed"), FGenericPlatformMisc::IsEnsureAllowed());
    RenderInfoHelper(TEXT("IsInLowPowerMode"), FGenericPlatformMisc::IsInLowPowerMode());
    RenderInfoHelper(TEXT("IsLocalPrintThreadSafe"), FGenericPlatformMisc::IsLocalPrintThreadSafe());
    RenderInfoHelper(TEXT("IsPackagedForDistribution"), FGenericPlatformMisc::IsPackagedForDistribution());
    RenderInfoHelper(TEXT("IsPGOEnabled"), FGenericPlatformMisc::IsPGOEnabled());
    RenderInfoHelper(TEXT("IsRegisteredForRemoteNotifications"), FGenericPlatformMisc::IsRegisteredForRemoteNotifications());
    RenderInfoHelper(TEXT("IsRemoteSession"), FGenericPlatformMisc::IsRemoteSession());
    RenderInfoHelper(TEXT("IsRunningInCloud"), FGenericPlatformMisc::IsRunningInCloud());
    RenderInfoHelper(TEXT("IsRunningOnBattery"), FGenericPlatformMisc::IsRunningOnBattery());
    RenderInfoHelper(TEXT("LaunchDir"), FGenericPlatformMisc::LaunchDir());
    RenderInfoHelper(TEXT("NeedsNonoptionalCPUFeaturesCheck"), FGenericPlatformMisc::NeedsNonoptionalCPUFeaturesCheck());
    RenderInfoHelper(TEXT("NumberOfCores"), FGenericPlatformMisc::NumberOfCores());
    RenderInfoHelper(TEXT("NumberOfCoresIncludingHyperthreads"), FGenericPlatformMisc::NumberOfCoresIncludingHyperthreads());
    RenderInfoHelper(TEXT("NumberOfIOWorkerThreadsToSpawn"), FGenericPlatformMisc::NumberOfIOWorkerThreadsToSpawn());
    RenderInfoHelper(TEXT("NumberOfWorkerThreadsToSpawn"), FGenericPlatformMisc::NumberOfWorkerThreadsToSpawn());
    RenderInfoHelper(TEXT("ProjectDir"), FGenericPlatformMisc::ProjectDir());
    RenderInfoHelper(TEXT("SupportsBackbufferSampling"), FGenericPlatformMisc::SupportsBackbufferSampling());
    RenderInfoHelper(TEXT("SupportsBrightness"), FGenericPlatformMisc::SupportsBrightness());
    RenderInfoHelper(TEXT("SupportsDeviceCheckToken"), FGenericPlatformMisc::SupportsDeviceCheckToken());
    RenderInfoHelper(TEXT("SupportsForceTouchInput"), FGenericPlatformMisc::SupportsForceTouchInput());
    RenderInfoHelper(TEXT("SupportsFullCrashDumps"), FGenericPlatformMisc::SupportsFullCrashDumps());
    RenderInfoHelper(TEXT("SupportsLocalCaching"), FGenericPlatformMisc::SupportsLocalCaching());
    RenderInfoHelper(TEXT("SupportsMessaging"), FGenericPlatformMisc::SupportsMessaging());
    RenderInfoHelper(TEXT("SupportsMultithreadedFileHandles"), FGenericPlatformMisc::SupportsMultithreadedFileHandles());
    RenderInfoHelper(TEXT("SupportsTouchInput"), FGenericPlatformMisc::SupportsTouchInput());
    RenderInfoHelper(TEXT("UseHDRByDefault"), FGenericPlatformMisc::UseHDRByDefault());
    RenderInfoHelper(TEXT("UseRenderThread"), FGenericPlatformMisc::UseRenderThread());
    ImGui::EndDisabled();
  }
}

void FDFX_StatData::RenderShadersTab() {
  static ImGuiTableFlags TableFlags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
    ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
    ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
    ImGuiTableFlags_SizingStretchProp;
  ImVec2 OuterSize = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 15);
  float InnerWidth = ImGui::CalcTextSize("X").x * 80;
  int32 ShaderTotal = 0;
  double TimeTotal = 0.0;
  if (ImGui::BeginTable("ShaderCompilerLog", 4, TableFlags, OuterSize, InnerWidth)) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Type");
    ImGui::TableSetupColumn("Hash");
    ImGui::TableSetupColumn("Time (ms)");
    ImGui::TableSetupColumn("Count");
    ImGui::TableHeadersRow();
    for (const FShaderCompileLog& ShaderLog : ShaderCompileLogs) {
      if (ShaderLog.Count == 0) {
        continue;
      }

      ImGui::TableNextColumn();
      switch (ShaderLog.Type) {
        case 1:
          ImGui::Text("CS");
          break;
        case 2:
          ImGui::Text("GS");
          break;
        case 4:
          ImGui::Text("RT");
          break;
      }
      ImGui::TableNextColumn();
      ImGui::Text("%s", TCHAR_TO_ANSI(*ShaderLog.Hash.Left(40)));
      ImGui::TableNextColumn();
      ImGui::Text("%.5f", ShaderLog.Time);
      ImGui::TableNextColumn();
      ImGui::Text("%i", ShaderLog.Count);
      TimeTotal += ShaderLog.Time;
      ShaderTotal++;
    }
    ImGui::EndTable();
    RenderHelpMarker("CS = ComputeShader, GS = GraphicsShader, RT = RayTracing.");
    ImGui::SameLine();
    ImGui::Text("Total Shaders : %i", ShaderTotal);
    ImGui::SameLine();
    ImGui::Text(" | Time : %.5f", TimeTotal);
  }

  if (ImGui::CollapsingHeader("r.ShaderPipelineCache Context")) {
    IConsoleManager& ConsoleManager = IConsoleManager::Get();
    static bool bEnabled = ConsoleManager.FindConsoleVariable(TEXT("r.ShaderPipelineCache.Enabled"))->GetBool();
    static int32 BatchSize = ConsoleManager.FindConsoleVariable(TEXT("r.ShaderPipelineCache.BatchSize"))->GetInt();
    static int32 BackgroundBatchSize = ConsoleManager.FindConsoleVariable(TEXT("r.ShaderPipelineCache.BackgroundBatchSize"))->GetInt();
    static bool bLogPSO = ConsoleManager.FindConsoleVariable(TEXT("r.ShaderPipelineCache.LogPSO"))->GetBool();
    static bool bSaveAfterPSOsLogged = ConsoleManager.FindConsoleVariable(TEXT("r.ShaderPipelineCache.SaveAfterPSOsLogged"))->GetBool();

    ImGui::BeginDisabled();
    RenderInfoHelper(TEXT("Enabled"), bEnabled);
    RenderInfoHelper(TEXT("BatchSize"), BatchSize);
    RenderInfoHelper(TEXT("BackgroundBatchSize"), BackgroundBatchSize);
    RenderInfoHelper(TEXT("LogPSO"), bLogPSO);
    RenderInfoHelper(TEXT("SaveAfterPSOsLogged"), bSaveAfterPSOsLogged);
    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("ShaderCodeLibrary Context")) {
    ImGui::BeginDisabled();
    RenderInfoHelper(TEXT("IsEnabled"), FShaderCodeLibrary::IsEnabled());
    RenderInfoHelper(TEXT("GetRuntimeShaderPlatform"), static_cast<int32>(FShaderCodeLibrary::GetRuntimeShaderPlatform()));
    RenderInfoHelper(TEXT("GetShaderCount"), FShaderCodeLibrary::GetShaderCount());
    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("PipelineFileCacheManager Context")) {
    ImGui::BeginDisabled();
    RenderInfoHelper(TEXT("IsPipelineFileCacheEnabled"), FPipelineFileCacheManager::IsPipelineFileCacheEnabled());
    RenderInfoHelper(TEXT("NumPSOsLogged"), FPipelineFileCacheManager::NumPSOsLogged());
    RenderInfoHelper(TEXT("GetGameUsageMask"), FPipelineFileCacheManager::GetGameUsageMask());
    ImGui::EndDisabled();
  }

/*    
    InfoHelper("GetGameVersionForPSOFileCache", FShaderPipelineCache::GetGameVersionForPSOFileCache());
    InfoHelper("GetStatId", FShaderPipelineCache::GetStatId());
    InfoHelper("IsBatchingPaused", FShaderPipelineCache::IsBatchingPaused());
    InfoHelper("IsPrecompiling", FShaderPipelineCache::IsPrecompiling());
    InfoHelper("IsTickable", FShaderPipelineCache::IsTickable());
    InfoHelper("NeedsRenderingResumedForRenderingThreadTick", FShaderPipelineCache::NeedsRenderingResumedForRenderingThreadTick());
    InfoHelper("NumPrecompilesRemaining", FShaderPipelineCache::NumPrecompilesRemaining());
*/

// OLD SHADER STATS
/* CRASH WITHOUT A LOCK
  if (ImGui::CollapsingHeader("Shader Compiler Stats")) {
    ImGui::BeginTable("Shader Compiler Stats", 5);
    ImGui::TableNextColumn(); ImGui::Text("Compiled");
    ImGui::TableNextColumn(); ImGui::Text("CompiledDouble");
    ImGui::TableNextColumn(); ImGui::Text("CompileTime");
    ImGui::TableNextColumn(); ImGui::Text("Cooked");
    ImGui::TableNextColumn(); ImGui::Text("CookedDouble");
    for (auto a_sstat : GShaderCompilerStats->GetShaderCompilerStats()) {
      for (auto sstat : a_sstat) {
        ImGui::TableNextColumn(); ImGui::Text("%i", sstat.Value.Compiled);
        ImGui::TableNextColumn(); ImGui::Text("%i", sstat.Value.CompiledDouble);
        ImGui::TableNextColumn(); ImGui::Text("%f", sstat.Value.CompileTime);
        ImGui::TableNextColumn(); ImGui::Text("%i", sstat.Value.Cooked);
        ImGui::TableNextColumn(); ImGui::Text("%i", sstat.Value.CookedDouble);
        //ImGui::TableNextColumn(); ImGui::Text("%i", sstat.Value.PermutationCompilations);
      }
    }
    ImGui::EndTable();
    InfoHelper("TotalShadersCompiled", GShaderCompilerStats->GetTotalShadersCompiled());
    // Cause a FatalCrash because it uses a lock
    //InfoHelper("TimeShaderCompilationWasActive", GShaderCompilerStats->GetTimeShaderCompilationWasActive());
  }
*/

/* Fatal Crash
  if (ImGui::CollapsingHeader("Shader Compiling Manager")) {
    ImGui::BeginDisabled();
    ImGui::Indent();
    InfoHelper("AllowShaderCompiling", AllowShaderCompiling());
    InfoHelper("AllowGlobalShaderLoad", AllowGlobalShaderLoad());
    //InfoHelper("AllowAsynchronousShaderCompiling", GShaderCompilingManager->AllowAsynchronousShaderCompiling());
    //InfoHelper("AreWarningsSuppressed", GShaderCompilingManager->AreWarningsSuppressed(EShaderPlatform::SP_PCD3D_SM6));
    //InfoHelper("GetNumLocalWorkers", GShaderCompilingManager->GetNumLocalWorkers());
    //InfoHelper("GetNumPendingJobs", GShaderCompilingManager->GetNumPendingJobs());
    //InfoHelper("GetNumOutstandingJobs", GShaderCompilingManager->GetNumOutstandingJobs());
    //InfoHelper("GetNumRemainingJobs", GShaderCompilingManager->GetNumRemainingJobs());
    InfoHelper("GetStaticAssetTypeName", GShaderCompilingManager->GetStaticAssetTypeName().ToString());
    InfoHelper("HasShaderJobs", GShaderCompilingManager->HasShaderJobs());
    InfoHelper("IgnoreAllThrottling", GShaderCompilingManager->IgnoreAllThrottling());
    InfoHelper("IsCompiling", GShaderCompilingManager->IsCompiling());
    InfoHelper("IsShaderCompilationSkipped", GShaderCompilingManager->IsShaderCompilationSkipped());
    InfoHelper("ShouldDisplayCompilingNotification", GShaderCompilingManager->ShouldDisplayCompilingNotification());
    InfoHelper("GetAbsoluteShaderDebugInfoDirectory\n", GShaderCompilingManager->GetAbsoluteShaderDebugInfoDirectory());
    ImGui::Unindent();
    ImGui::EndDisabled();
  }
*/
}

void FDFX_StatData::RenderStatTab() {
  for (FDFXStatCmd& Elem : FDFX_Module::StatCommands) {
    Elem.bEnabled = ViewportClient->IsStatEnabled(Elem.Command);
  }

  if (ImGui::CollapsingHeader("Favorites")) {
    ImGui::BeginTable("##tblFavBase", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextColumn();
    ImGui::TableNextColumn();
    ImGui::BeginTable("##tblFavToggles", 2, ImGuiTableFlags_SizingStretchProp);
    RenderStats(EDFXStatCategory::Favorites);
    ImGui::EndTable();
    ImGui::EndTable();
  }

  if (ImGui::CollapsingHeader("Common")) {
    ImGui::BeginTable("##tblCommonBase", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextColumn();
    ImGui::TableNextColumn();
    ImGui::BeginTable("##tblCommonToggles", 2, ImGuiTableFlags_SizingStretchProp);
    RenderStats(EDFXStatCategory::Common);
    ImGui::EndTable();
    ImGui::EndTable();
  }

  if (ImGui::CollapsingHeader("Performance")) {
    ImGui::BeginTable("##tblPerfBase", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextColumn();
    ImGui::TableNextColumn();
    ImGui::BeginTable("##tblPerfToggles", 2, ImGuiTableFlags_SizingStretchProp);
    RenderStats(EDFXStatCategory::Performance);
    ImGui::EndTable();
    ImGui::EndTable();
  }

  if (ImGui::CollapsingHeader("List")) {
    static char FilterList[128] = "";
    ImGui::Text("Filter : ");
    ImGui::SameLine();
    ImGui::InputText("##ListFilter", FilterList, IM_ARRAYSIZE(FilterList));
    ImGui::BeginTable("##tblListBase", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextColumn();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Add To Favorite");
    ImGui::BeginTable("##tblListAddToFav", 1, ImGuiTableFlags_SizingStretchProp);
    char ChkId[5] = "";
    bool LocalFav = false;
    for (int32 i = 0; i < FDFX_Module::StatCommands.Num(); ++i) {
      bool bFavFlag = (FDFX_Module::StatCommands[i].Category == EDFXStatCategory::Favorites);
      LocalFav = bFavFlag;
      snprintf(ChkId, 5, "##%i", i);
      ImGui::TableNextColumn();
      ImGui::Checkbox(ChkId, &LocalFav);
      if (LocalFav) {
        FDFX_Module::StatCommands[i].Category = EDFXStatCategory::Favorites;
      } else {
        if (FDFX_Module::StatCommands[i].Category == EDFXStatCategory::Favorites) {
          FDFX_Module::StatCommands[i].Category = EDFXStatCategory::None;
        }
      }
    }
    ImGui::EndTable();
    ImGui::TableNextColumn();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "STAT Command");
    ImGui::BeginTable("##tblListToggles", 2, ImGuiTableFlags_SizingStretchProp);
    RenderStats(EDFXStatCategory::All, FString(FilterList));
    ImGui::EndTable();
    ImGui::EndTable();
  }

  if (ImGui::CollapsingHeader("Capture")) {
    ImGui::Text("Starts a statistics capture, creating a new file in the Profiling directory.");
    if (ImGui::Button("StartFile")) {
      ViewportClient->ConsoleCommand("Stat StartFile");
    }
    if (ImGui::Button("StopFile")) {
      ViewportClient->ConsoleCommand("Stat StopFile");
    }
  }

  if (ImGui::CollapsingHeader("Extras")) {
    if (ImGui::Button("Stat Help")) {
      ViewportClient->ConsoleCommand("Stat Help");
      ImGui::SameLine();
      RenderHelpMarker("List available STAT commands in the OutputLog window.");
    }
  }
}

void FDFX_StatData::RenderSettingsTab() {
  if (ThreadPlotConfig.HistoryDuration > GlobalStatHistoryDuration) {
    ThreadPlotConfig.HistoryDuration = GlobalStatHistoryDuration;
  }
  if (FramePlotConfig.HistoryDuration > GlobalStatHistoryDuration) {
    FramePlotConfig.HistoryDuration = GlobalStatHistoryDuration;
  }
  if (FPSPlotConfig.HistoryDuration > GlobalStatHistoryDuration) {
    FPSPlotConfig.HistoryDuration = GlobalStatHistoryDuration;
  }

  if (ImGui::CollapsingHeader("Graphs")) {
    ImGui::Checkbox("Display Graphs", &bShowPlots);
    ImGui::Text("History :");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::SliderFloat("##HistoryGlobal", &GlobalStatHistoryDuration, 0.1f, 10.0f, "%.1f s");

    if (bShowPlots) {
      ImGui::Indent();
      if (ImGui::CollapsingHeader("Threads")) {
        ImGui::Checkbox("Display Threads", &ThreadPlotConfig.bVisible);
        ImGui::Checkbox("Sort plot order", &bSortPlots);
        ImGui::SameLine();
        RenderHelpMarker("Sort graphs order to display better colors but can cause flickering if two threads have similar values.");
        ImGui::SliderFloat("History##1", &ThreadPlotConfig.HistoryDuration, 0.1f, GlobalStatHistoryDuration, "%.1f s");
        ImGui::SliderFloat("Position X##1", &ThreadPlotConfig.Position.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Position Y##1", &ThreadPlotConfig.Position.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size X##1", &ThreadPlotConfig.Size.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size Y##1", &ThreadPlotConfig.Size.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Range Min##1", &ThreadPlotConfig.Range.x, 0.1f, 60.0f, "%.3f ms");
        ImGui::SliderFloat("Range Max##1", &ThreadPlotConfig.Range.y, 0.1f, 60.0f, "%.3f ms");
        ImGui::ColorEdit4("Background##1", &ThreadPlotConfig.BackgroundColor.x);
        ImGui::SliderFloat("Plot Alpha##1", &ThreadPlotConfig.FillAlpha, 0.0f, 1.0f, "%.2f");
        ImGui::ColorEdit4("Plot Background##1", &ThreadPlotConfig.PlotBackgroundColor.x);
        if (ImGui::CollapsingHeader("Game##01")) {
          ImGui::Checkbox("Display Game##01", &ThreadPlotStyles[0].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##01", &ThreadPlotStyles[0].LineColor.x);
          ImGui::ColorEdit4("Plot Shade##01", &ThreadPlotStyles[0].ShadeColor.x);
        }
        if (ImGui::CollapsingHeader("Render##11")) {
          ImGui::Checkbox("Display Render##11", &ThreadPlotStyles[1].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##11", &ThreadPlotStyles[1].LineColor.x);
          ImGui::ColorEdit4("Plot Shade##11", &ThreadPlotStyles[1].ShadeColor.x);
        }
        if (ImGui::CollapsingHeader("GPU##21")) {
          ImGui::Checkbox("Display GPU##21", &ThreadPlotStyles[2].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##21", &ThreadPlotStyles[2].LineColor.x);
          ImGui::ColorEdit4("Plot Shade##21", &ThreadPlotStyles[2].ShadeColor.x);
        }
        if (ImGui::CollapsingHeader("RHI##31")) {
          ImGui::Checkbox("Display RHI##31", &ThreadPlotStyles[3].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##31", &ThreadPlotStyles[3].LineColor.x);
          ImGui::ColorEdit4("Plot Shade##31", &ThreadPlotStyles[3].ShadeColor.x);
        }
        if (ImGui::CollapsingHeader("Swap##41")) {
          ImGui::Checkbox("Display Swap##41", &ThreadPlotStyles[4].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##41", &ThreadPlotStyles[4].LineColor.x);
          ImGui::ColorEdit4("Plot Shade##41", &ThreadPlotStyles[4].ShadeColor.x);
        }
        if (ImGui::CollapsingHeader("Input##51")) {
          ImGui::Checkbox("Display Input##51", &ThreadPlotStyles[5].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##51", &ThreadPlotStyles[5].LineColor.x);
          ImGui::ColorEdit4("Plot Shade##51", &ThreadPlotStyles[5].ShadeColor.x);
        }
        if (ImGui::CollapsingHeader("ImGui##61")) {
          ImGui::Checkbox("Display ImGui##61", &ThreadPlotStyles[6].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##61", &ThreadPlotStyles[6].LineColor.x);
          ImGui::ColorEdit4("Plot Shade##61", &ThreadPlotStyles[6].ShadeColor.x);
        }
        ImGui::SliderFloat("Marker Line##1", &ThreadPlotConfig.MarkerLineWidth, 1.0f, 144.0f, "%.3f");
        ImGui::SliderFloat("Marker Thickness##1", &ThreadPlotConfig.MarkerThickness, 1.0f, 10.0f, "%.0f");
      }
      if (ImGui::CollapsingHeader("Frame")) {
        ImGui::Checkbox("Display Frame", &FramePlotConfig.bVisible);
        ImGui::SliderFloat("History##2", &FramePlotConfig.HistoryDuration, 0.1f, GlobalStatHistoryDuration, "%.1f s");
        ImGui::SliderFloat("Position X##2", &FramePlotConfig.Position.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Position Y##2", &FramePlotConfig.Position.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size X##2", &FramePlotConfig.Size.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size Y##2", &FramePlotConfig.Size.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Range Min##2", &FramePlotConfig.Range.x, 0.1f, 60.0f, "%.3f ms");
        ImGui::SliderFloat("Range Max##2", &FramePlotConfig.Range.y, 0.1f, 60.0f, "%.3f ms");
        ImGui::ColorEdit4("Background##2", &FramePlotConfig.BackgroundColor.x);
        ImGui::SliderFloat("Plot Alpha##2", &FramePlotConfig.FillAlpha, 0.0f, 1.0f, "%.2f");
        ImGui::ColorEdit4("Plot Background##2", &FramePlotConfig.PlotBackgroundColor.x);
        ImGui::ColorEdit4("Plot Line##2", &FramePlotConfig.LineColor.x);
        ImGui::ColorEdit4("Plot Shade##2", &FramePlotConfig.ShadeColor.x);
        ImGui::SliderFloat("Marker Line##2", &FramePlotConfig.MarkerLineWidth, 1.0f, 144.0f, "%.3f");
        ImGui::SliderFloat("Marker Thickness##2", &FramePlotConfig.MarkerThickness, 1.0f, 10.0f, "%.0f");
      }
      if (ImGui::CollapsingHeader("FPS")) {
        ImGui::Checkbox("Display FPS", &FPSPlotConfig.bVisible);
        ImGui::SliderFloat("History##3", &FPSPlotConfig.HistoryDuration, 0.1f, GlobalStatHistoryDuration, "%.1f s");
        ImGui::SliderFloat("Position X##3", &FPSPlotConfig.Position.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Position Y##3", &FPSPlotConfig.Position.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size X##3", &FPSPlotConfig.Size.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size Y##3", &FPSPlotConfig.Size.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Range Min##3", &FPSPlotConfig.Range.x, 0.1f, 220.0f, "%.3f ms");
        ImGui::SliderFloat("Range Max##3", &FPSPlotConfig.Range.y, 0.1f, 240.0f, "%.3f ms");
        ImGui::ColorEdit4("Background##3", &FPSPlotConfig.BackgroundColor.x);
        ImGui::SliderFloat("Plot Alpha##3", &FPSPlotConfig.FillAlpha, 0.0f, 1.0f, "%.2f");
        ImGui::ColorEdit4("Plot Background##3", &FPSPlotConfig.PlotBackgroundColor.x);
        ImGui::ColorEdit4("Plot Line##3", &FPSPlotConfig.LineColor.x);
        ImGui::ColorEdit4("Plot Shade##3", &FPSPlotConfig.ShadeColor.x);
        ImGui::SliderFloat("Marker Line##3", &FPSPlotConfig.MarkerLineWidth, 1.0f, 144.0f, "%.3f");
        ImGui::SliderFloat("Marker Thickness##3", &FPSPlotConfig.MarkerThickness, 1.0f, 10.0f, "%.0f");
      }
      ImGui::Unindent();
    }
  }

  if (ImGui::CollapsingHeader("Advanced")) {
    //ImGui::Checkbox("Use external window", &bExternalWindow);
    //ImGui::Checkbox("Disable in-game controls", &bDisableGameControls);
    ImGui::Checkbox("Show Debug Tab", &bShowDebugTab);
    if (ImGui::Button("Reset DFoundryFX")) {
      ImGui::ClearIniSettings();
      //bIsDefaultLoaded = false;
      LoadDefaultValues(ViewportClient);
    }
    if (ImGui::Button("Reset ImGui Settings")) {
      ImGui::ClearIniSettings();
    }
  }
}

void FDFX_StatData::RenderDebugTab() {
  char EnabledStats[32];
  char Hitches[32];

  snprintf(EnabledStats, 32, "Enabled Stats : %i", ViewportClient->GetEnabledStats()->Num());
  snprintf(Hitches, 32, "Hitches : %i", ViewportClient->GetStatHitchesData()->Count);

  ImGui::Text("FPS : %2d", FPS);
  ImGui::Text("Frame : %4.3f", FrameTime);
  ImGui::Text("Game : %4.3f", GameThreadTime);
  ImGui::Text("Render : %4.3f", RenderThreadTime);
  ImGui::Text("GPU : %4.3f", GPUFrameTime);
  ImGui::Text("RHI : %4.3f", RHIThreadTime);
  ImGui::Text("Swap : %4.3f", SwapBufferTime);
  ImGui::Text("Input : %4.3f", InputLatencyTime);
  ImGui::Text("ImGui : %4.3f", ImGuiThreadTime);

  if (ImGui::CollapsingHeader(Hitches)) {
    ImGui::Indent();
    ImGui::Text("Last Time : %4.3f", ViewportClient->GetStatHitchesData()->LastTime);
    for (int32 i = 0; i < ViewportClient->GetStatHitchesData()->Count; ++i) {
      ImGui::Text("Hitch (ms): %4.3f##%i", ViewportClient->GetStatHitchesData()->Hitches[i], i);
    }
    ImGui::Unindent();
  }

  if (ImGui::CollapsingHeader(EnabledStats)) {
    ImGui::Indent();
    FString JoinedStr;
    for (const FString& Stat : *ViewportClient->GetEnabledStats()) {
      JoinedStr += Stat + TEXT("\n");
    }
    ImGui::Text(TCHAR_TO_ANSI(*JoinedStr));
    ImGui::Unindent();
  }
  ImGui::Text("Frame Count: %d", FrameCount);
  ImGui::Text("ImPlot Frame Count: %i", static_cast<int32>(ImPlotFrameCount));
  ImGui::Text("Current Time: %f", CurrentTime);
  ImGui::Text("Last Time: %f", LastTime);
  ImGui::Text("Delta Time: %f", DeltaTime);
  ImGui::Text("FPS float: %f", 1000.0f / DeltaTime);
}

void FDFX_StatData::LoadDefaultValues(UGameViewportClient* InViewportClient) {
  SCOPE_CYCLE_COUNTER(STAT_StatLoadDefault);

  ViewportClient = InViewportClient;
  ViewportClient->GetViewportSize(ViewportSize);

  ThreadPlotConfig.bVisible = true;
  ThreadPlotConfig.HistoryDuration = 3.0f;
  ThreadPlotConfig.Range = ImVec2(0, 20);
  ThreadPlotConfig.Position = ImVec2(0, 0);
  ThreadPlotConfig.Size = ImVec2(ViewportSize.X / 4, ViewportSize.Y / 3);
  ThreadPlotConfig.BackgroundColor = ImVec4(0.21f, 0.22f, 0.23f, 0.05f);
  ThreadPlotConfig.PlotBackgroundColor = ImVec4(0.32f, 0.50f, 0.77f, 0.05f);
  ThreadPlotConfig.FillAlpha = 0.5f;

  ThreadPlotStyles[0].bShowFramePlot = true;
  ThreadPlotStyles[0].LineColor = ImVec4(0.75f, 0.196f, 0.196f, 1.0f);
  ThreadPlotStyles[0].ShadeColor = ImVec4(1.0f, 0.392f, 0.392f, 1.0f);
  ThreadPlotStyles[1].bShowFramePlot = true;
  ThreadPlotStyles[1].LineColor = ImVec4(0.184f, 0.184f, 0.466f, 1.0f);
  ThreadPlotStyles[1].ShadeColor = ImVec4(0.369f, 0.369f, 0.933f, 1.0f);
  ThreadPlotStyles[2].bShowFramePlot = true;
  ThreadPlotStyles[2].LineColor = ImVec4(0.75f, 0.75f, 0.192f, 1.0f);
  ThreadPlotStyles[2].ShadeColor = ImVec4(1.0f, 1.0f, 0.392f, 1.0f);
  ThreadPlotStyles[3].bShowFramePlot = true;
  ThreadPlotStyles[3].LineColor = ImVec4(0.622f, 0.184f, 0.622f, 1.0f);
  ThreadPlotStyles[3].ShadeColor = ImVec4(0.933f, 0.369f, 0.933f, 1.0f);
  ThreadPlotStyles[4].bShowFramePlot = true;
  ThreadPlotStyles[4].LineColor = ImVec4(0.196f, 0.75f, 0.75f, 1.0f);
  ThreadPlotStyles[4].ShadeColor = ImVec4(0.392f, 1.0f, 1.0f, 1.0f);
  ThreadPlotStyles[5].bShowFramePlot = true;
  ThreadPlotStyles[5].LineColor = ImVec4(0.196f, 0.75f, 0.333f, 1.0f);
  ThreadPlotStyles[5].ShadeColor = ImVec4(0.392f, 1.0f, 0.5f, 1.0f);
  ThreadPlotStyles[6].bShowFramePlot = true;
  ThreadPlotStyles[6].LineColor = ImVec4(0.080f, 0.145f, 0.318f, 1.0f);
  ThreadPlotStyles[6].ShadeColor = ImVec4(0.161f, 0.29f, 0.478f, 1.0f);

  ThreadPlotConfig.MarkerColor = ImVec4(0.0f, 0.25f, 0.0f, 1.0f);
  ThreadPlotConfig.MarkerLineWidth = 16.667f;
  ThreadPlotConfig.MarkerThickness = 1.0f;

  FramePlotConfig.bVisible = true;
  FramePlotConfig.HistoryDuration = 3.0f;
  FramePlotConfig.Range = ImVec2(8, 24);
  FramePlotConfig.Position = ImVec2(0, (ViewportSize.Y / 3) * 1);
  FramePlotConfig.Size = ImVec2(ViewportSize.X / 4, ViewportSize.Y / 3);
  FramePlotConfig.BackgroundColor = ImVec4(0.21f, 0.22f, 0.23f, 0.05f);
  FramePlotConfig.PlotBackgroundColor = ImVec4(0.32f, 0.50f, 0.77f, 0.05f);
  FramePlotConfig.FillAlpha = 0.5f;
  FramePlotConfig.LineColor = ImVec4(0.161f, 0.29f, 0.478f, 1.0f);
  FramePlotConfig.ShadeColor = ImVec4(0.298f, 0.447f, 0.69f, 1.0f);
  FramePlotConfig.MarkerColor = ImVec4(0.0f, 0.25f, 0.0f, 1.0f);
  FramePlotConfig.MarkerLineWidth = 16.667f;
  FramePlotConfig.MarkerThickness = 1.0f;

  FPSPlotConfig.bVisible = true;
  FPSPlotConfig.HistoryDuration = 3.0f;
  FPSPlotConfig.Range = ImVec2(20, 80);
  FPSPlotConfig.Position = ImVec2(0, (ViewportSize.Y / 3) * 2);
  FPSPlotConfig.Size = ImVec2(ViewportSize.X, ViewportSize.Y / 3);
  FPSPlotConfig.BackgroundColor = ImVec4(0.21f, 0.22f, 0.23f, 0.05f);
  FPSPlotConfig.PlotBackgroundColor = ImVec4(0.32f, 0.50f, 0.77f, 0.05f);
  FPSPlotConfig.FillAlpha = 0.5f;
  FPSPlotConfig.LineColor = ImVec4(0.161f, 0.29f, 0.478f, 1.0f);
  FPSPlotConfig.ShadeColor = ImVec4(0.298f, 0.447f, 0.69f, 1.0f);
  FPSPlotConfig.MarkerColor = ImVec4(0.0f, 0.25f, 0.0f, 1.0f);
  FPSPlotConfig.MarkerLineWidth = 60.0f;
  FPSPlotConfig.MarkerThickness = 1.0f;

  bShowPlots = true;
  bSortPlots = false;
  bShowDebugTab = true;
  GlobalStatHistoryDuration = 10.0f;

  ImPlotFrameCount = 0.0;
  TimeHistory.Reset();
  FrameCountHistory.Reset();
  FrameTimeHistory.Reset();
  FPSHistory.Reset();
  GameThreadTimeHistory.Reset();
  RenderThreadTimeHistory.Reset();
  GPUFrameTimeHistory.Reset();
  RHIThreadTimeHistory.Reset();
  SwapBufferTimeHistory.Reset();
  InputLatencyTimeHistory.Reset();
  ImGuiThreadTimeHistory.Reset();

  // Pre-loading Engine Tab contents
  //InitEngineContextEntries();
}

void FDFX_StatData::InitEngineContextEntries() {
  Engine_MemoryEntries.Empty();
  Engine_ViewportEntries.Empty();
  Engine_GEngineEntries.Empty();
  Engine_RHIEntries.Empty();
  Engine_PlatformEntries.Empty();

  Engine_MemoryEntries.Emplace(TEXT("Mem"), TFunction<FString()>([]() { return GetMemoryString(FPlatformMemory::GetStats().UsedPhysical); }));
  Engine_MemoryEntries.Emplace(TEXT("MemPeak"), TFunction<FString()>([]() { return GetMemoryString(FPlatformMemory::GetStats().PeakUsedPhysical); }));
  Engine_MemoryEntries.Emplace(TEXT("MemAvail"), TFunction<FString()>([]() { return GetMemoryString(FPlatformMemory::GetStats().AvailablePhysical); }));
  Engine_MemoryEntries.Emplace(TEXT("MemTotal"), TFunction<FString()>([]() { return GetMemoryString(FPlatformMemory::GetStats().TotalPhysical); }));
  Engine_MemoryEntries.Emplace(TEXT("VMem"), TFunction<FString()>([]() { return GetMemoryString(FPlatformMemory::GetStats().UsedVirtual); }));
  Engine_MemoryEntries.Emplace(TEXT("VMemPeak"), TFunction<FString()>([]() { return GetMemoryString(FPlatformMemory::GetStats().PeakUsedVirtual); }));
  Engine_MemoryEntries.Emplace(TEXT("VMemAvail"), TFunction<FString()>([]() { return GetMemoryString(FPlatformMemory::GetStats().AvailableVirtual); }));
  Engine_MemoryEntries.Emplace(TEXT("VMemTotal"), TFunction<FString()>([]() { return GetMemoryString(FPlatformMemory::GetStats().TotalVirtual); }));

  Engine_ViewportEntries.Emplace(TEXT("bIsPlayInEditorViewport"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->bIsPlayInEditorViewport ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("GetDPIScale"), TFunction<FString()>([]() { return FString::Printf(TEXT("%.2f"), ViewportClient->GetDPIScale()); }));
  Engine_ViewportEntries.Emplace(TEXT("GetDPIDerivedResolutionFraction"), TFunction<FString()>([]() { return FString::Printf(TEXT("%.2f"), ViewportClient->GetDPIDerivedResolutionFraction()); }));
  Engine_ViewportEntries.Emplace(TEXT("IsCursorVisible"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsCursorVisible() ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsForegroundWindow"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsForegroundWindow() ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsExclusiveFullscreen"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsExclusiveFullscreen() ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsFullscreen"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsFullscreen() ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsGameRenderingEnabled"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsGameRenderingEnabled() ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsHDRViewport"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsHDRViewport() ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsKeyboardAvailable"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsKeyboardAvailable(0) ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsMouseAvailable"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsMouseAvailable(0) ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsPenActive"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsPenActive() ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsPlayInEditorViewport"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsPlayInEditorViewport() ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsSlateViewport"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsSlateViewport() ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsSoftwareCursorVisible"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsSoftwareCursorVisible() ? TEXT("true") : TEXT("false")); }));
  Engine_ViewportEntries.Emplace(TEXT("IsStereoRenderingAllowed"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), ViewportClient->Viewport->IsStereoRenderingAllowed() ? TEXT("true") : TEXT("false")); }));

  Engine_GEngineEntries.Emplace(TEXT("IsEditor"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->IsEditor() ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("IsAllowedFramerateSmoothing"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->IsAllowedFramerateSmoothing() ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bForceDisableFrameRateSmoothing"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bForceDisableFrameRateSmoothing ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bSmoothFrameRate"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bSmoothFrameRate ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("MinDesiredFrameRate"), TFunction<FString()>([]() { return FString::Printf(TEXT("%.2f"), GEngine->MinDesiredFrameRate); }));
  Engine_GEngineEntries.Emplace(TEXT("bUseFixedFrameRate"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bUseFixedFrameRate ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("FixedFrameRate"), TFunction<FString()>([]() { return FString::Printf(TEXT("%.2f"), GEngine->FixedFrameRate); }));
  Engine_GEngineEntries.Emplace(TEXT("bCanBlueprintsTickByDefault"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bCanBlueprintsTickByDefault ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("IsControllerIdUsingPlatformUserId"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->IsControllerIdUsingPlatformUserId() ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("IsStereoscopic3D"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->IsStereoscopic3D(ViewportClient->Viewport) ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("IsVanillaProduct"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->IsVanillaProduct() ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("HasMultipleLocalPlayers"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->HasMultipleLocalPlayers(ViewportClient->GetWorld()) ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("AreEditorAnalyticsEnabled"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->AreEditorAnalyticsEnabled() ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bAllowMultiThreadedAnimationUpdate"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bAllowMultiThreadedAnimationUpdate ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bDisableAILogging"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bDisableAILogging ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bEnableOnScreenDebugMessages"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bEnableOnScreenDebugMessages ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bEnableOnScreenDebugMessagesDisplay"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bEnableOnScreenDebugMessagesDisplay ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bEnableEditorPSysRealtimeLOD"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bEnableEditorPSysRealtimeLOD ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bEnableVisualLogRecordingOnStart"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bEnableVisualLogRecordingOnStart ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bGenerateDefaultTimecode"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bGenerateDefaultTimecode ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bIsInitialized"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bIsInitialized ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bLockReadOnlyLevels"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bLockReadOnlyLevels ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bOptimizeAnimBlueprintMemberVariableAccess"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bOptimizeAnimBlueprintMemberVariableAccess ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bPauseOnLossOfFocus"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bPauseOnLossOfFocus ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bRenderLightMapDensityGrayscale"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bRenderLightMapDensityGrayscale ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bShouldGenerateLowQualityLightmaps_DEPRECATED"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bShouldGenerateLowQualityLightmaps_DEPRECATED ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("BSPSelectionHighlightIntensity"), TFunction<FString()>([]() { return FString::Printf(TEXT("%.2f"), GEngine->BSPSelectionHighlightIntensity); }));
  Engine_GEngineEntries.Emplace(TEXT("bStartedLoadMapMovie"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bStartedLoadMapMovie ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bSubtitlesEnabled"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bSubtitlesEnabled ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bSubtitlesForcedOff"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bSubtitlesForcedOff ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("bSuppressMapWarnings"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->bSuppressMapWarnings ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("DisplayGamma"), TFunction<FString()>([]() { return FString::Printf(TEXT("%.2f"), GEngine->DisplayGamma); }));
  Engine_GEngineEntries.Emplace(TEXT("IsAutosaving"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->IsAutosaving() ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("MaximumLoopIterationCount"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GEngine->MaximumLoopIterationCount); }));
  Engine_GEngineEntries.Emplace(TEXT("MaxLightMapDensity"), TFunction<FString()>([]() { return FString::Printf(TEXT("%.2f"), GEngine->MaxLightMapDensity); }));
  Engine_GEngineEntries.Emplace(TEXT("MaxOcclusionPixelsFraction"), TFunction<FString()>([]() { return FString::Printf(TEXT("%.2f"), GEngine->MaxOcclusionPixelsFraction); }));
  Engine_GEngineEntries.Emplace(TEXT("MaxParticleResize"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GEngine->MaxParticleResize); }));
  Engine_GEngineEntries.Emplace(TEXT("MaxParticleResizeWarn"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GEngine->MaxParticleResizeWarn); }));
  Engine_GEngineEntries.Emplace(TEXT("MaxPixelShaderAdditiveComplexityCount"), TFunction<FString()>([]() { return FString::Printf(TEXT("%.2f"), GEngine->MaxPixelShaderAdditiveComplexityCount); }));
  Engine_GEngineEntries.Emplace(TEXT("UseSkeletalMeshMinLODPerQualityLevels"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->UseSkeletalMeshMinLODPerQualityLevels ? TEXT("true") : TEXT("false")); }));
  Engine_GEngineEntries.Emplace(TEXT("UseStaticMeshMinLODPerQualityLevels"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GEngine->UseStaticMeshMinLODPerQualityLevels ? TEXT("true") : TEXT("false")); }));

  Engine_RHIEntries.Emplace(TEXT("GRHIAdapterDriverDate"), TFunction<FString()>([]() { return FString(GRHIAdapterDriverDate); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIAdapterDriverOnDenyList"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHIAdapterDriverOnDenyList ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIAdapterInternalDriverVersion"), TFunction<FString()>([]() { return FString(GRHIAdapterInternalDriverVersion); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIAdapterName"), TFunction<FString()>([]() { return FString(GRHIAdapterName); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIAdapterUserDriverVersion"), TFunction<FString()>([]() { return FString(GRHIAdapterUserDriverVersion); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIAttachmentVariableRateShadingEnabled"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsAttachmentVariableRateShading ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIDeviceId"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GRHIDeviceId); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIDeviceIsAMDPreGCNArchitecture"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHIDeviceIsAMDPreGCNArchitecture ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIDeviceIsIntegrated"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHIDeviceIsIntegrated ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIDeviceRevision"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GRHIDeviceRevision); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIForceNoDeletionLatencyForStreamingTextures"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHIForceNoDeletionLatencyForStreamingTextures ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIIsHDREnabled"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHIIsHDREnabled ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHILazyShaderCodeLoading"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHILazyShaderCodeLoading ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIMinimumWaveSize"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GRHIMinimumWaveSize); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIMaximumWaveSize"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GRHIMaximumWaveSize); }));
  Engine_RHIEntries.Emplace(TEXT("GRHINeedsExtraDeletionLatency"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHINeedsExtraDeletionLatency ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHINeedsUnatlasedCSMDepthsWorkaround"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHINeedsUnatlasedCSMDepthsWorkaround ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIPersistentThreadGroupCount"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GRHIPersistentThreadGroupCount); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIPresentCounter"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GRHIPresentCounter); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIRayTracingAccelerationStructureAlignment"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GRHIRayTracingAccelerationStructureAlignment); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIRayTracingScratchBufferAlignment"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GRHIRayTracingScratchBufferAlignment); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIRequiresRenderTargetForPixelShaderUAVs"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHIRequiresRenderTargetForPixelShaderUAVs ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsArrayIndexFromAnyShader"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsArrayIndexFromAnyShader ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsAsyncPipelinePrecompile"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsAsyncPipelinePrecompile ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsAsyncTextureCreation"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsAsyncTextureCreation ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsAtomicUInt64"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsAtomicUInt64 ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsAttachmentVariableRateShading"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsAttachmentVariableRateShading ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsBackBufferWithCustomDepthStencil"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsBackBufferWithCustomDepthStencil ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsBaseVertexIndex"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsBaseVertexIndex ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsComplexVariableRateShadingCombinerOps"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsComplexVariableRateShadingCombinerOps ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsConservativeRasterization"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsConservativeRasterization ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsDepthUAV"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsDepthUAV ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsDirectGPUMemoryLock"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsDirectGPUMemoryLock ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsDrawIndirect"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsDrawIndirect ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsDX12AtomicUInt64"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsDX12AtomicUInt64 ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsDynamicResolution"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsDynamicResolution ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsEfficientUploadOnResourceCreation"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsEfficientUploadOnResourceCreation ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsExactOcclusionQueries"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsExactOcclusionQueries ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsExplicitFMask"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsExplicitFMask ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsExplicitHTile"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsExplicitHTile ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsFirstInstance"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsFirstInstance ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsFrameCyclesBubblesRemoval"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsFrameCyclesBubblesRemoval ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsGPUTimestampBubblesRemoval"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsGPUTimestampBubblesRemoval ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsHDROutput"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsHDROutput ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsInlineRayTracing"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsInlineRayTracing ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsLargerVariableRateShadingSizes"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsLargerVariableRateShadingSizes ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsLateVariableRateShadingUpdate"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsLateVariableRateShadingUpdate ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsLazyShaderCodeLoading"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsLazyShaderCodeLoading ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsMapWriteNoOverwrite"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsMapWriteNoOverwrite ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsMeshShadersTier0"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsMeshShadersTier0 ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsMeshShadersTier1"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsMeshShadersTier1 ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsMSAADepthSampleAccess"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsMSAADepthSampleAccess ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsMultithreadedResources"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsMultithreadedResources ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsMultithreadedShaderCreation"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsMultithreadedShaderCreation ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsMultithreading"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsMultithreading ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsParallelRHIExecute"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsParallelRHIExecute ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsPipelineFileCache"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsPipelineFileCache ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsPipelineStateSortKey"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsPipelineStateSortKey ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsPipelineVariableRateShading"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsPipelineVariableRateShading ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsPixelShaderUAVs"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsPixelShaderUAVs ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsPrimitiveShaders"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsPrimitiveShaders ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsQuadTopology"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsQuadTopology ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsRawViewsForAnyBuffer"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsRawViewsForAnyBuffer ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsRayTracing"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsRayTracing ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsRayTracingAMDHitToken"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsRayTracingAMDHitToken ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsRayTracingAsyncBuildAccelerationStructure"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsRayTracingAsyncBuildAccelerationStructure ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsRayTracingDispatchIndirect"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsRayTracingDispatchIndirect ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsRayTracingPSOAdditions"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsRayTracingPSOAdditions ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsRayTracingShaders"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsRayTracingShaders ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsRectTopology"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsRectTopology ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsResummarizeHTile"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsResummarizeHTile ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsRHIOnTaskThread"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsRHIOnTaskThread ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsRHIThread"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsRHIThread ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsRWTextureBuffers"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsRWTextureBuffers ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsSeparateDepthStencilCopyAccess"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsSeparateDepthStencilCopyAccess ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsShaderTimestamp"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsShaderTimestamp ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsStencilRefFromPixelShader"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsStencilRefFromPixelShader ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsTextureStreaming"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsTextureStreaming ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsUAVFormatAliasing"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsUAVFormatAliasing ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsUpdateFromBufferTexture"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsUpdateFromBufferTexture ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsVariableRateShadingAttachmentArrayTextures"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsVariableRateShadingAttachmentArrayTextures ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHISupportsWaveOperations"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHISupportsWaveOperations ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIThreadNeedsKicking"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHIThreadNeedsKicking ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIThreadTime"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GRHIThreadTime); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIValidationEnabled"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GRHIValidationEnabled ? TEXT("true") : TEXT("false")); }));
//  Engine_RHIEntries.Emplace(TEXT("GRHIVariableRateShadingEnabled"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), GVRSImageManager.IsAttachmentVRSEnabled() ? TEXT("true") : TEXT("false")); }));
  Engine_RHIEntries.Emplace(TEXT("GRHIVendorId"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), GRHIVendorId); }));

  Engine_PlatformEntries.Emplace(TEXT("AllowAudioThread"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::AllowAudioThread() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("AllowLocalCaching"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::AllowLocalCaching() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("AllowThreadHeartBeat"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::AllowThreadHeartBeat() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("CloudDir"), TFunction<FString()>([]() { return FGenericPlatformMisc::CloudDir(); }));
  Engine_PlatformEntries.Emplace(TEXT("DesktopTouchScreen"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::DesktopTouchScreen() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("EngineDir"), TFunction<FString()>([]() { return FGenericPlatformMisc::EngineDir(); }));
  Engine_PlatformEntries.Emplace(TEXT("FullscreenSameAsWindowedFullscreen"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::FullscreenSameAsWindowedFullscreen() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("GamePersistentDownloadDir"), TFunction<FString()>([]() { return FGenericPlatformMisc::GamePersistentDownloadDir(); }));
  Engine_PlatformEntries.Emplace(TEXT("GameTemporaryDownloadDir"), TFunction<FString()>([]() { return FGenericPlatformMisc::GameTemporaryDownloadDir(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetBatteryLevel"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::GetBatteryLevel()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetBrightness"), TFunction<FString()>([]() { return FString::Printf(TEXT("%.2f"), FGenericPlatformMisc::GetBrightness()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetCPUBrand"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetCPUBrand(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetCPUChipset"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetCPUChipset(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetCPUInfo"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::GetCPUInfo()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetCPUVendor"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetCPUVendor(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetDefaultDeviceProfileName"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetDefaultDeviceProfileName(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetDefaultLanguage"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetDefaultLanguage(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetDefaultLocale"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetDefaultLocale(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetDefaultPathSeparator"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::GetDefaultPathSeparator()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetDeviceId"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetDeviceId(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetDeviceMakeAndModel"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetDeviceMakeAndModel(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetDeviceTemperatureLevel"), TFunction<FString()>([]() { return FString::Printf(TEXT("%.2f"), FGenericPlatformMisc::GetDeviceTemperatureLevel()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetDeviceVolume"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::GetDeviceVolume()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetEngineMode"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::GetEngineMode()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetEpicAccountId"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetEpicAccountId(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetFileManagerName"), TFunction<FString()>([]() {  return FGenericPlatformMisc::GetFileManagerName().ToString(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetLastError"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::GetLastError()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetLocalCurrencyCode"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetLocalCurrencyCode(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetLocalCurrencySymbol"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetLocalCurrencySymbol(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetLoginId"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetLoginId(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetMaxPathLength"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::GetMaxPathLength()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetMaxRefreshRate"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::GetMaxRefreshRate()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetMaxSupportedRefreshRate"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::GetMaxSupportedRefreshRate()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetMaxSyncInterval"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::GetMaxSyncInterval()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetMobilePropagateAlphaSetting"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::GetMobilePropagateAlphaSetting() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("GetNullRHIShaderFormat"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::GetNullRHIShaderFormat()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetOperatingSystemId"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), *FGenericPlatformMisc::GetOperatingSystemId()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetOSVersion"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetOSVersion(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetPathVarDelimiter"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::GetPathVarDelimiter()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetPlatformFeaturesModuleName"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::GetPlatformFeaturesModuleName()); }));
  Engine_PlatformEntries.Emplace(TEXT("GetPrimaryGPUBrand"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetPrimaryGPUBrand(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetTimeZoneId"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetTimeZoneId(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetUBTPlatform"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetUBTPlatform(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetUniqueAdvertisingId"), TFunction<FString()>([]() { return FGenericPlatformMisc::GetUniqueAdvertisingId(); }));
  Engine_PlatformEntries.Emplace(TEXT("GetUseVirtualJoysticks"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::GetUseVirtualJoysticks() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("GetVolumeButtonsHandledBySystem"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::GetVolumeButtonsHandledBySystem() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("HasActiveWiFiConnection"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::HasActiveWiFiConnection() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("HasMemoryWarningHandler"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::HasMemoryWarningHandler() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("HasNonoptionalCPUFeatures"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::HasNonoptionalCPUFeatures() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("HasProjectPersistentDownloadDir"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::HasProjectPersistentDownloadDir() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("HasSeparateChannelForDebugOutput"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::HasSeparateChannelForDebugOutput() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("HasVariableHardware"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::HasVariableHardware() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("Is64bitOperatingSystem"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::Is64bitOperatingSystem() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("IsDebuggerPresent"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::IsDebuggerPresent() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("IsEnsureAllowed"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::IsEnsureAllowed() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("IsInLowPowerMode"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::IsInLowPowerMode() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("IsLocalPrintThreadSafe"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::IsLocalPrintThreadSafe() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("IsPackagedForDistribution"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::IsPackagedForDistribution() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("IsPGOEnabled"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::IsPGOEnabled() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("IsRegisteredForRemoteNotifications"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::IsRegisteredForRemoteNotifications() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("IsRemoteSession"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::IsRemoteSession() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("IsRunningInCloud"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::IsRunningInCloud() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("IsRunningOnBattery"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::IsRunningOnBattery() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("LaunchDir"), TFunction<FString()>([]() { return FGenericPlatformMisc::LaunchDir(); }));
  Engine_PlatformEntries.Emplace(TEXT("NeedsNonoptionalCPUFeaturesCheck"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::NeedsNonoptionalCPUFeaturesCheck() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("NumberOfCores"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::NumberOfCores()); }));
  Engine_PlatformEntries.Emplace(TEXT("NumberOfCoresIncludingHyperthreads"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::NumberOfCoresIncludingHyperthreads()); }));
  Engine_PlatformEntries.Emplace(TEXT("NumberOfIOWorkerThreadsToSpawn"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::NumberOfIOWorkerThreadsToSpawn()); }));
  Engine_PlatformEntries.Emplace(TEXT("NumberOfWorkerThreadsToSpawn"), TFunction<FString()>([]() { return FString::Printf(TEXT("%d"), FGenericPlatformMisc::NumberOfWorkerThreadsToSpawn()); }));
  Engine_PlatformEntries.Emplace(TEXT("ProjectDir"), TFunction<FString()>([]() { return FGenericPlatformMisc::ProjectDir(); }));
  Engine_PlatformEntries.Emplace(TEXT("SupportsBackbufferSampling"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::SupportsBackbufferSampling() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("SupportsBrightness"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::SupportsBrightness() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("SupportsDeviceCheckToken"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::SupportsDeviceCheckToken() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("SupportsForceTouchInput"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::SupportsForceTouchInput() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("SupportsFullCrashDumps"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::SupportsFullCrashDumps() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("SupportsLocalCaching"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::SupportsLocalCaching() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("SupportsMessaging"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::SupportsMessaging() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("SupportsMultithreadedFileHandles"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::SupportsMultithreadedFileHandles() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("SupportsTouchInput"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::SupportsTouchInput() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("UseHDRByDefault"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::UseHDRByDefault() ? TEXT("true") : TEXT("false")); }));
  Engine_PlatformEntries.Emplace(TEXT("UseRenderThread"), TFunction<FString()>([]() { return FString::Printf(TEXT("%s"), FGenericPlatformMisc::UseRenderThread() ? TEXT("true") : TEXT("false")); }));
}

void FDFX_StatData::LoadThreadPlotConfig() {
  ImGui::SetNextWindowPos(ThreadPlotConfig.Position);
  ImGui::SetNextWindowSize(ThreadPlotConfig.Size);
  ImGui::SetNextWindowBgAlpha(ThreadPlotConfig.BackgroundColor.w);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ThreadPlotConfig.BackgroundColor);
  ImGui::Begin("Threads", nullptr, MainWindowFlags);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(100, 75));
  ImPlot::PushStyleVar(ImPlotStyleVar_LegendPadding, ImVec2(0, 0));
  ImPlot::PushStyleColor(ImPlotCol_PlotBg, ThreadPlotConfig.PlotBackgroundColor);
  ImPlot::BeginPlot("THREADS (MS)", ImVec2(-1, -1), PlotFlags | ImPlotFlags_NoLegend);
  ImPlot::SetupAxes("Threads", "", AxisFlags, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoLabel);
  ImPlot::SetupAxisLimits(ImAxis_X1, CurrentTime - ThreadPlotConfig.HistoryDuration, CurrentTime, ImGuiCond_Always);
  ImPlot::SetupAxisLimits(ImAxis_Y1, ThreadPlotConfig.Range.x, ThreadPlotConfig.Range.y, ImGuiCond_Always);
  ImPlot::SetupFinish();
  ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, ThreadPlotConfig.FillAlpha);

  TArray<bool> ThreadDrawOrder;
  TArray<float> ThreadPlotOrder;
  ThreadDrawOrder.Init(false, 7);
  ThreadPlotOrder.SetNumZeroed(7);

  if (bSortPlots) {
    ThreadPlotOrder[0] = GameThreadTime;
    ThreadPlotOrder[1] = RenderThreadTime;
    ThreadPlotOrder[2] = GPUFrameTime;
    ThreadPlotOrder[3] = RHIThreadTime;
    ThreadPlotOrder[4] = SwapBufferTime;
    ThreadPlotOrder[5] = InputLatencyTime;
    ThreadPlotOrder[6] = ImGuiThreadTime;
    ThreadPlotOrder.Sort();
  } else {
    ThreadPlotOrder[0] = ImGuiThreadTime;
    ThreadPlotOrder[1] = InputLatencyTime;
    ThreadPlotOrder[2] = SwapBufferTime;
    ThreadPlotOrder[3] = RHIThreadTime;
    ThreadPlotOrder[4] = GPUFrameTime;
    ThreadPlotOrder[5] = RenderThreadTime;
    ThreadPlotOrder[6] = GameThreadTime;
  }

  for (int32 i = 6; i >= 0; --i) {
    if (ThreadPlotOrder[i] == GameThreadTime && !ThreadDrawOrder[0]) {
      if (ThreadPlotStyles[0].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, ThreadPlotStyles[0].LineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, ThreadPlotStyles[0].ShadeColor);
        ImPlot::PlotLine("Game", &TimeHistory.Data[0], &GameThreadTimeHistory.Data[0], TimeHistory.Data.Num(), LineFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PlotShaded("Game", &TimeHistory.Data[0], &GameThreadTimeHistory.Data[0], TimeHistory.Data.Num(), -INFINITY, ShadeFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[0] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == RenderThreadTime && !ThreadDrawOrder[1]) {
      if (ThreadPlotStyles[1].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, ThreadPlotStyles[1].LineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, ThreadPlotStyles[1].ShadeColor);
        ImPlot::PlotLine("Render", &TimeHistory.Data[0], &RenderThreadTimeHistory.Data[0], TimeHistory.Data.Num(), LineFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PlotShaded("Render", &TimeHistory.Data[0], &RenderThreadTimeHistory.Data[0], TimeHistory.Data.Num(), -INFINITY, ShadeFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[1] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == GPUFrameTime && !ThreadDrawOrder[2]) {
      if (ThreadPlotStyles[2].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, ThreadPlotStyles[2].LineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, ThreadPlotStyles[2].ShadeColor);
        ImPlot::PlotLine("GPU", &TimeHistory.Data[0], &GPUFrameTimeHistory.Data[0], TimeHistory.Data.Num(), LineFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PlotShaded("GPU", &TimeHistory.Data[0], &GPUFrameTimeHistory.Data[0], TimeHistory.Data.Num(), -INFINITY, ShadeFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[2] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == RHIThreadTime && !ThreadDrawOrder[3]) {
      if (ThreadPlotStyles[3].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, ThreadPlotStyles[3].LineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, ThreadPlotStyles[3].ShadeColor);
        ImPlot::PlotLine("RHI", &TimeHistory.Data[0], &RHIThreadTimeHistory.Data[0], TimeHistory.Data.Num(), LineFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PlotShaded("RHI", &TimeHistory.Data[0], &RHIThreadTimeHistory.Data[0], TimeHistory.Data.Num(), -INFINITY, ShadeFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[3] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == SwapBufferTime && !ThreadDrawOrder[4]) {
      if (ThreadPlotStyles[4].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, ThreadPlotStyles[4].LineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, ThreadPlotStyles[4].ShadeColor);
        ImPlot::PlotLine("Swap", &TimeHistory.Data[0], &SwapBufferTimeHistory.Data[0], TimeHistory.Data.Num(), LineFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PlotShaded("Swap", &TimeHistory.Data[0], &SwapBufferTimeHistory.Data[0], TimeHistory.Data.Num(), -INFINITY, ShadeFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[4] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == InputLatencyTime && !ThreadDrawOrder[5]) {
      if (ThreadPlotStyles[5].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, ThreadPlotStyles[5].LineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, ThreadPlotStyles[5].ShadeColor);
        ImPlot::PlotLine("Input", &TimeHistory.Data[0], &InputLatencyTimeHistory.Data[0], TimeHistory.Data.Num(), LineFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PlotShaded("Input", &TimeHistory.Data[0], &InputLatencyTimeHistory.Data[0], TimeHistory.Data.Num(), -INFINITY, ShadeFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[5] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == ImGuiThreadTime && !ThreadDrawOrder[6]) {
      if (ThreadPlotStyles[6].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, ThreadPlotStyles[6].LineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, ThreadPlotStyles[6].ShadeColor);
        ImPlot::PlotLine("ImGui", &TimeHistory.Data[0], &ImGuiThreadTimeHistory.Data[0], TimeHistory.Data.Num(), LineFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PlotShaded("ImGui", &TimeHistory.Data[0], &ImGuiThreadTimeHistory.Data[0], TimeHistory.Data.Num(), -INFINITY, ShadeFlags, TimeHistory.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[6] = true;
      continue;
    }
  }

  double MarkerLine = ThreadPlotConfig.MarkerLineWidth;
  ImPlot::DragLineY(0, &MarkerLine, ThreadPlotConfig.MarkerColor, ThreadPlotConfig.MarkerThickness, DragFlags);
  ImPlot::PopStyleVar();

  ImPlot::EndPlot();
  ImPlot::PopStyleColor();
  ImPlot::PopStyleVar(4);
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(4);

  // Custom Plot Legend
  ImGui::SetNextWindowPos(ImVec2(ThreadPlotConfig.Position.x, ThreadPlotConfig.Position.y + ImGui::CalcTextSize("THREADS").y + 4));
  ImGui::SetNextWindowSize(ImVec2(-1, -1));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
  ImGui::Begin("##ThreadsLegendWin", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoNav |
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoBringToFrontOnFocus );

  ImGui::BeginTable("##tblThreadLegend", 2);
  if (ThreadPlotStyles[0].bShowFramePlot) {
    ImGui::TableNextColumn();
    RenderThreadMarker(0);
    ImGui::TextColored(ThreadPlotStyles[0].ShadeColor, "_ Game");
    ImGui::TableNextColumn();
    ImGui::Text("%4.3f", GameThreadTime);
  }
  if (ThreadPlotStyles[1].bShowFramePlot) {
    ImGui::TableNextColumn();
    RenderThreadMarker(1);
    ImGui::TextColored(ThreadPlotStyles[1].ShadeColor, "_ Render");
    ImGui::TableNextColumn();
    ImGui::Text("%4.3f", RenderThreadTime);
  }
  if (ThreadPlotStyles[2].bShowFramePlot) {
    ImGui::TableNextColumn();
    RenderThreadMarker(2);
    ImGui::TextColored(ThreadPlotStyles[2].ShadeColor, "_ GPU");
    ImGui::TableNextColumn();
    ImGui::Text("%4.3f", GPUFrameTime);
  }
  if (ThreadPlotStyles[3].bShowFramePlot) {
    ImGui::TableNextColumn();
    RenderThreadMarker(3);
    ImGui::TextColored(ThreadPlotStyles[3].ShadeColor, "_ RHI");
    ImGui::TableNextColumn();
    ImGui::Text("%4.3f", RHIThreadTime);
  }
  if (ThreadPlotStyles[4].bShowFramePlot) {
    ImGui::TableNextColumn();
    RenderThreadMarker(4);
    ImGui::TextColored(ThreadPlotStyles[4].ShadeColor, "_ Swap");
    ImGui::TableNextColumn();
    ImGui::Text("%4.3f", SwapBufferTime);
  }
  if (ThreadPlotStyles[5].bShowFramePlot) {
    ImGui::TableNextColumn();
    RenderThreadMarker(5);
    ImGui::TextColored(ThreadPlotStyles[5].ShadeColor, "_ Input");
    ImGui::TableNextColumn();
    ImGui::Text("%4.3f", InputLatencyTime);
  }
  if (ThreadPlotStyles[6].bShowFramePlot) {
    ImGui::TableNextColumn();
    RenderThreadMarker(6);
    ImGui::TextColored(ThreadPlotStyles[6].ShadeColor, "_ ImGui");
    ImGui::TableNextColumn();
    ImGui::Text("%4.3f", ImGuiThreadTime);
  }

  int32 NumDrawCalls = GNumDrawCallsRHI[0];
  ImGui::TableNextColumn();
  ImGui::Text("Draws");
  ImGui::TableNextColumn();
  ImGui::Text("%i", NumDrawCalls);
  ImGui::TableNextColumn();
  ImGui::Text("Triangles");
  int32 NumPrimitives = GNumPrimitivesDrawnRHI[0];
  if (NumPrimitives < 10000) {
    ImGui::TableNextColumn();
    ImGui::Text("%i", NumPrimitives);
  } else {
    ImGui::TableNextColumn();
    ImGui::Text("%.1f K", NumPrimitives / 1000.0f);
  }

  ImGui::EndTable();
  ImGui::End();
  ImGui::PopStyleVar(4);

  ImGui::BringWindowToDisplayFront(ImGui::FindWindowByName("##ThreadsLegendWin"));
}

void FDFX_StatData::LoadFramePlotConfig() {
  ImGui::SetNextWindowPos(FramePlotConfig.Position);
  ImGui::SetNextWindowSize(FramePlotConfig.Size);
  ImGui::SetNextWindowBgAlpha(FramePlotConfig.BackgroundColor.w);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, FramePlotConfig.BackgroundColor);
  ImGui::Begin("Frametime", nullptr, MainWindowFlags);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(100, 75));
  ImPlot::PushStyleVar(ImPlotStyleVar_LegendPadding, ImVec2(0, 0));
  ImPlot::PushStyleColor(ImPlotCol_PlotBg, FramePlotConfig.PlotBackgroundColor);
  ImPlot::BeginPlot("FRAME-TIME (MS)", ImVec2(-1, -1), PlotFlags | ImPlotFlags_NoLegend);
  ImPlot::SetupAxes("", "", AxisFlags, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_Invert);
  ImPlot::SetupAxisLimits(ImAxis_X1, CurrentTime - FramePlotConfig.HistoryDuration, CurrentTime, ImGuiCond_Always);
  ImPlot::SetupAxisLimits(ImAxis_Y1, FramePlotConfig.Range.x, FramePlotConfig.Range.y, ImGuiCond_Always);
  ImPlot::SetupFinish();
  ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, FramePlotConfig.FillAlpha);
  ImPlot::PushStyleColor(ImPlotCol_Line, FramePlotConfig.LineColor);
  ImPlot::PushStyleColor(ImPlotCol_Fill, FramePlotConfig.ShadeColor);
  ImPlot::PlotLine("##Frame", &TimeHistory.Data[0], &FrameTimeHistory.Data[0], TimeHistory.Data.Num(), 0, TimeHistory.Offset, sizeof(double));
  ImPlot::PlotShaded("##Frame", &TimeHistory.Data[0], &FrameTimeHistory.Data[0], TimeHistory.Data.Num(), -INFINITY, 0, TimeHistory.Offset, sizeof(double));
  double MarkerLine = FramePlotConfig.MarkerLineWidth;
  ImPlot::DragLineY(0, &MarkerLine, FramePlotConfig.MarkerColor, FramePlotConfig.MarkerThickness, DragFlags);
  ImPlot::PopStyleColor(2);
  ImPlot::PopStyleVar();
  ImPlot::EndPlot();
  ImPlot::PopStyleColor();
  ImPlot::PopStyleVar(4);
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(4);
}

void FDFX_StatData::LoadFPSPlotConfig() {
  ImGui::SetNextWindowPos(FPSPlotConfig.Position);
  if (bMainWindowExpanded) {

    ImGui::SetNextWindowSize(ImVec2(ViewportSize.X - ViewportSize.X / 4, ViewportSize.Y / 3));
  } else {
    ImGui::SetNextWindowSize(FPSPlotConfig.Size);
  }
  ImGui::SetNextWindowBgAlpha(FPSPlotConfig.PlotBackgroundColor.w);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, FPSPlotConfig.BackgroundColor);
  ImGui::Begin("FPS", nullptr, MainWindowFlags);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(100, 75));
  ImPlot::PushStyleVar(ImPlotStyleVar_LegendPadding, ImVec2(0, 0));
  ImPlot::PushStyleColor(ImPlotCol_PlotBg, FPSPlotConfig.PlotBackgroundColor);
  ImPlot::BeginPlot("FRAME-RATE (FPS)", ImVec2(-1, -1), PlotFlags | ImPlotFlags_NoLegend);
  ImPlot::SetupAxes("", "", AxisFlags, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoLabel);
  ImPlot::SetupAxisLimits(ImAxis_X1, CurrentTime - FPSPlotConfig.HistoryDuration, CurrentTime, ImGuiCond_Always);
  ImPlot::SetupAxisLimits(ImAxis_Y1, FPSPlotConfig.Range.x, FPSPlotConfig.Range.y, ImGuiCond_Always);
  ImPlot::SetupFinish();
  ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, FPSPlotConfig.FillAlpha);
  ImPlot::PushStyleColor(ImPlotCol_Line, FPSPlotConfig.LineColor);
  ImPlot::PushStyleColor(ImPlotCol_Fill, FPSPlotConfig.ShadeColor);
  ImPlot::PlotLine("##FPS", &TimeHistory.Data[0], &FPSHistory.Data[0], TimeHistory.Data.Num(), 0, TimeHistory.Offset, sizeof(double));
  ImPlot::PlotShaded("##FPS", &TimeHistory.Data[0], &FPSHistory.Data[0], TimeHistory.Data.Num(), -INFINITY, 0, TimeHistory.Offset, sizeof(double));
  double MarkerLine = FPSPlotConfig.MarkerLineWidth;
  ImPlot::DragLineY(0, &MarkerLine, FPSPlotConfig.MarkerColor, FPSPlotConfig.MarkerThickness, DragFlags);
  ImPlot::PopStyleColor(2);
  ImPlot::PopStyleVar();
  ImPlot::EndPlot();
  ImPlot::PopStyleColor();
  ImPlot::PopStyleVar(4);
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(4);
}

void FDFX_StatData::AddShaderLog(int32 psoLogType, const FString& ShaderHash, double CompileTime) {
  bool bShaderLogExists = false;
  for (FShaderCompileLog& ShaderLog : ShaderCompileLogs) {
    if (ShaderLog.Hash.Left(40) == ShaderHash.Left(40)) {
      ShaderLog.Type |= psoLogType;
      ShaderLog.Time += CompileTime;
      ShaderLog.Count++;
      bShaderLogExists = true;
      break;
    }
  }
  if (!bShaderLogExists) {
    FShaderCompileLog NewLog;
    NewLog.Type = psoLogType;
    NewLog.Hash = ShaderHash;
    NewLog.Time = CompileTime;
    NewLog.Count = 1;
    ShaderCompileLogs.Add(NewLog);
  }
}

void FDFX_StatData::ToggleStat(const FString& StatName, bool& bValue) {
  bool bStatEnabled = ViewportClient->IsStatEnabled(StatName);
  if ((bValue && !bStatEnabled) || (!bValue && bStatEnabled)) {
    FString Command = TEXT("Stat ") + StatName;
    ViewportClient->ConsoleCommand(Command);
  }
}

void FDFX_StatData::RenderToggleButton(const char* LabelId, bool* Value) {
  ImVec4* Colors = ImGui::GetStyle().Colors;
  ImVec2 CursorPos = ImGui::GetCursorScreenPos();
  ImDrawList* DrawList = ImGui::GetWindowDrawList();

  float Height = ImGui::GetFrameHeight();
  float Width = Height * 1.55f;
  float Radius = Height * 0.50f;
  float Rounding = 0.2f;

  ImGui::InvisibleButton(LabelId, ImVec2(Width, Height));
  if (ImGui::IsItemClicked()) {
    *Value = !*Value;
  }
  ImGuiContext& gg = *GImGui;
  float AnimSpeed = 0.085f;
  float TAnim = 0.0f;
  if (gg.LastActiveId == gg.CurrentWindow->GetID(LabelId)) {
    TAnim = ImSaturate(gg.LastActiveIdTimer / AnimSpeed);
  }
  ImVec4 ButtonColor = *Value ? Colors[ImGuiCol_ButtonActive] : ImVec4(0.78f, 0.78f, 0.78f, 1.0f);
  if (ImGui::IsItemHovered()) {
    DrawList->AddRectFilled(CursorPos, ImVec2(CursorPos.x + Width, CursorPos.y + Height),
      ImGui::GetColorU32(ButtonColor), Height * Rounding);
  } else {
    DrawList->AddRectFilled(CursorPos, ImVec2(CursorPos.x + Width, CursorPos.y + Height),
      ImGui::GetColorU32(*Value ? Colors[ImGuiCol_Button] : ImVec4(0.85f, 0.85f, 0.85f, 1.0f)), Height * Rounding);
  }

  ImVec2 Center = ImVec2(Radius + (*Value ? 1.0f : 0.0f) * (Width - Radius * 2.0f), Radius);
  DrawList->AddRectFilled(ImVec2((CursorPos.x + Center.x) - 9.0f, CursorPos.y + 1.5f),
    ImVec2((CursorPos.x + (Width / 2) + Center.x) - 9.0f, CursorPos.y + Height - 1.5f),
    IM_COL32(255, 255, 255, 255), Height * Rounding);
}

void FDFX_StatData::RenderHelpMarker(const char* Description) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(Description);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

void FDFX_StatData::RenderThreadMarker(int32 PlotColorIndex) {
  ImDrawList* DrawList = ImGui::GetWindowDrawList();
  ImVec2 Pos = ImGui::GetCursorScreenPos();
  ImVec2 MarkerMin = ImVec2(Pos.x, Pos.y - 2);
  ImVec2 MarkerMax = ImVec2(Pos.x + 8, Pos.y + ImGui::GetTextLineHeight() + 2);
  DrawList->AddRect(MarkerMin, MarkerMax, ImColor(ThreadPlotStyles[PlotColorIndex].LineColor));
  DrawList->AddRectFilled(MarkerMin, MarkerMax, ImColor(ThreadPlotStyles[PlotColorIndex].ShadeColor));
}

void FDFX_StatData::RenderStats(EDFXStatCategory InCategory, const FString& Filter) {
  bool LocalToggle = false;

  for (FDFXStatCmd& Elem : FDFX_Module::StatCommands) {
    if (!Filter.IsEmpty() && !Elem.Command.Contains(Filter)) {
      continue;
    }
    if (Elem.Category == InCategory ||
        (Elem.Category == EDFXStatCategory::Favorites && InCategory == EDFXStatCategory::Favorites) ||
        InCategory == EDFXStatCategory::All) {
      const FString FormattedCmd = FString::Printf(TEXT("Stat %s"), *Elem.Command);
      const FString StatId = FString::Printf(TEXT("Stat_%s"), *Elem.Command);
      const char* StatCmdAnsi = TCHAR_TO_UTF8(*FormattedCmd);
      const char* StatIdAnsi = TCHAR_TO_UTF8(*StatId);

      ImGui::TableNextColumn();
      ImGui::Text(StatCmdAnsi);
      LocalToggle = Elem.bEnabled;
      ImGui::TableNextColumn();
      RenderToggleButton(StatIdAnsi, &LocalToggle);
      ToggleStat(Elem.Command, LocalToggle);
      Elem.bEnabled = LocalToggle;
    }
  }
}

template<typename T>
void FDFX_StatData::RenderInfoHelper(const FString& Info, const T& Value) {
  if constexpr (std::is_same_v<T, bool>) {
    bool LocalValue = Value;
    ImGui::Checkbox(TCHAR_TO_ANSI(*Info), &LocalValue);
  } else {
    bool DummyCheckbox = false;
    ImGui::Checkbox("", &DummyCheckbox);
    ImGui::SameLine();

    FString ValueFormatted;
    if constexpr (std::is_same_v<T, FText>) {
      ValueFormatted = Value.ToString();
    } else if constexpr (std::is_same_v<T, FString>) {
      ValueFormatted = Value;
    } else if constexpr (std::is_convertible_v<T, const TCHAR*>) {
      ValueFormatted = FString(Value);
    } else if constexpr (std::is_same_v<T, int32>) {
      ValueFormatted = FString::Printf(TEXT("%d"), Value);
    } else if constexpr (std::is_same_v<T, uint32>) {
      ValueFormatted = FString::Printf(TEXT("%u"), Value);
    } else if constexpr (std::is_same_v<T, uint64>) {
      ValueFormatted = FString::Printf(TEXT("%llu"), Value);
    } else if constexpr (std::is_same_v<T, float>) {
      ValueFormatted = FString::Printf(TEXT("%f"), Value);
    } else if constexpr (std::is_same_v<T, double>) {
      ValueFormatted = FString::Printf(TEXT("%f"), Value);
    } else {
      ValueFormatted = TEXT("Unsupported");
    }

    FString DisplayText = FString::Printf(TEXT("%s - %s"), *Info, *ValueFormatted);
    ImGui::Text(TCHAR_TO_ANSI(*DisplayText));
  }
}