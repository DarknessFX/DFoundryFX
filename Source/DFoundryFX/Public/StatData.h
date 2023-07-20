#pragma once

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Misc/App.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "RHI.h"
#include "Stats/Stats2.h"
#include "Stats/StatsData.h"
#include "ShaderCodeLibrary.h"
#include "ShaderPipelineCache.h"
#include "HAL/ConsoleManager.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/implot.h"

class DFOUNDRYFX_API FDFX_StatData
{
public:
  static void RunDFoundryFX(UGameViewportClient* Viewport, uint64 ImGuiThreadTime);

  enum EStatHeader : int {
    All = 0,
    None = 1,
    Common = 2,
    Perf = 3,
    Fav = 8,
  };
  struct FStatCmd {
    EStatHeader Header;
    bool Enable;
    FString Command;
  };
  static inline TArray<FStatCmd> aStatCmds;

  static void LoadSTAT(FDFX_StatData::EStatHeader InHeader, FString InList);
  static void LoadCVAR();

  static inline bool bMainWindowOpen = false;
  static inline bool bExternalWindow = false;
  static inline bool bDisableGameControls = true;

  static void AddShaderLog(int Type, FString Hash, double Time);

private:
  static inline bool bIsDefaultLoaded = false;
  static void LoadDefaultValues(FVector2D InViewportSize);

  static inline UGameViewportClient* m_Viewport;
  static inline FVector2D ViewSize;

  static void UpdateStats();
  static void MainWindow();

  static void Tab_Engine();
  static void Tab_Shaders();
  static void Tab_STAT();
  static void Tab_Settings();
  static void Tab_Debug();

  static inline bool bShowPlots = true;
  static inline bool bPlotsSort = true;
  static inline bool bShowDebugTab = true;

  static inline const int StatHistoryMax = 10;
  static inline float StatHistoryGlobal = 3.0f; // seconds
  struct FPlotWindow {
    FPlotWindow() {} // D11.DH: Clang bug https://github.com/llvm/llvm-project/issues/36032
    bool bShowPlot = true;
    float History = 3.0f;
    ImVec2 Range = ImVec2(0, 0);
    ImVec2 Position = ImVec2(0, 0);
    ImVec2 Size = ImVec2(0, 0);
    ImVec4 BackgroundColor = ImVec4(0.21, 0.22, 0.23, 0.05);
    ImVec4 PlotBackgroundColor = ImVec4(0.32, 0.50, 0.77, 0.05);
    float PlotStyleFillAlpha = 0.5;
    ImVec4 PlotLineColor = ImVec4(0.161, 0.29, 0.478, 1);
    ImVec4 PlotShadeColor = ImVec4(0.298, 0.447, 0.69, 1);
    ImVec4 MarkerColor = ImVec4(0.0, 0.25, 0.0, 1.0);
    float MarkerLine = 16.667;
    float MarkerThick = 1;
  };
  static inline FPlotWindow pwThread;
  static inline FPlotWindow pwFrame;
  static inline FPlotWindow pwFPS;

  struct FThreadColors {
    bool bShowFramePlot;
    ImVec4 PlotLineColor;
    ImVec4 PlotShadeColor;
  };
  static inline FThreadColors pwThreadColor[7];

  static void LoadThreadPlot();
  static void LoadFramePlot();
  static void LoadFPSPlot();

  static void LoadDemos();

  static inline const int HistoryMaxSize = 600;
  struct FHistoryBuffer {
    int MaxSize;
    int Offset;
    ImVector<double> Data;
    FHistoryBuffer (int max_size = HistoryMaxSize) {
      MaxSize = max_size;
      Offset = 0;
      Data.reserve(MaxSize);
    }
    void Add(double Value) {
      if (Data.size() < MaxSize)
      {
        Data.push_back(Value);
      } else {
        Data[Offset] = Value;
        Offset = (Offset + 1) % MaxSize;
      }
    }
    void Erase() {
      if (Data.size() > 0) {
        Data.shrink(0);
        Offset = 0;
      }
    }
  };

