#include "DFoundryFX_StatData.h"
#include "DFoundryFX_Module.h"
#include "GameFramework/GameUserSettings.h"
#include <type_traits>

DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatLoadDefault"), STAT_StatLoadDefault, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatUpdate"), STAT_StatUpdate, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatMainWin"), STAT_StatMainWin, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatPlotThreads"), STAT_StatPlotThreads, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatPlotFrametime"), STAT_StatPlotFrametime, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_StatPlotFPS"), STAT_StatPlotFramerate, STATGROUP_DFoundryFX);

UGameViewportClient* FDFX_StatData::ViewportClient = nullptr;
FVector2D FDFX_StatData::ViewportSize = FVector2D::ZeroVector;
FDisplayMetrics FDFX_StatData::DisplayMetrics;

bool FDFX_StatData::bMainWindowExpanded = true;
bool FDFX_StatData::bHasPendingChanges = false;
bool FDFX_StatData::bShowPlots = true;
bool FDFX_StatData::bSortPlots = true;
bool FDFX_StatData::bShowDebugTab = true;

float FDFX_StatData::GlobalStatHistoryDuration = 3.0f;

FDFX_StatData::FPlotConfig FDFX_StatData::PlotConfigThreads;
FDFX_StatData::FPlotConfig FDFX_StatData::PlotConfigFramerate;
FDFX_StatData::FPlotConfig FDFX_StatData::PlotConfigFrametime;

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

