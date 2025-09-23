#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Misc/App.h"
#include "GenericPlatform/GenericApplication.h"
#include "GameFramework/PlayerController.h"
#include "RHI.h"
#include "Stats/Stats.h"
#include "Stats/StatsData.h"
#include "ShaderCodeLibrary.h"
#include "ShaderPipelineCache.h"
#include "VariableRateShadingImageManager.h"
#include "HAL/ConsoleManager.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/implot.h"
#include "DFoundryFX_StatDataType.h"
#include "Templates/Function.h"

class DFOUNDRYFX_API FDFX_StatData
{
public:
  static void RunDFoundryFX(uint64 ImGuiThreadTimeMs);

  static void LoadDefaultValues(UGameViewportClient* InViewportClient);
  static void AddShaderLog(int32 LogType, const FString& ShaderHash, double CompileTime);

  static bool bMainWindowExpanded;

  static bool bHasPendingChanges;
  static void ApplyPendingChanges();
private:

  static UGameViewportClient* ViewportClient;
  static FVector2D ViewportSize;
  static FDisplayMetrics DisplayMetrics;

  static void UpdateStats();
  static void RenderMainWindow();

  // Main Window Tabs
  static void RenderEngineTab();
  static void RenderEngineTab_Data();
  static void RenderShadersTab();
  static void RenderStatTab();
  static void RenderSettingsTab();
  static void RenderDebugTab();

  // ImPlot
  static void RenderPlotThreads();
  static void RenderPlotFrametime();
  static void RenderPlotFramerate();

  enum class EPlotType {
    Threads,
    Frametime,
    Framerate
  };
  static void RenderPlotStyleBegin(EPlotType Type);
  static void RenderPlotStyleEnd();

  static double ImPlotFrameCount;

  static bool bShowPlots;
  static bool bSortPlots;
  static bool bShowDebugTab;

  static const int32 MaxStatHistory = 10;
  static float GlobalStatHistoryDuration;

  struct FPlotConfig {
    FPlotConfig() = default;
    bool bVisible = true;
    float HistoryDuration = 3.0f;
    ImVec2 Range = ImVec2(0, 0);
    ImVec2 Position = ImVec2(0, 0);
    ImVec2 Size = ImVec2(0, 0);
    ImVec4 BackgroundColor = ImVec4(0.21f, 0.22f, 0.23f, 0.05f);
    ImVec4 PlotBackgroundColor = ImVec4(0.32f, 0.50f, 0.77f, 0.05f);
    float FillAlpha = 0.5f;
    ImVec4 LineColor = ImVec4(0.161f, 0.29f, 0.478f, 1.0f);
    ImVec4 ShadeColor = ImVec4(0.298f, 0.447f, 0.69f, 1.0f);
    ImVec4 MarkerColor = ImVec4(0.0f, 0.25f, 0.0f, 1.0f);
    float MarkerLineWidth = 16.667f;
    float MarkerThickness = 1.0f;
  };
  static FPlotConfig PlotConfigThreads;
  static FPlotConfig PlotConfigFramerate;
  static FPlotConfig PlotConfigFrametime;

  struct FThreadPlotStyle {
    bool bShowFramePlot;
    ImVec4 LineColor;
    ImVec4 ShadeColor;
  };
  static FThreadPlotStyle ThreadPlotStyles[7];

  static const int32 MaxHistorySize = 600;
  struct FHistoryBuffer {
    FHistoryBuffer() {
      MaxSize = MaxHistorySize;
      Offset = 0;
      Data.Reserve(MaxHistorySize);
    }
    int32 MaxSize;
    int32 Offset;
    TArray<double> Data;
    void Add(double Value) {
      if (Data.Num() < MaxSize) {
        Data.Add(Value);
      } else {
        Data[Offset] = Value;
        Offset = (Offset + 1) % MaxSize;
      }
    }
    void Reset() {
      if (Data.Num() > 0) {
        Data.Reset();
        Offset = 0;
      }
    }
  };

  // Real-time data
  static double CurrentTime;
  static double LastTime;
  static double DeltaTime;
  static int32 FrameCount;
  static float FrameTime;
  static int32 FPS;
  static float GameThreadTime;
  static float RenderThreadTime;
  static float GPUFrameTime;
  static float RHIThreadTime;
  static float SwapBufferTime;
  static float InputLatencyTime;
  static float ImGuiThreadTime;

  static FHistoryBuffer TimeHistory;
  static FHistoryBuffer FrameCountHistory;
  static FHistoryBuffer FrameTimeHistory;
  static FHistoryBuffer FPSHistory;
  static FHistoryBuffer GameThreadTimeHistory;
  static FHistoryBuffer RenderThreadTimeHistory;
  static FHistoryBuffer GPUFrameTimeHistory;
  static FHistoryBuffer RHIThreadTimeHistory;
  static FHistoryBuffer SwapBufferTimeHistory;
  static FHistoryBuffer InputLatencyTimeHistory;
  static FHistoryBuffer ImGuiThreadTimeHistory;

  static inline ImGuiWindowFlags PlotWindowFlags = ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoBringToFrontOnFocus
    | ImGuiWindowFlags_NoFocusOnAppearing
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoScrollWithMouse
    | ImGuiWindowFlags_NoMouseInputs
    | ImGuiWindowFlags_NoDocking;

  static inline ImPlotFlags PlotFlags = ImPlotFlags_NoMenus
    | ImPlotFlags_NoBoxSelect
    | ImPlotFlags_NoMouseText
    | ImPlotFlags_NoInputs
    | ImPlotFlags_NoFrame;

  static inline ImPlotAxisFlags AxisFlags = ImPlotAxisFlags_NoDecorations
    | ImPlotAxisFlags_NoMenus
    | ImPlotAxisFlags_NoSideSwitch
    | ImPlotAxisFlags_NoHighlight
    | ImPlotAxisFlags_NoLabel
    | ImPlotAxisFlags_Opposite;

  static inline ImPlotShadedFlags ShadeFlags = 0;
  static inline ImPlotLineFlags LineFlags = 0;
  static inline ImPlotLineFlags DragFlags = ImPlotDragToolFlags_NoCursors
    | ImPlotDragToolFlags_NoFit
    | ImPlotDragToolFlags_NoInputs;

  static void ToggleStat(const FString& StatName, bool& bValue);
  static void RenderToggleButton(const char* LabelId, bool* Value);
  static void RenderHelpMarker(const char* Description);
  static void RenderThreadMarker(int32 PlotColorIndex);
  template<typename T>
  static void RenderInfoHelper(const FString& Info, const T& Value);

  static void RenderStats(EDFXStatCategory InCategory, const FString& Filter = TEXT(""));

  // Shader Compiler
  struct FShaderCompileLog {
    int32 Type;
    int32 Count;
    double Time;
    FString Hash;
  };
  static TArray<FShaderCompileLog> ShaderCompileLogs;
};