  static inline double m_CurrentTime;
  static inline double m_LastTime;
  static inline double m_DiffTime;
  static inline int32  m_FrameCount;
  static inline float  m_FrameTime;
  static inline int    m_FramesPerSecond;
  static inline float  m_GameThreadTime;
  static inline float  m_RenderThreadTime;
  static inline float  m_GPUFrameTime;
  static inline float  m_RHIThreadTime;
  static inline float  m_SwapBufferTime;
  static inline float  m_InputLatencyTime;
  static inline float  m_ImGuiThreadTime;

  static inline double ImPlotFrameCount;
  static inline FHistoryBuffer HistoryTime {HistoryMaxSize};
  static inline FHistoryBuffer FrameCount {HistoryMaxSize};
  static inline FHistoryBuffer FrameTime {HistoryMaxSize};
  static inline FHistoryBuffer FramesPerSecond {HistoryMaxSize};
  static inline FHistoryBuffer GameThreadTime {HistoryMaxSize};
  static inline FHistoryBuffer RenderThreadTime {HistoryMaxSize};
  static inline FHistoryBuffer GPUFrameTime {HistoryMaxSize};
  static inline FHistoryBuffer RHIThreadTime {HistoryMaxSize};
  static inline FHistoryBuffer SwapBufferTime {HistoryMaxSize};
  static inline FHistoryBuffer InputLatencyTime {HistoryMaxSize};
  static inline FHistoryBuffer ImGuiThreadTime {HistoryMaxSize};

  static inline ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoBringToFrontOnFocus |
    ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoMouseInputs |
    ImGuiWindowFlags_NoDocking;

  static inline ImPlotFlags plot_flags = ImPlotFlags_NoMenus |
    ImPlotFlags_NoBoxSelect |
    ImPlotFlags_NoMouseText |
    ImPlotFlags_NoInputs |
    ImPlotFlags_NoFrame;

  static inline ImPlotAxisFlags axis_flags = ImPlotAxisFlags_NoDecorations |
    ImPlotAxisFlags_NoMenus |
    ImPlotAxisFlags_NoSideSwitch |
    ImPlotAxisFlags_NoHighlight |
    ImPlotAxisFlags_NoLabel |
    ImPlotAxisFlags_Opposite;

  static inline ImPlotShadedFlags shade_flags = 0;
  static inline ImPlotLineFlags line_flags = 0;

  static inline ImPlotLineFlags drag_flags = ImPlotDragToolFlags_NoCursors |
    ImPlotDragToolFlags_NoFit | 
    ImPlotDragToolFlags_NoInputs;

  static inline void ToggleButton(const char* str_id, bool* v);
  static inline void ToggleStat(FString StatName, bool& bValue);
  static inline void HelpMarker(const char* desc);
  static inline void ThreadMarker(int PlotColorId);
  static inline void InfoHelper(FString InInfo, bool InValue);
  static inline void InfoHelper(FString InInfo, int32 InValue);
  static inline void InfoHelper(FString InInfo, uint32 InValue);
  static inline void InfoHelper(FString InInfo, uint64 InValue);
  static inline void InfoHelper(FString InInfo, float InValue);
  static inline void InfoHelper(FString InInfo, double InValue);
  static inline void InfoHelper(FString InInfo, FString InValue);
  static inline void InfoHelper(FString InInfo, FText InValue);
  static inline void InfoHelper(FString InInfo, const TCHAR* InValue);

  static inline void DrawSTAT(FDFX_StatData::EStatHeader InHeader, FString InFilter = "");

  struct FShaderCompilerLog {
    int Type;
    int Count;
    double Time;
    FString Hash;
  };
  static inline TArray<FShaderCompilerLog> ShaderCompilerLog;
};