void FDFX_StatData::RunDFoundryFX(uint64 ImGuiThreadTimeMs) {
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
    if (PlotConfigThreads.bVisible) {
      {
        SCOPE_CYCLE_COUNTER(STAT_StatPlotThreads);
        RenderPlotThreads();
      }
    }
    if (PlotConfigFramerate.bVisible) {
      {
        SCOPE_CYCLE_COUNTER(STAT_StatPlotFrametime);
        RenderPlotFrametime();
      }
    }
    if (PlotConfigFrametime.bVisible) {
      {
        SCOPE_CYCLE_COUNTER(STAT_StatPlotFramerate);
        RenderPlotFramerate();
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
#if PLATFORM_WINDOWS
  const APlayerController* PC = ViewportClient->GetWorld()->GetFirstPlayerController();
  bool bShouldDraw = ImGui::IsWindowHovered() && !PC->bShowMouseCursor;
  if (bShouldDraw != ImGui::GetIO().MouseDrawCursor) {
    ImGui::GetIO().MouseDrawCursor = bShouldDraw;
  }
#endif
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
    static bool bVSyncPrev = false;
    static bool bVSync = ConsoleManager.FindConsoleVariable(TEXT("r.VSync"))->GetBool();
    static bool bVSyncEditorPrev = false;
    static bool bVSyncEditor = ConsoleManager.FindConsoleVariable(TEXT("r.VSyncEditor"))->GetBool();
    static bool bFullscreenWindowed = (ViewportClient->IsFullScreenViewport() && !ViewportClient->IsExclusiveFullscreenViewport());
    static bool bFullscreenWindowedPrev = bFullscreenWindowed;
    static bool bFullscreenExclusive = (ViewportClient->IsFullScreenViewport() && ViewportClient->IsExclusiveFullscreenViewport());
    static bool bFullscreenExclusivePrev = bFullscreenExclusive;
    static int32 ScreenResolution[2] = { ViewportSize.X, ViewportSize.Y };
    static int32 ScreenPercentage = []() { int32 Value = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"))->GetInt();
      return Value == 0 ? 100 : Value; }();
    static int32 MaxFPS = []() { int32 Value = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))->GetInt();
      UE_LOG(LogTemp, Warning, TEXT("t.MaxFPS read as %d"), Value);
      return Value == 0 ? 500 : Value; }();
    static int32 ResolutionQuality = ConsoleManager.FindConsoleVariable(TEXT("sg.ResolutionQuality"))->GetInt();
    static int32 ViewDistanceScale = ConsoleManager.FindConsoleVariable(TEXT("r.ViewDistanceScale"))->GetInt();
    static int32 PostProcessQuality = ConsoleManager.FindConsoleVariable(TEXT("sg.PostProcessQuality"))->GetInt();
    static int32 ShadowQuality = ConsoleManager.FindConsoleVariable(TEXT("sg.ShadowQuality"))->GetInt();
    static int32 TextureQuality = ConsoleManager.FindConsoleVariable(TEXT("sg.TextureQuality"))->GetInt();
    static int32 EffectsQuality = ConsoleManager.FindConsoleVariable(TEXT("sg.EffectsQuality"))->GetInt();
    static int32 DetailMode = ConsoleManager.FindConsoleVariable(TEXT("r.DetailMode"))->GetInt();
    static int32 SkeletalMeshLODBias = ConsoleManager.FindConsoleVariable(TEXT("r.SkeletalMeshLODBias"))->GetInt();

    ImGui::BeginTable("##tblWindowSettingsBase", 3, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("VSync");
    ImGui::TableNextColumn();
    if (ImGui::Checkbox("Game", &bVSync)) {
      if (bVSync != bVSyncPrev) {
        ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.VSync %i"), bVSync ? 1 : 0));
      }
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Only work for Packaged Game Builds or Play Stand Alone Game.");
    }
    bVSyncPrev = bVSync;

    ImGui::TableNextColumn();
    if (ImGui::Checkbox("Editor", &bVSyncEditor)) {
      if (bVSyncEditor != bVSyncEditorPrev) {
        ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.VSyncEditor %i"), bVSyncEditor ? 1 : 0));
      }
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Only work while PlayInEditor.");
    }
    bVSyncEditorPrev = bVSyncEditor;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("Fullscreen");
    ImGui::TableNextColumn();
    if (ImGui::Checkbox("Windowed", &bFullscreenWindowed)) {
      if (bFullscreenWindowed != bFullscreenWindowedPrev) {
        if (bFullscreenWindowed) {
          ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%iwf"), DisplayMetrics.PrimaryDisplayWidth, DisplayMetrics.PrimaryDisplayHeight));
        } else {
          ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%iw"), ScreenResolution[0], ScreenResolution[1]));
        }
      }
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Only work for Packaged Game Builds or Play Stand Alone Game.");
    }
    bFullscreenWindowedPrev = bFullscreenWindowed;
    ImGui::TableNextColumn();
    if (ImGui::Checkbox("Exclusive", &bFullscreenExclusive)) {
      if (bFullscreenExclusive != bFullscreenExclusivePrev) {
        if (bFullscreenExclusive) {
          ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%if"), DisplayMetrics.PrimaryDisplayWidth, DisplayMetrics.PrimaryDisplayHeight));
        } else {
          ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%iw"), ScreenResolution[0], ScreenResolution[1]));
        }
      }
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Only work for Packaged Game Builds or Play Stand Alone Game.");
    }
    bFullscreenExclusivePrev = bFullscreenExclusive;
    ImGui::EndTable();

    ImGui::BeginTable("##tblScalabilitySettingsBase", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("Window Size");
    ImGui::TableNextColumn();
    ImGui::InputInt2("##WindowSize", ScreenResolution);
    ImGui::SameLine();
    if (ImGui::Button("Apply##btnVS1")) {
      ViewportClient->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%iw"), ScreenResolution[0], ScreenResolution[1]));
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Only work for Packaged Game Builds or Play Stand Alone Game.");
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("ScreenPercent");
    ImGui::TableNextColumn();
    ImGui::InputInt("##ScreenPercent", &ScreenPercentage, 1, 5);
    ImGui::SameLine();
    if (ImGui::Button("Apply##btnVS3")) {
      ConsoleManager.FindConsoleVariable(TEXT("r.ScreenPercentage"))->Set(ScreenPercentage);
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Only work for Packaged Game Builds or Play Stand Alone Game.");
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("MaxFPS");
    ImGui::TableNextColumn();
    ImGui::InputInt("##MaxFPS", &MaxFPS, 1, 10);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("IF Vsync is off,\nset maximum frames per second."); }
    ImGui::SameLine();
    if (ImGui::Button("Apply##btnVS2")) {
      if (MaxFPS != ConsoleManager.FindConsoleVariable(TEXT("t.MaxFPS"))->GetInt()) {
        ConsoleManager.FindConsoleVariable(TEXT("t.MaxFPS"))->Set(MaxFPS);
      }
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("Res.Quality");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Resolution Quality."); }
    ImGui::TableNextColumn();
    ImGui::InputInt("##ResQuality", &ResolutionQuality, 1, 5);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("0: Low\n1: Medium\n2: High\n3: Epic\n4: Cinematic\n5: Custom"); }
    ImGui::SameLine();
    if (ImGui::Button("Apply##btnVS4")) {
      if (ResolutionQuality != ConsoleManager.FindConsoleVariable(TEXT("sg.ResolutionQuality"))->GetInt()) {
        ConsoleManager.FindConsoleVariable(TEXT("sg.ResolutionQuality"))->Set(ResolutionQuality);
      }
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("View Distance");
    ImGui::TableNextColumn();
    ImGui::InputInt("##ViewDistance", &ViewDistanceScale, 1, 5);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("0: Low\n1: Medium\n2: High\n3: Epic\n4: Cinematic\n5: Custom"); }
    ImGui::SameLine();
    if (ImGui::Button("Apply##btnVS5")) {
      if (ViewDistanceScale != ConsoleManager.FindConsoleVariable(TEXT("r.ViewDistanceScale"))->GetInt()) {
        ConsoleManager.FindConsoleVariable(TEXT("r.ViewDistanceScale"))->Set(ViewDistanceScale);
      }
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("PP Quality");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("PostProcess Quality."); }
    ImGui::TableNextColumn();
    ImGui::InputInt("##PPQuality", &PostProcessQuality, 1, 5);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("0: Low\n1: Medium\n2: High\n3: Epic\n4: Cinematic\n5: Custom"); }
    ImGui::SameLine();
    if (ImGui::Button("Apply##btnVS6")) {
      if (PostProcessQuality != ConsoleManager.FindConsoleVariable(TEXT("sg.PostProcessQuality"))->GetInt()) {
        ConsoleManager.FindConsoleVariable(TEXT("sg.PostProcessQuality"))->Set(PostProcessQuality);
      }
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("Shadow Qual.");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Shadow Quality."); }
    ImGui::TableNextColumn();
    ImGui::InputInt("##ShadowQual", &ShadowQuality, 1, 5);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("0: Low\n1: Medium\n2: High\n3: Epic\n4: Cinematic\n5: Custom"); }
    ImGui::SameLine();
    if (ImGui::Button("Apply##btnVS7")) {
      if (ShadowQuality != ConsoleManager.FindConsoleVariable(TEXT("sg.ShadowQuality"))->GetInt()) {
        ConsoleManager.FindConsoleVariable(TEXT("sg.ShadowQuality"))->Set(ShadowQuality);
      }
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("Texture Qual.");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Texture Quality."); }
    ImGui::TableNextColumn();
    ImGui::InputInt("##TextureQual", &TextureQuality, 1, 5);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("0: Low\n1: Medium\n2: High\n3: Epic\n4: Cinematic\n5: Custom"); }
    ImGui::SameLine();
    if (ImGui::Button("Apply##btnVS8")) {
      if (TextureQuality != ConsoleManager.FindConsoleVariable(TEXT("sg.TextureQuality"))->GetInt()) {
        ConsoleManager.FindConsoleVariable(TEXT("sg.TextureQuality"))->Set(TextureQuality);
      }
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("Effects Qual.");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Effects Quality."); }
    ImGui::TableNextColumn();
    ImGui::InputInt("##EffectsQual", &EffectsQuality, 1, 5);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("0: Low\n1: Medium\n2: High\n3: Epic\n4: Cinematic\n5: Custom"); }
    ImGui::SameLine();
    if (ImGui::Button("Apply##btnVS9")) {
      if (EffectsQuality != ConsoleManager.FindConsoleVariable(TEXT("sg.EffectsQuality"))->GetInt()) {
        ConsoleManager.FindConsoleVariable(TEXT("sg.EffectsQuality"))->Set(EffectsQuality);
      }
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("Detail Mode");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Detail Mode."); }
    ImGui::TableNextColumn();
    ImGui::InputInt("##DetailMode", &DetailMode, 1, 5);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("0: Low\n1: Medium\n2: High\n3: Epic\n4: Cinematic\n5: Custom"); }
    ImGui::SameLine();
    if (ImGui::Button("Apply##btnVS10")) {
      if (DetailMode != ConsoleManager.FindConsoleVariable(TEXT("r.DetailMode"))->GetInt()) {
        ConsoleManager.FindConsoleVariable(TEXT("r.DetailMode"))->Set(DetailMode);
      }
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("Skeletal LOD");
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Skeletal Level Of Detail."); }
    ImGui::TableNextColumn();
    ImGui::InputInt("##SkeletalLOD", &SkeletalMeshLODBias, 1, 5);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("0: Low\n1: Medium\n2: High\n3: Epic\n4: Cinematic\n5: Custom"); }
    ImGui::SameLine();
    if (ImGui::Button("Apply##btnVS11")) {
      if (SkeletalMeshLODBias != ConsoleManager.FindConsoleVariable(TEXT("r.SkeletalMeshLODBias"))->GetInt()) {
        ConsoleManager.FindConsoleVariable(TEXT("r.SkeletalMeshLODBias"))->Set(SkeletalMeshLODBias);
      }
    }
    ImGui::EndTable();
  }

  RenderEngineTab_Data();
  // Platform specific context.
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
      ImGui::Text("%s", StringCast<ANSICHAR>(*ShaderLog.Hash.Left(40)).Get());
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
  if (PlotConfigThreads.HistoryDuration > GlobalStatHistoryDuration) {
    PlotConfigThreads.HistoryDuration = GlobalStatHistoryDuration;
  }
  if (PlotConfigFramerate.HistoryDuration > GlobalStatHistoryDuration) {
    PlotConfigFramerate.HistoryDuration = GlobalStatHistoryDuration;
  }
  if (PlotConfigFrametime.HistoryDuration > GlobalStatHistoryDuration) {
    PlotConfigFrametime.HistoryDuration = GlobalStatHistoryDuration;
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
        ImGui::Checkbox("Display Threads", &PlotConfigThreads.bVisible);
        ImGui::Checkbox("Sort plot order", &bSortPlots);
        ImGui::SameLine();
        RenderHelpMarker("Sort graphs order to display better colors but can cause flickering if two threads have similar values.");
        ImGui::SliderFloat("History##1", &PlotConfigThreads.HistoryDuration, 0.1f, GlobalStatHistoryDuration, "%.1f s");
        ImGui::SliderFloat("Position X##1", &PlotConfigThreads.Position.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Position Y##1", &PlotConfigThreads.Position.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size X##1", &PlotConfigThreads.Size.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size Y##1", &PlotConfigThreads.Size.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Range Min##1", &PlotConfigThreads.Range.x, 0.1f, 60.0f, "%.3f ms");
        ImGui::SliderFloat("Range Max##1", &PlotConfigThreads.Range.y, 0.1f, 60.0f, "%.3f ms");
        ImGui::ColorEdit4("Background##1", &PlotConfigThreads.BackgroundColor.x);
        ImGui::SliderFloat("Plot Alpha##1", &PlotConfigThreads.FillAlpha, 0.0f, 1.0f, "%.2f");
        ImGui::ColorEdit4("Plot Background##1", &PlotConfigThreads.PlotBackgroundColor.x);
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
        ImGui::SliderFloat("Marker Line##1", &PlotConfigThreads.MarkerLineWidth, 1.0f, 144.0f, "%.3f");
        ImGui::SliderFloat("Marker Thickness##1", &PlotConfigThreads.MarkerThickness, 1.0f, 10.0f, "%.0f");
      }
      if (ImGui::CollapsingHeader("Frame")) {
        ImGui::Checkbox("Display Frame", &PlotConfigFramerate.bVisible);
        ImGui::SliderFloat("History##2", &PlotConfigFramerate.HistoryDuration, 0.1f, GlobalStatHistoryDuration, "%.1f s");
        ImGui::SliderFloat("Position X##2", &PlotConfigFramerate.Position.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Position Y##2", &PlotConfigFramerate.Position.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size X##2", &PlotConfigFramerate.Size.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size Y##2", &PlotConfigFramerate.Size.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Range Min##2", &PlotConfigFramerate.Range.x, 0.1f, 60.0f, "%.3f ms");
        ImGui::SliderFloat("Range Max##2", &PlotConfigFramerate.Range.y, 0.1f, 60.0f, "%.3f ms");
        ImGui::ColorEdit4("Background##2", &PlotConfigFramerate.BackgroundColor.x);
        ImGui::SliderFloat("Plot Alpha##2", &PlotConfigFramerate.FillAlpha, 0.0f, 1.0f, "%.2f");
        ImGui::ColorEdit4("Plot Background##2", &PlotConfigFramerate.PlotBackgroundColor.x);
        ImGui::ColorEdit4("Plot Line##2", &PlotConfigFramerate.LineColor.x);
        ImGui::ColorEdit4("Plot Shade##2", &PlotConfigFramerate.ShadeColor.x);
        ImGui::SliderFloat("Marker Line##2", &PlotConfigFramerate.MarkerLineWidth, 1.0f, 144.0f, "%.3f");
        ImGui::SliderFloat("Marker Thickness##2", &PlotConfigFramerate.MarkerThickness, 1.0f, 10.0f, "%.0f");
      }
      if (ImGui::CollapsingHeader("FPS")) {
        ImGui::Checkbox("Display FPS", &PlotConfigFrametime.bVisible);
        ImGui::SliderFloat("History##3", &PlotConfigFrametime.HistoryDuration, 0.1f, GlobalStatHistoryDuration, "%.1f s");
        ImGui::SliderFloat("Position X##3", &PlotConfigFrametime.Position.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Position Y##3", &PlotConfigFrametime.Position.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size X##3", &PlotConfigFrametime.Size.x, 0.0f, ViewportSize.X - 1.0f, "%.0f px");
        ImGui::SliderFloat("Size Y##3", &PlotConfigFrametime.Size.y, 0.0f, ViewportSize.Y - 1.0f, "%.0f px");
        ImGui::SliderFloat("Range Min##3", &PlotConfigFrametime.Range.x, 0.1f, 220.0f, "%.3f ms");
        ImGui::SliderFloat("Range Max##3", &PlotConfigFrametime.Range.y, 0.1f, 240.0f, "%.3f ms");
        ImGui::ColorEdit4("Background##3", &PlotConfigFrametime.BackgroundColor.x);
        ImGui::SliderFloat("Plot Alpha##3", &PlotConfigFrametime.FillAlpha, 0.0f, 1.0f, "%.2f");
        ImGui::ColorEdit4("Plot Background##3", &PlotConfigFrametime.PlotBackgroundColor.x);
        ImGui::ColorEdit4("Plot Line##3", &PlotConfigFrametime.LineColor.x);
        ImGui::ColorEdit4("Plot Shade##3", &PlotConfigFrametime.ShadeColor.x);
        ImGui::SliderFloat("Marker Line##3", &PlotConfigFrametime.MarkerLineWidth, 1.0f, 144.0f, "%.3f");
        ImGui::SliderFloat("Marker Thickness##3", &PlotConfigFrametime.MarkerThickness, 1.0f, 10.0f, "%.0f");
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
    ImGui::Text("%s", StringCast<ANSICHAR>(*JoinedStr).Get());
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
  FDisplayMetrics::RebuildDisplayMetrics(DisplayMetrics);

  // TODO DPIScale is a mess and can be affected by :
  //   Editor Settings > General > Appearance > Enable High DPI Support
  //   Editor Settings > Performance > Viewport Resolution section
  //   Project Settings > Performance > Viewport Resolution section
  //   Project Settings > Engine > User Interface > Allow High DPI in Game Mode
  
  // HACK trying to fix inconsistence of DPI Scale between UE and ImGui
  //if (ViewportClient->GetWorld()->IsPlayInEditor()) {
    IConsoleManager& ConsoleManager = IConsoleManager::Get();
    int32 DPIAware = ConsoleManager.FindConsoleVariable(TEXT("EnableHighDPIAwareness"))->GetInt();
    if (DPIAware == 1) {
      float DPIScale = ViewportClient->GetDPIScale();
      if (DPIScale != 1.f)  {
        ViewportSize.X = ViewportSize.X / DPIScale;
        ViewportSize.Y = ViewportSize.Y / DPIScale;
      }
    }

    UE_LOG(LogDFoundryFX, Log, TEXT("LoadDefaultValues: Resolution: %ix%i | DPIAware: %i | DPIScale : %.2f."), (int)ViewportSize.X, (int)ViewportSize.Y, DPIAware, ViewportClient->GetDPIScale());
    //GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("LoadDefaultValues: Resolution: %ix%i | DPIAware: %i | DPIScale : %.2f."), (int)ViewportSize.X, (int)ViewportSize.Y, DPIAware, ViewportClient->GetDPIScale()));
  //}

  PlotConfigThreads.bVisible = true;
  PlotConfigThreads.HistoryDuration = 3.0f;
  PlotConfigThreads.Range = ImVec2(0, 20);
  PlotConfigThreads.Position = ImVec2(0, 0);
  PlotConfigThreads.Size = ImVec2(ViewportSize.X / 4, ViewportSize.Y / 3);
  PlotConfigThreads.BackgroundColor = ImVec4(0.21f, 0.22f, 0.23f, 0.05f);
  PlotConfigThreads.PlotBackgroundColor = ImVec4(0.32f, 0.50f, 0.77f, 0.05f);
  PlotConfigThreads.FillAlpha = 0.5f;

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

  PlotConfigThreads.MarkerColor = ImVec4(0.0f, 0.25f, 0.0f, 1.0f);
  PlotConfigThreads.MarkerLineWidth = 16.667f;
  PlotConfigThreads.MarkerThickness = 1.0f;

  PlotConfigFramerate.bVisible = true;
  PlotConfigFramerate.HistoryDuration = 3.0f;
  PlotConfigFramerate.Range = ImVec2(8, 24);
  PlotConfigFramerate.Position = ImVec2(0, (ViewportSize.Y / 3) * 1);
  PlotConfigFramerate.Size = ImVec2(ViewportSize.X / 4, ViewportSize.Y / 3);
  PlotConfigFramerate.BackgroundColor = ImVec4(0.21f, 0.22f, 0.23f, 0.05f);
  PlotConfigFramerate.PlotBackgroundColor = ImVec4(0.32f, 0.50f, 0.77f, 0.05f);
  PlotConfigFramerate.FillAlpha = 0.5f;
  PlotConfigFramerate.LineColor = ImVec4(0.161f, 0.29f, 0.478f, 1.0f);
  PlotConfigFramerate.ShadeColor = ImVec4(0.298f, 0.447f, 0.69f, 1.0f);
  PlotConfigFramerate.MarkerColor = ImVec4(0.0f, 0.25f, 0.0f, 1.0f);
  PlotConfigFramerate.MarkerLineWidth = 16.667f;
  PlotConfigFramerate.MarkerThickness = 1.0f;

  PlotConfigFrametime.bVisible = true;
  PlotConfigFrametime.HistoryDuration = 3.0f;
  PlotConfigFrametime.Range = ImVec2(20, 80);
  PlotConfigFrametime.Position = ImVec2(0, (ViewportSize.Y / 3) * 2);
  PlotConfigFrametime.Size = ImVec2(ViewportSize.X, ViewportSize.Y / 3);
  PlotConfigFrametime.BackgroundColor = ImVec4(0.21f, 0.22f, 0.23f, 0.05f);
  PlotConfigFrametime.PlotBackgroundColor = ImVec4(0.32f, 0.50f, 0.77f, 0.05f);
  PlotConfigFrametime.FillAlpha = 0.5f;
  PlotConfigFrametime.LineColor = ImVec4(0.161f, 0.29f, 0.478f, 1.0f);
  PlotConfigFrametime.ShadeColor = ImVec4(0.298f, 0.447f, 0.69f, 1.0f);
  PlotConfigFrametime.MarkerColor = ImVec4(0.0f, 0.25f, 0.0f, 1.0f);
  PlotConfigFrametime.MarkerLineWidth = 60.0f;
  PlotConfigFrametime.MarkerThickness = 1.0f;

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
}

void FDFX_StatData::RenderPlotThreads() {
  ImGui::SetNextWindowPos(PlotConfigThreads.Position);
  ImGui::SetNextWindowSize(PlotConfigThreads.Size);
  RenderPlotStyleBegin(EPlotType::Threads);
  ImPlot::BeginPlot("THREADS (MS)", ImVec2(-1, -1), PlotFlags | ImPlotFlags_NoLegend);
  ImPlot::SetupAxes("Threads", "", AxisFlags, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoLabel);
  ImPlot::SetupAxisLimits(ImAxis_X1, CurrentTime - PlotConfigThreads.HistoryDuration, CurrentTime, ImGuiCond_Always);
  ImPlot::SetupAxisLimits(ImAxis_Y1, PlotConfigThreads.Range.x, PlotConfigThreads.Range.y, ImGuiCond_Always);
  ImPlot::SetupFinish();
  ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, PlotConfigThreads.FillAlpha);

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

  double MarkerLine = PlotConfigThreads.MarkerLineWidth;
  ImPlot::DragLineY(0, &MarkerLine, PlotConfigThreads.MarkerColor, PlotConfigThreads.MarkerThickness, DragFlags);
  ImPlot::PopStyleVar();

  ImPlot::EndPlot();
  RenderPlotStyleEnd();

  // Custom Plot Legend
  ImGui::SetNextWindowPos(ImVec2(PlotConfigThreads.Position.x, PlotConfigThreads.Position.y + ImGui::CalcTextSize("THREADS").y + 4));
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

void FDFX_StatData::RenderPlotFrametime() {
  ImGui::SetNextWindowPos(PlotConfigFramerate.Position);
  ImGui::SetNextWindowSize(PlotConfigFramerate.Size);
  RenderPlotStyleBegin(EPlotType::Frametime);
  ImPlot::BeginPlot("FRAME-TIME (MS)", ImVec2(-1, -1), PlotFlags | ImPlotFlags_NoLegend);
  ImPlot::SetupAxes("", "", AxisFlags, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_Invert);
  ImPlot::SetupAxisLimits(ImAxis_X1, CurrentTime - PlotConfigFramerate.HistoryDuration, CurrentTime, ImGuiCond_Always);
  ImPlot::SetupAxisLimits(ImAxis_Y1, PlotConfigFramerate.Range.x, PlotConfigFramerate.Range.y, ImGuiCond_Always);
  ImPlot::SetupFinish();
  ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, PlotConfigFramerate.FillAlpha);
  ImPlot::PushStyleColor(ImPlotCol_Line, PlotConfigFramerate.LineColor);
  ImPlot::PushStyleColor(ImPlotCol_Fill, PlotConfigFramerate.ShadeColor);
  ImPlot::PlotLine("##Frame", &TimeHistory.Data[0], &FrameTimeHistory.Data[0], TimeHistory.Data.Num(), 0, TimeHistory.Offset, sizeof(double));
  ImPlot::PlotShaded("##Frame", &TimeHistory.Data[0], &FrameTimeHistory.Data[0], TimeHistory.Data.Num(), -INFINITY, 0, TimeHistory.Offset, sizeof(double));
  double MarkerLine = PlotConfigFramerate.MarkerLineWidth;
  ImPlot::DragLineY(0, &MarkerLine, PlotConfigFramerate.MarkerColor, PlotConfigFramerate.MarkerThickness, DragFlags);
  ImPlot::PopStyleColor(2);
  ImPlot::PopStyleVar();
  ImPlot::EndPlot();
  RenderPlotStyleEnd();
}

void FDFX_StatData::RenderPlotFramerate() {  //FPS
  ImGui::SetNextWindowPos(PlotConfigFrametime.Position);
  if (bMainWindowExpanded) {

    ImGui::SetNextWindowSize(ImVec2(ViewportSize.X - ViewportSize.X / 4, ViewportSize.Y / 3));
  } else {
    ImGui::SetNextWindowSize(PlotConfigFrametime.Size);
  }
  RenderPlotStyleBegin(EPlotType::Framerate);
  ImPlot::BeginPlot("FRAME-RATE (FPS)", ImVec2(-1, -1), PlotFlags | ImPlotFlags_NoLegend);
  ImPlot::SetupAxes("", "", AxisFlags, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoLabel);
  ImPlot::SetupAxisLimits(ImAxis_X1, CurrentTime - PlotConfigFrametime.HistoryDuration, CurrentTime, ImGuiCond_Always);
  ImPlot::SetupAxisLimits(ImAxis_Y1, PlotConfigFrametime.Range.x, PlotConfigFrametime.Range.y, ImGuiCond_Always);
  ImPlot::SetupFinish();
  ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, PlotConfigFrametime.FillAlpha);
  ImPlot::PushStyleColor(ImPlotCol_Line, PlotConfigFrametime.LineColor);
  ImPlot::PushStyleColor(ImPlotCol_Fill, PlotConfigFrametime.ShadeColor);
  ImPlot::PlotLine("##FPS", &TimeHistory.Data[0], &FPSHistory.Data[0], TimeHistory.Data.Num(), 0, TimeHistory.Offset, sizeof(double));
  ImPlot::PlotShaded("##FPS", &TimeHistory.Data[0], &FPSHistory.Data[0], TimeHistory.Data.Num(), -INFINITY, 0, TimeHistory.Offset, sizeof(double));
  double MarkerLine = PlotConfigFrametime.MarkerLineWidth;
  ImPlot::DragLineY(0, &MarkerLine, PlotConfigFrametime.MarkerColor, PlotConfigFrametime.MarkerThickness, DragFlags);
  ImPlot::PopStyleColor(2);
  ImPlot::PopStyleVar();
  ImPlot::EndPlot();
  RenderPlotStyleEnd();
}

void FDFX_StatData::RenderPlotStyleBegin(EPlotType Type) {
  const char* Title = "";
  FPlotConfig Config;
  switch (Type) {
    case EPlotType::Threads:   Title = "Threads";   Config = PlotConfigThreads;   break;
    case EPlotType::Frametime: Title = "Frametime"; Config = PlotConfigFramerate; break;
    case EPlotType::Framerate: Title = "Framerate"; Config = PlotConfigFrametime;   break;
  }
  ImGui::SetNextWindowBgAlpha(Config.BackgroundColor.w);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, Config.BackgroundColor);
  ImGui::Begin(Title, nullptr, PlotWindowFlags);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(100, 75));
  ImPlot::PushStyleVar(ImPlotStyleVar_LegendPadding, ImVec2(0, 0));
  ImPlot::PushStyleColor(ImPlotCol_PlotBg, Config.PlotBackgroundColor);
}

void FDFX_StatData::RenderPlotStyleEnd() {
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

      ImGui::TableNextColumn();
      ImGui::Text("%s", StringCast<ANSICHAR>(*FormattedCmd).Get());
      LocalToggle = Elem.bEnabled;
      ImGui::TableNextColumn();
      RenderToggleButton(StringCast<ANSICHAR>(*StatId).Get(), &LocalToggle);
      ToggleStat(Elem.Command, LocalToggle);
      Elem.bEnabled = LocalToggle;
    }
  }
}