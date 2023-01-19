#include "StatData.h"

#define LOCTEXT_NAMESPACE "DFX_StatData"
void FDFX_StatData::RunDFoundryFX(UGameViewportClient* Viewport, uint64 ImGuiThread)
{
  m_Viewport = Viewport;
  m_Viewport->GetViewportSize(ViewSize);
  m_ImGuiThreadTime = 0.9 * m_ImGuiThreadTime + 0.1 * (ImGuiThread * FPlatformTime::GetSecondsPerCycle64());

  LoadDefaultValues(ViewSize);

  UpdateStats();
  MainWindow();

  if (bShowPlots)
  {
    if (pwThread.bShowPlot)  LoadThreadPlot();
    if (pwFrame.bShowPlot)   LoadFramePlot();
    if (pwFPS.bShowPlot)     LoadFPSPlot();
  }

  //EnableDebugWindow();
}

void FDFX_StatData::UpdateStats()
{
  if (FApp::IsBenchmarking() || FApp::UseFixedTimeStep()) {
    m_CurrentTime = FPlatformTime::Seconds();
    if (m_LastTime == 0) 
      m_LastTime = m_CurrentTime;
    m_DiffTime = m_CurrentTime - m_LastTime;
    m_LastTime = m_CurrentTime;
  } else {
    m_CurrentTime = FApp::GetCurrentTime();
    m_LastTime = FApp::GetLastTime();
    m_DiffTime = m_CurrentTime - m_LastTime;
  }

  m_FrameCount = FStats::GameThreadStatsFrame.Load(EMemoryOrder::Relaxed);
  m_FrameTime = 0.9 * m_FrameTime + 0.1 * (m_DiffTime * 1000.0f);
  m_FramesPerSecond = static_cast<int>(round(1000 / m_FrameTime));
  m_GameThreadTime = 0.9 * m_GameThreadTime + 0.1 * FPlatformTime::ToMilliseconds(GGameThreadTime);
  m_RenderThreadTime = 0.9 * m_RenderThreadTime + 0.1 * FPlatformTime::ToMilliseconds(GRenderThreadTime);
  m_GPUFrameTime = 0.9 * m_GPUFrameTime + 0.1 * FPlatformTime::ToMilliseconds(GGPUFrameTime);
  m_RHIThreadTime = 0.9 * m_RHIThreadTime + 0.1 * FPlatformTime::ToMilliseconds(GWorkingRHIThreadTime); // GRHIThreadTime display some crazy values on Shipping builds, changed to GWorkingRHIThreadTime.
  m_SwapBufferTime = 0.9 * m_SwapBufferTime + 0.1 * FPlatformTime::ToMilliseconds(GSwapBufferTime);
  m_InputLatencyTime = 0.9 * m_InputLatencyTime + 0.1 * FPlatformTime::ToMilliseconds(GInputLatencyTimer.DeltaTime);

  // Save data for ImPlot
  HistoryTime.Add(m_CurrentTime);
  FrameCount.Add(ImPlotFrameCount);
  FrameTime.Add(m_FrameTime);
  FramesPerSecond.Add( static_cast<double>(m_FramesPerSecond) );
  GameThreadTime.Add(m_GameThreadTime);
  RenderThreadTime.Add(m_RenderThreadTime);
  GPUFrameTime.Add(m_GPUFrameTime);
  RHIThreadTime.Add(m_RHIThreadTime);
  SwapBufferTime.Add(m_SwapBufferTime);
  InputLatencyTime.Add(m_InputLatencyTime);
  ImGuiThreadTime.Add(m_ImGuiThreadTime);

  ImPlotFrameCount++;
}

void FDFX_StatData::LoadThreadPlot()
{
  ImGui::SetNextWindowPos(pwThread.Position);
  ImGui::SetNextWindowSize(pwThread.Size);
  ImGui::SetNextWindowBgAlpha(pwThread.BackgroundColor.w);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, pwThread.BackgroundColor);
  ImGui::Begin("Threads", nullptr, window_flags);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(100, 75));
  ImPlot::PushStyleVar(ImPlotStyleVar_LegendPadding, ImVec2(0, 0));
  ImPlot::PushStyleColor(ImPlotCol_PlotBg, pwThread.PlotBackgroundColor);
  ImPlot::BeginPlot("THREADS (MS)", ImVec2(-1, -1), plot_flags | ImPlotFlags_NoLegend);
  ImPlot::SetupAxes("Threads", "", axis_flags, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoLabel);
  ImPlot::SetupAxisLimits(ImAxis_X1, m_CurrentTime - pwThread.History, m_CurrentTime, ImGuiCond_Always);
  ImPlot::SetupAxisLimits(ImAxis_Y1, pwThread.Range.x, pwThread.Range.y, ImGuiCond_Always);
  ImPlot::SetupFinish();
  ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, pwThread.PlotStyleFillAlpha);

  TArray<bool>  ThreadDrawOrder;
  TArray<float> ThreadPlotOrder;
  ThreadDrawOrder.Init(false, 7);
  ThreadPlotOrder.SetNumZeroed(7);

  if (bPlotsSort) {
    ThreadPlotOrder[0] = m_GameThreadTime;
    ThreadPlotOrder[1] = m_RenderThreadTime;
    ThreadPlotOrder[2] = m_GPUFrameTime;
    ThreadPlotOrder[3] = m_RHIThreadTime;
    ThreadPlotOrder[4] = m_SwapBufferTime;
    ThreadPlotOrder[5] = m_InputLatencyTime;
    ThreadPlotOrder[6] = m_ImGuiThreadTime;
    ThreadPlotOrder.Sort();
  }
  else {
    ThreadPlotOrder[0] = m_ImGuiThreadTime;
    ThreadPlotOrder[1] = m_InputLatencyTime;
    ThreadPlotOrder[2] = m_SwapBufferTime;
    ThreadPlotOrder[3] = m_RHIThreadTime;
    ThreadPlotOrder[4] = m_GPUFrameTime;
    ThreadPlotOrder[5] = m_RenderThreadTime;
    ThreadPlotOrder[6] = m_GameThreadTime;
  }

  for (int i = 6; i >= 0; --i)
  {
    if (ThreadPlotOrder[i] == m_GameThreadTime && !ThreadDrawOrder[0]) {
      if (pwThreadColor[0].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, pwThreadColor[0].PlotLineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, pwThreadColor[0].PlotShadeColor);
        ImPlot::PlotLine("Game", &HistoryTime.Data[0], &GameThreadTime.Data[0], HistoryTime.Data.size(), line_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PlotShaded("Game", &HistoryTime.Data[0], &GameThreadTime.Data[0], HistoryTime.Data.size(), -INFINITY, shade_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[0] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == m_RenderThreadTime && !ThreadDrawOrder[1]) {
      if (pwThreadColor[1].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, pwThreadColor[1].PlotLineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, pwThreadColor[1].PlotShadeColor);
        ImPlot::PlotLine("Render", &HistoryTime.Data[0], &RenderThreadTime.Data[0], HistoryTime.Data.size(), line_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PlotShaded("Render", &HistoryTime.Data[0], &RenderThreadTime.Data[0], HistoryTime.Data.size(), -INFINITY, shade_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[1] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == m_GPUFrameTime && !ThreadDrawOrder[2]) {
      if (pwThreadColor[2].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, pwThreadColor[2].PlotLineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, pwThreadColor[2].PlotShadeColor);
        ImPlot::PlotLine("GPU", &HistoryTime.Data[0], &GPUFrameTime.Data[0], HistoryTime.Data.size(), line_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PlotShaded("GPU", &HistoryTime.Data[0], &GPUFrameTime.Data[0], HistoryTime.Data.size(), -INFINITY, shade_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[2] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == m_RHIThreadTime && !ThreadDrawOrder[3]) {
      if (pwThreadColor[3].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, pwThreadColor[3].PlotLineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, pwThreadColor[3].PlotShadeColor);
        ImPlot::PlotLine("RHI", &HistoryTime.Data[0], &RHIThreadTime.Data[0], HistoryTime.Data.size(), line_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PlotShaded("RHI", &HistoryTime.Data[0], &RHIThreadTime.Data[0], HistoryTime.Data.size(), -INFINITY, shade_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[3] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == m_SwapBufferTime && !ThreadDrawOrder[4]) {
      if (pwThreadColor[4].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, pwThreadColor[4].PlotLineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, pwThreadColor[4].PlotShadeColor);
        ImPlot::PlotLine("Swap", &HistoryTime.Data[0], &SwapBufferTime.Data[0], HistoryTime.Data.size(), line_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PlotShaded("Swap", &HistoryTime.Data[0], &SwapBufferTime.Data[0], HistoryTime.Data.size(), -INFINITY, shade_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[4] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == m_InputLatencyTime && !ThreadDrawOrder[5]) {
      if (pwThreadColor[5].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, pwThreadColor[5].PlotLineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, pwThreadColor[5].PlotShadeColor);
        ImPlot::PlotLine("Input", &HistoryTime.Data[0], &InputLatencyTime.Data[0], HistoryTime.Data.size(), line_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PlotShaded("Input", &HistoryTime.Data[0], &InputLatencyTime.Data[0], HistoryTime.Data.size(), -INFINITY, shade_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[5] = true;
      continue;
    }

    if (ThreadPlotOrder[i] == m_ImGuiThreadTime && !ThreadDrawOrder[6]) {
      if (pwThreadColor[6].bShowFramePlot) {
        ImPlot::PushStyleColor(ImPlotCol_Line, pwThreadColor[6].PlotLineColor);
        ImPlot::PushStyleColor(ImPlotCol_Fill, pwThreadColor[6].PlotShadeColor);
        ImPlot::PlotLine("ImGui", &HistoryTime.Data[0], &ImGuiThreadTime.Data[0], HistoryTime.Data.size(), line_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PlotShaded("ImGui", &HistoryTime.Data[0], &ImGuiThreadTime.Data[0], HistoryTime.Data.size(), -INFINITY, shade_flags, HistoryTime.Offset, sizeof(double));
        ImPlot::PopStyleColor(2);
      }
      ThreadDrawOrder[6] = true;
      continue;
    }
  }

  double MarkerLine = pwThread.MarkerLine;
  ImPlot::DragLineY(0, &MarkerLine, ImVec4(0.0, 0.25, 0.0, 1.0), pwThread.MarkerThick, drag_flags); //pwThread.MarkerColor
  ImPlot::PopStyleVar();

  ImPlot::EndPlot();
  ImPlot::PopStyleColor();
  ImPlot::PopStyleVar(4);
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(4);

  // Custom Plot Legend
  ImGui::SetNextWindowPos(ImVec2(pwThread.Position.x, pwThread.Position.y + ImGui::CalcTextSize("THREADS").y + 4));
  ImGui::SetNextWindowSize(ImVec2(-1, -1));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0);
  ImGui::Begin("##ThreadsLegendWin", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoNav |
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoBringToFrontOnFocus); //window_flags

  ImGui::BeginTable("##tblThreadLegend", 2);
  if (pwThreadColor[0].bShowFramePlot) {
    ImGui::TableNextColumn();
    FDFX_StatData::ThreadMarker(0);
    ImGui::TextColored(pwThreadColor[0].PlotShadeColor, "_ Game");
    ImGui::TableNextColumn(); ImGui::Text("%4.3f", m_GameThreadTime);
  }
  if (pwThreadColor[1].bShowFramePlot) {
    ImGui::TableNextColumn();
    FDFX_StatData::ThreadMarker(1);
    ImGui::TextColored(pwThreadColor[1].PlotShadeColor, "_ Render");
    ImGui::TableNextColumn(); ImGui::Text("%4.3f", m_RenderThreadTime);
  }
  if (pwThreadColor[2].bShowFramePlot) {
    ImGui::TableNextColumn();
    FDFX_StatData::ThreadMarker(2);
    ImGui::TextColored(pwThreadColor[2].PlotShadeColor, "_ GPU");
    ImGui::TableNextColumn(); ImGui::Text("%4.3f", m_GPUFrameTime);
  }
  if (pwThreadColor[3].bShowFramePlot) {
    ImGui::TableNextColumn();
    FDFX_StatData::ThreadMarker(3);
    ImGui::TextColored(pwThreadColor[3].PlotShadeColor, "_ RHI");
    ImGui::TableNextColumn(); ImGui::Text("%4.3f", m_RHIThreadTime);
  }
  if (pwThreadColor[4].bShowFramePlot) {
    ImGui::TableNextColumn();
    FDFX_StatData::ThreadMarker(4);
    ImGui::TextColored(pwThreadColor[4].PlotShadeColor, "_ Swap");
    ImGui::TableNextColumn(); ImGui::Text("%4.3f", m_SwapBufferTime);
  }
  if (pwThreadColor[5].bShowFramePlot) {
    ImGui::TableNextColumn();
    FDFX_StatData::ThreadMarker(5);
    ImGui::TextColored(pwThreadColor[5].PlotShadeColor, "_ Input");
    ImGui::TableNextColumn(); ImGui::Text("%4.3f", m_InputLatencyTime);
  }
  if (pwThreadColor[6].bShowFramePlot) {
    ImGui::TableNextColumn();
    FDFX_StatData::ThreadMarker(6);
    ImGui::TextColored(pwThreadColor[6].PlotShadeColor, "_ ImGui");
    ImGui::TableNextColumn(); ImGui::Text("%4.3f", m_ImGuiThreadTime);
  }

  int32 NumDrawCalls = GNumDrawCallsRHI[0];
  ImGui::TableNextColumn(); ImGui::Text("Draws");
  ImGui::TableNextColumn(); ImGui::Text("%i", NumDrawCalls);
  ImGui::TableNextColumn(); ImGui::Text("Triangles");
  int32 NumPrimitives = GNumPrimitivesDrawnRHI[0];
  if (NumPrimitives < 10000) {
    ImGui::TableNextColumn(); ImGui::Text("%i", NumPrimitives);
  } else {
    ImGui::TableNextColumn(); ImGui::Text("%.1f K", NumPrimitives / 1000.f);
  }

  ImGui::EndTable();
  ImGui::End();
  ImGui::PopStyleVar(4);
  ImGui::BringWindowToDisplayFront(ImGui::FindWindowByName("##ThreadsLegendWin"));

}

void FDFX_StatData::LoadFramePlot()
{
  ImGui::SetNextWindowPos(pwFrame.Position);
  ImGui::SetNextWindowSize(pwFrame.Size);
  ImGui::SetNextWindowBgAlpha(pwFrame.BackgroundColor.w);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, pwFrame.BackgroundColor);
  ImGui::Begin("Frametime", nullptr, window_flags);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(100, 75));
  ImPlot::PushStyleVar(ImPlotStyleVar_LegendPadding, ImVec2(0, 0));
  ImPlot::PushStyleColor(ImPlotCol_PlotBg, pwFrame.PlotBackgroundColor);
  ImPlot::BeginPlot("FRAME-TIME (MS)", ImVec2(-1, -1), plot_flags | ImPlotFlags_NoLegend);
  ImPlot::SetupAxes("", "", axis_flags, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_Invert);
  ImPlot::SetupAxisLimits(ImAxis_X1, m_CurrentTime - pwFrame.History, m_CurrentTime, ImGuiCond_Always);
  ImPlot::SetupAxisLimits(ImAxis_Y1, pwFrame.Range.x, pwFrame.Range.y, ImGuiCond_Always);
  ImPlot::SetupFinish();
  ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, pwFrame.PlotStyleFillAlpha);
  ImPlot::PushStyleColor(ImPlotCol_Line, pwFrame.PlotLineColor);
  ImPlot::PushStyleColor(ImPlotCol_Fill, pwFrame.PlotShadeColor);
  ImPlot::PlotLine("##Frame", &HistoryTime.Data[0], &FrameTime.Data[0], HistoryTime.Data.size(), 0, HistoryTime.Offset, sizeof(double));
  ImPlot::PlotShaded("##Frame", &HistoryTime.Data[0], &FrameTime.Data[0], HistoryTime.Data.size(), -INFINITY, 0, HistoryTime.Offset, sizeof(double));
  double MarkerLine = pwFrame.MarkerLine;
  ImPlot::DragLineY(0, &MarkerLine, ImVec4(0.0, 0.25, 0.0, 1.0), pwFrame.MarkerThick, drag_flags);
  ImPlot::PopStyleColor(2);
  ImPlot::PopStyleVar();
  ImPlot::EndPlot();
  ImPlot::PopStyleColor();
  ImPlot::PopStyleVar(4);
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(4);
}

void FDFX_StatData::LoadFPSPlot()
{
  ImGui::SetNextWindowPos(pwFPS.Position);
  ImGui::SetNextWindowSize(pwFPS.Size);
  ImGui::SetNextWindowBgAlpha(pwFPS.PlotBackgroundColor.w);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, pwFPS.BackgroundColor);
  ImGui::Begin("FPS", nullptr, window_flags);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(100, 75));
  ImPlot::PushStyleVar(ImPlotStyleVar_LegendPadding, ImVec2(0, 0));
  ImPlot::PushStyleColor(ImPlotCol_PlotBg, pwFPS.PlotBackgroundColor);
  ImPlot::BeginPlot("FRAME-RATE (FPS)", ImVec2(-1, -1), plot_flags | ImPlotFlags_NoLegend);
  ImPlot::SetupAxes("", "", axis_flags, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoLabel);
  ImPlot::SetupAxisLimits(ImAxis_X1, m_CurrentTime - pwFrame.History, m_CurrentTime, ImGuiCond_Always);
  ImPlot::SetupAxisLimits(ImAxis_Y1, pwFPS.Range.x, pwFPS.Range.y, ImGuiCond_Always);
  ImPlot::SetupFinish();
  ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, pwFPS.PlotStyleFillAlpha);
  ImPlot::PushStyleColor(ImPlotCol_Line, pwFPS.PlotLineColor);
  ImPlot::PushStyleColor(ImPlotCol_Fill, pwFPS.PlotShadeColor);
  ImPlot::PlotLine("##FPS", &HistoryTime.Data[0], &FramesPerSecond.Data[0], HistoryTime.Data.size(), 0, HistoryTime.Offset, sizeof(double));
  ImPlot::PlotShaded("##FPS", &HistoryTime.Data[0], &FramesPerSecond.Data[0], HistoryTime.Data.size(), -INFINITY, 0, HistoryTime.Offset, sizeof(double));
  double MarkerLine = pwFPS.MarkerLine;
  ImPlot::DragLineY(0, &MarkerLine, ImVec4(0.0, 0.25, 0.0, 1.0), pwFPS.MarkerThick, drag_flags);
  ImPlot::PopStyleColor(2);
  ImPlot::PopStyleVar();
  ImPlot::EndPlot();
  ImPlot::PopStyleColor();
  ImPlot::PopStyleVar(4);
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(4);
}

void FDFX_StatData::MainWindow()
{
  APlayerController* aPC = m_Viewport->GetWorld()->GetFirstPlayerController();
  char WindowTitle[] = "DFoundryFX";
  ImGui::Begin(WindowTitle, NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

  if (ImGui::IsWindowCollapsed())  {
    bMainWindowOpen = false;
    const int Offset = 64;
    ImGui::SetWindowPos(ImVec2(ViewSize.X - ImGui::CalcTextSize(WindowTitle).x - Offset, 0));
    ImGui::SetWindowSize(ImVec2(ImGui::CalcTextSize(WindowTitle).x + Offset, 0));

    const ImRect titleBarRect = ImGui::GetCurrentWindow()->TitleBarRect();
    ImGui::PushClipRect(titleBarRect.Min, titleBarRect.Max, false);
    ImGui::SetCursorPos(ImVec2(ImGui::CalcTextSize(WindowTitle).x + Offset / 2, 0.0f));
    ImGui::Text(" "); ImGui::SameLine();
    ImGui::Checkbox("##DisplayGraphs", &bShowPlots);
    ImGui::PopClipRect();

    if (ImGui::IsWindowHovered()) {
      ImGui::GetIO().MouseDrawCursor = true;
    }
    else {
      ImGui::GetIO().MouseDrawCursor = false;
    }
  } else {
    bMainWindowOpen = true;
    ImGui::SetWindowPos(ImVec2(ViewSize.X - (ViewSize.X / 4), 0));
    ImGui::SetWindowSize(ImVec2(ViewSize.X / 4, ViewSize.Y));
    ImGui::BeginTabBar("##MainWindowTabBar");

    if (ImGui::BeginTabItem("Engine"))    Tab_Engine();
    if (ImGui::BeginTabItem("Shaders"))   Tab_Shaders();
    if (ImGui::BeginTabItem("STAT"))      Tab_STAT();
    if (ImGui::BeginTabItem("Settings"))  Tab_Settings();
    if (bShowDebugTab)
      if (ImGui::BeginTabItem("Debug"))   Tab_Debug();

    ImGui::EndTabBar();

    if (ImGui::IsWindowHovered()) {
      ImGui::GetIO().MouseDrawCursor = true;
    }
  }

  ImGui::End();
}

void FDFX_StatData::Tab_Engine()
{
  UWorld* uWorld = m_Viewport->GetWorld();
  IConsoleManager& m_ConMng = IConsoleManager::Get();

  if (ImGui::CollapsingHeader("Viewport Settings")) {
    static int m_vwSize[2] = { static_cast<int>(ViewSize.X) , static_cast<int>(ViewSize.Y) };
    static int m_MaxFPS = m_ConMng.FindConsoleVariable(TEXT("t.MaxFPS"))->GetInt();
    static bool m_bVSync = m_ConMng.FindConsoleVariable(TEXT("r.VSync"))->GetBool();
    static int m_ScrPct = m_ConMng.FindConsoleVariable(TEXT("r.ScreenPercentage"))->GetInt();
    static int m_ResQlt = m_ConMng.FindConsoleVariable(TEXT("sg.ResolutionQuality"))->GetInt();
    static int m_vwDist = m_ConMng.FindConsoleVariable(TEXT("r.ViewDistanceScale"))->GetInt();
    static int m_PPQlt = m_ConMng.FindConsoleVariable(TEXT("sg.PostProcessQuality"))->GetInt();
    static int m_ShdwQlt = m_ConMng.FindConsoleVariable(TEXT("sg.ShadowQuality"))->GetInt();
    static int m_TexQlt = m_ConMng.FindConsoleVariable(TEXT("sg.TextureQuality"))->GetInt();
    static int m_FXQlt = m_ConMng.FindConsoleVariable(TEXT("sg.EffectsQuality"))->GetInt();
    static int m_DetMd = m_ConMng.FindConsoleVariable(TEXT("r.DetailMode"))->GetInt();
    static int m_SKLOD = m_ConMng.FindConsoleVariable(TEXT("r.SkeletalMeshLODBias"))->GetInt();
    static bool m_bFullscreen = m_Viewport->Viewport->IsFullscreen();
    static bool m_bFullscreenExclusive = (m_ConMng.FindConsoleVariable(TEXT("r.FullscreenMode"))->GetInt() == 1? false : true);

    ImGui::BeginTable("##tblViewSettingsBase", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextColumn();
    ImGui::InputInt2("Size", m_vwSize); ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS1")) {
      if (m_bFullscreen) {
        m_Viewport->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%if"), m_vwSize[0], m_vwSize[1]));
      } else {
        m_Viewport->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%iw"), m_vwSize[0], m_vwSize[1]));
      }
    }

    ImGui::TableNextColumn();
    if (ImGui::Checkbox("Fullscreen", &m_bFullscreen)) {
      if (m_bFullscreen != m_Viewport->Viewport->IsFullscreen()) {
        if (m_bFullscreen) {
          m_Viewport->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%if"), m_vwSize[0], m_vwSize[1]));
        } else {
          m_Viewport->ConsoleCommand(FString::Printf(TEXT("r.SetRes %ix%iw"), m_vwSize[0], m_vwSize[1]));
        }
      }
    }
    ImGui::TableNextColumn();

    ImGui::TableNextColumn();
    if (ImGui::Checkbox("FullscreenExclusive", &m_bFullscreenExclusive)) {
      if (m_bFullscreenExclusive != (m_ConMng.FindConsoleVariable(TEXT("r.FullscreenMode"))->GetInt() == 1 ? false : true)) {
        if (m_bFullscreenExclusive) {
          m_Viewport->ConsoleCommand(FString::Printf(TEXT("r.FullscreenMode 2")));
          m_bFullscreenExclusive = true;
        } else {
          m_Viewport->ConsoleCommand(FString::Printf(TEXT("r.FullscreenMode 1")));
          m_bFullscreenExclusive = false;
        }
      }
    }
    ImGui::TableNextColumn();

    ImGui::TableNextColumn();
    if (ImGui::Checkbox("VSync", &m_bVSync)) {
      if (m_bVSync != m_ConMng.FindConsoleVariable(TEXT("r.VSync"))->GetBool()) {
        if (m_bVSync) {
          m_Viewport->ConsoleCommand("r.VSync 1");
        } else {
          m_Viewport->ConsoleCommand("r.VSync 0");
        }
      }
    }
    ImGui::TableNextColumn();

    ImGui::TableNextColumn();
    ImGui::InputInt("MaxFPS", &m_MaxFPS, 1, 5); ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS2"))
      m_Viewport->ConsoleCommand(FString::Printf(TEXT("t.MaxFPS %i"), m_MaxFPS));
    ImGui::TableNextColumn();
    ImGui::InputInt("ScreenPercent", &m_ScrPct, 1, 5); ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS3"))
      m_Viewport->ConsoleCommand(FString::Printf(TEXT("r.ScreenPercentage %i"), m_ScrPct));
    ImGui::TableNextColumn();
    ImGui::InputInt("Res.Quality", &m_ResQlt, 1, 5); ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS4"))
      m_Viewport->ConsoleCommand(FString::Printf(TEXT("sg.ResolutionQuality %i"), m_ResQlt));
    ImGui::TableNextColumn();
    ImGui::InputInt("ViewDistance", &m_vwDist, 1, 5); ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS5"))
      m_Viewport->ConsoleCommand(FString::Printf(TEXT("r.ViewDistanceScale %i"), m_vwDist));
    ImGui::TableNextColumn();
    ImGui::InputInt("PP Quality", &m_PPQlt, 1, 5); ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS6"))
      m_Viewport->ConsoleCommand(FString::Printf(TEXT("sg.PostProcessQuality %i"), m_PPQlt));
    ImGui::TableNextColumn();
    ImGui::InputInt("Shadow Qual.", &m_ShdwQlt, 1, 5); ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS7"))
      m_Viewport->ConsoleCommand(FString::Printf(TEXT("sg.ShadowQuality %i"), m_ShdwQlt));
    ImGui::TableNextColumn();
    ImGui::InputInt("Texture Qual.", &m_TexQlt, 1, 5); ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS8"))
      m_Viewport->ConsoleCommand(FString::Printf(TEXT("sg.TextureQuality %i"), m_TexQlt));
    ImGui::TableNextColumn();
    ImGui::InputInt("Effects Qual.", &m_FXQlt, 1, 5); ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS9"))
      m_Viewport->ConsoleCommand(FString::Printf(TEXT("sg.EffectsQuality %i"), m_FXQlt));
    ImGui::TableNextColumn();
    ImGui::InputInt("Detail Mode", &m_DetMd, 1, 5); ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS10"))
      m_Viewport->ConsoleCommand(FString::Printf(TEXT("r.DetailMode %i"), m_DetMd));
    ImGui::TableNextColumn();
    ImGui::InputInt("Skeletal LOD", &m_SKLOD, 1, 5); ImGui::SameLine();
    ImGui::TableNextColumn();
    if (ImGui::Button("Apply##btnVS11"))
      m_Viewport->ConsoleCommand(FString::Printf(TEXT("r.SkeletalMeshLODBias %i"), m_SKLOD));

    ImGui::EndTable();
  }

  if (ImGui::CollapsingHeader("Memory Stats")) {
    FPlatformMemoryStats Stats = FPlatformMemory::GetStats();
    ImGui::Text("Mem       %s", TCHAR_TO_ANSI(*GetMemoryString(Stats.UsedPhysical)));
    ImGui::Text("MemPeak   %s", TCHAR_TO_ANSI(*GetMemoryString(Stats.PeakUsedPhysical)));
    ImGui::Text("MemAvail  %s", TCHAR_TO_ANSI(*GetMemoryString(Stats.AvailablePhysical)));
    ImGui::Text("MemTotal  %s", TCHAR_TO_ANSI(*GetMemoryString(Stats.TotalPhysical)));
    ImGui::Text("VMem      %s", TCHAR_TO_ANSI(*GetMemoryString(Stats.UsedVirtual)));
    ImGui::Text("VMemPeak  %s", TCHAR_TO_ANSI(*GetMemoryString(Stats.PeakUsedVirtual)));
    ImGui::Text("VMemAvail %s", TCHAR_TO_ANSI(*GetMemoryString(Stats.AvailableVirtual)));
    ImGui::Text("VMemTotal %s", TCHAR_TO_ANSI(*GetMemoryString(Stats.TotalVirtual)));
  }

  if (ImGui::CollapsingHeader("Viewport Context")) {
    ImGui::BeginDisabled();
    InfoHelper("bIsPlayInEditorViewport", m_Viewport->bIsPlayInEditorViewport);
    InfoHelper("GetDPIScale", m_Viewport->GetDPIScale());
    InfoHelper("GetDPIDerivedResolutionFraction", m_Viewport->GetDPIDerivedResolutionFraction());
    InfoHelper("IsCursorVisible", m_Viewport->Viewport->IsCursorVisible());
    InfoHelper("IsForegroundWindow", m_Viewport->Viewport->IsForegroundWindow());
    InfoHelper("IsExclusiveFullscreen", m_Viewport->Viewport->IsExclusiveFullscreen());
    InfoHelper("IsFullscreen", m_Viewport->Viewport->IsFullscreen());
    InfoHelper("IsGameRenderingEnabled", m_Viewport->Viewport->IsGameRenderingEnabled());
    InfoHelper("IsHDRViewport", m_Viewport->Viewport->IsHDRViewport());
    InfoHelper("IsKeyboardAvailable", m_Viewport->Viewport->IsKeyboardAvailable(0));
    InfoHelper("IsMouseAvailable", m_Viewport->Viewport->IsMouseAvailable(0));
    InfoHelper("IsPenActive", m_Viewport->Viewport->IsPenActive());
    InfoHelper("IsPlayInEditorViewport", m_Viewport->Viewport->IsPlayInEditorViewport());
    InfoHelper("IsSlateViewport", m_Viewport->Viewport->IsSlateViewport());
    InfoHelper("IsSoftwareCursorVisible", m_Viewport->Viewport->IsSoftwareCursorVisible());
    InfoHelper("IsStereoRenderingAllowed", m_Viewport->Viewport->IsStereoRenderingAllowed());
    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("GEngine Context")) {
    ImGui::BeginDisabled();

    InfoHelper("IsEditor", GEngine->IsEditor());
    InfoHelper("IsAllowedFramerateSmoothing", GEngine->IsAllowedFramerateSmoothing());
    InfoHelper("bForceDisableFrameRateSmoothing", GEngine->bForceDisableFrameRateSmoothing);
    InfoHelper("bSmoothFrameRate", GEngine->bSmoothFrameRate);
    InfoHelper("MinDesiredFrameRate", GEngine->MinDesiredFrameRate);
    InfoHelper("bUseFixedFrameRate", GEngine->bUseFixedFrameRate);
    InfoHelper("FixedFrameRate", GEngine->FixedFrameRate);
    InfoHelper("bCanBlueprintsTickByDefault", GEngine->bCanBlueprintsTickByDefault);
    InfoHelper("IsControllerIdUsingPlatformUserId", GEngine->IsControllerIdUsingPlatformUserId());
    InfoHelper("IsStereoscopic3D", GEngine->IsStereoscopic3D(m_Viewport->Viewport));
    InfoHelper("IsVanillaProduct", GEngine->IsVanillaProduct());
    InfoHelper("HasMultipleLocalPlayers", GEngine->HasMultipleLocalPlayers(uWorld));

    InfoHelper("AreEditorAnalyticsEnabled", GEngine->AreEditorAnalyticsEnabled());
    InfoHelper("bAllowMultiThreadedAnimationUpdate", GEngine->bAllowMultiThreadedAnimationUpdate);
    InfoHelper("bDisableAILogging", GEngine->bDisableAILogging);
    InfoHelper("bEnableOnScreenDebugMessages", GEngine->bEnableOnScreenDebugMessages);
    InfoHelper("bEnableOnScreenDebugMessagesDisplay", GEngine->bEnableOnScreenDebugMessagesDisplay);
    InfoHelper("bEnableEditorPSysRealtimeLOD", GEngine->bEnableEditorPSysRealtimeLOD);
    InfoHelper("bEnableVisualLogRecordingOnStart", GEngine->bEnableVisualLogRecordingOnStart);
    InfoHelper("bGenerateDefaultTimecode", GEngine->bGenerateDefaultTimecode);
    InfoHelper("bIsInitialized", GEngine->bIsInitialized);
    InfoHelper("bLockReadOnlyLevels", GEngine->bLockReadOnlyLevels);
    InfoHelper("bOptimizeAnimBlueprintMemberVariableAccess", GEngine->bOptimizeAnimBlueprintMemberVariableAccess);
    InfoHelper("bPauseOnLossOfFocus", GEngine->bPauseOnLossOfFocus);
    InfoHelper("bRenderLightMapDensityGrayscale", GEngine->bRenderLightMapDensityGrayscale);
    InfoHelper("bShouldGenerateLowQualityLightmaps_DEPRECATED", GEngine->bShouldGenerateLowQualityLightmaps_DEPRECATED);
    InfoHelper("BSPSelectionHighlightIntensity", GEngine->BSPSelectionHighlightIntensity);
    InfoHelper("bStartedLoadMapMovie", GEngine->bStartedLoadMapMovie);
    InfoHelper("bSubtitlesEnabled", GEngine->bSubtitlesEnabled);
    InfoHelper("bSubtitlesForcedOff", GEngine->bSubtitlesForcedOff);
    InfoHelper("bSuppressMapWarnings", GEngine->bSuppressMapWarnings);
    InfoHelper("DisplayGamma", GEngine->DisplayGamma);
    InfoHelper("IsAutosaving", GEngine->IsAutosaving());
    InfoHelper("MaximumLoopIterationCount", GEngine->MaximumLoopIterationCount);
    InfoHelper("MaxLightMapDensity", GEngine->MaxLightMapDensity);
    InfoHelper("MaxOcclusionPixelsFraction", GEngine->MaxOcclusionPixelsFraction);
    InfoHelper("MaxParticleResize", GEngine->MaxParticleResize);
    InfoHelper("MaxParticleResizeWarn", GEngine->MaxParticleResizeWarn);
    InfoHelper("MaxPixelShaderAdditiveComplexityCount", GEngine->MaxPixelShaderAdditiveComplexityCount);
    InfoHelper("StreamingDistanceFactor", GEngine->StreamingDistanceFactor);
    InfoHelper("UseSkeletalMeshMinLODPerQualityLevels", GEngine->UseSkeletalMeshMinLODPerQualityLevels);
    InfoHelper("UseStaticMeshMinLODPerQualityLevels", GEngine->UseStaticMeshMinLODPerQualityLevels);

    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("RHI Context")) {
    ImGui::BeginDisabled();

    InfoHelper("GRHIAdapterDriverDate", GRHIAdapterDriverDate);
    InfoHelper("GRHIAdapterDriverOnDenyList", GRHIAdapterDriverOnDenyList);
    InfoHelper("GRHIAdapterInternalDriverVersion", GRHIAdapterInternalDriverVersion);
    InfoHelper("GRHIAdapterName", GRHIAdapterName);
    InfoHelper("GRHIAdapterUserDriverVersion", GRHIAdapterUserDriverVersion);
    InfoHelper("GRHIAttachmentVariableRateShadingEnabled", GRHIAttachmentVariableRateShadingEnabled);
    InfoHelper("GRHIDeviceId", GRHIDeviceId);
    InfoHelper("GRHIDeviceIsAMDPreGCNArchitecture", GRHIDeviceIsAMDPreGCNArchitecture);
    InfoHelper("GRHIDeviceIsIntegrated", GRHIDeviceIsIntegrated);
    InfoHelper("GRHIDeviceRevision", GRHIDeviceRevision);
    InfoHelper("GRHIForceNoDeletionLatencyForStreamingTextures", GRHIForceNoDeletionLatencyForStreamingTextures);
    InfoHelper("GRHIIsHDREnabled", GRHIIsHDREnabled);
    InfoHelper("GRHILazyShaderCodeLoading", GRHILazyShaderCodeLoading);
    InfoHelper("GRHIMaximumReccommendedOustandingOcclusionQueries", GRHIMaximumReccommendedOustandingOcclusionQueries);
    InfoHelper("GRHIMinimumWaveSize", GRHIMinimumWaveSize);
    InfoHelper("GRHIMaximumWaveSize", GRHIMaximumWaveSize);
    InfoHelper("GRHINeedsExtraDeletionLatency", GRHINeedsExtraDeletionLatency);
    InfoHelper("GRHINeedsUnatlasedCSMDepthsWorkaround", GRHINeedsUnatlasedCSMDepthsWorkaround);
    InfoHelper("GRHIPersistentThreadGroupCount", GRHIPersistentThreadGroupCount);
    InfoHelper("GRHIPresentCounter", GRHIPresentCounter);
    InfoHelper("GRHIRayTracingAccelerationStructureAlignment", GRHIRayTracingAccelerationStructureAlignment);
    InfoHelper("GRHIRayTracingScratchBufferAlignment", GRHIRayTracingScratchBufferAlignment);
    InfoHelper("GRHIRequiresRenderTargetForPixelShaderUAVs", GRHIRequiresRenderTargetForPixelShaderUAVs);
    InfoHelper("GRHISupportsArrayIndexFromAnyShader", GRHISupportsArrayIndexFromAnyShader);
    InfoHelper("GRHISupportsAsyncPipelinePrecompile", GRHISupportsAsyncPipelinePrecompile);
    InfoHelper("GRHISupportsAsyncTextureCreation", GRHISupportsAsyncTextureCreation);
    InfoHelper("GRHISupportsAtomicUInt64", GRHISupportsAtomicUInt64);
    InfoHelper("GRHISupportsAttachmentVariableRateShading", GRHISupportsAttachmentVariableRateShading);
    InfoHelper("GRHISupportsBackBufferWithCustomDepthStencil", GRHISupportsBackBufferWithCustomDepthStencil);
    InfoHelper("GRHISupportsBaseVertexIndex", GRHISupportsBaseVertexIndex);
    InfoHelper("GRHISupportsBindless", GRHISupportsBindless);
    InfoHelper("GRHISupportsComplexVariableRateShadingCombinerOps", GRHISupportsComplexVariableRateShadingCombinerOps);
    InfoHelper("GRHISupportsConservativeRasterization", GRHISupportsConservativeRasterization);
    //InfoHelper("GRHISupportsCopyToTextureMultipleMips", GRHISupportsCopyToTextureMultipleMips);
    InfoHelper("GRHISupportsDepthUAV", GRHISupportsDepthUAV);
    InfoHelper("GRHISupportsDirectGPUMemoryLock", GRHISupportsDirectGPUMemoryLock);
    InfoHelper("GRHISupportsDrawIndirect", GRHISupportsDrawIndirect);
    InfoHelper("GRHISupportsDX12AtomicUInt64", GRHISupportsDX12AtomicUInt64);
    InfoHelper("GRHISupportsDynamicResolution", GRHISupportsDynamicResolution);
    InfoHelper("GRHISupportsEfficientUploadOnResourceCreation", GRHISupportsEfficientUploadOnResourceCreation);
    InfoHelper("GRHISupportsExactOcclusionQueries", GRHISupportsExactOcclusionQueries);
    InfoHelper("GRHISupportsExplicitFMask", GRHISupportsExplicitFMask);
    InfoHelper("GRHISupportsExplicitHTile", GRHISupportsExplicitHTile);
    InfoHelper("GRHISupportsFirstInstance", GRHISupportsFirstInstance);
    InfoHelper("GRHISupportsFrameCyclesBubblesRemoval", GRHISupportsFrameCyclesBubblesRemoval);
    InfoHelper("GRHISupportsGPUTimestampBubblesRemoval", GRHISupportsGPUTimestampBubblesRemoval); 
    InfoHelper("GRHISupportsHDROutput", GRHISupportsHDROutput);
    InfoHelper("GRHISupportsInlineRayTracing", GRHISupportsInlineRayTracing);
    InfoHelper("GRHISupportsLargerVariableRateShadingSizes", GRHISupportsLargerVariableRateShadingSizes);
    InfoHelper("GRHISupportsLateVariableRateShadingUpdate", GRHISupportsLateVariableRateShadingUpdate);
    InfoHelper("GRHISupportsLazyShaderCodeLoading", GRHISupportsLazyShaderCodeLoading);
    InfoHelper("GRHISupportsMapWriteNoOverwrite", GRHISupportsMapWriteNoOverwrite);
    InfoHelper("GRHISupportsMeshShadersTier0", GRHISupportsMeshShadersTier0);
    InfoHelper("GRHISupportsMeshShadersTier1", GRHISupportsMeshShadersTier1);
    InfoHelper("GRHISupportsMSAADepthSampleAccess", GRHISupportsMSAADepthSampleAccess);
    InfoHelper("GRHISupportsMultithreadedResources", GRHISupportsMultithreadedResources);
    InfoHelper("GRHISupportsMultithreadedShaderCreation", GRHISupportsMultithreadedShaderCreation);
    InfoHelper("GRHISupportsMultithreading", GRHISupportsMultithreading);
    InfoHelper("GRHISupportsParallelRHIExecute", GRHISupportsParallelRHIExecute);
    InfoHelper("GRHISupportsPipelineFileCache", GRHISupportsPipelineFileCache);
    InfoHelper("GRHISupportsPipelineStateSortKey", GRHISupportsPipelineStateSortKey);
    InfoHelper("GRHISupportsPipelineVariableRateShading", GRHISupportsPipelineVariableRateShading);
    InfoHelper("GRHISupportsPixelShaderUAVs", GRHISupportsPixelShaderUAVs);
    InfoHelper("GRHISupportsPrimitiveShaders", GRHISupportsPrimitiveShaders);
    InfoHelper("GRHISupportsQuadTopology", GRHISupportsQuadTopology);
    InfoHelper("GRHISupportsRawViewsForAnyBuffer", GRHISupportsRawViewsForAnyBuffer);
    InfoHelper("GRHISupportsRayTracing", GRHISupportsRayTracing);
    InfoHelper("GRHISupportsRayTracingAMDHitToken", GRHISupportsRayTracingAMDHitToken);
    InfoHelper("GRHISupportsRayTracingAsyncBuildAccelerationStructure", GRHISupportsRayTracingAsyncBuildAccelerationStructure);
    InfoHelper("GRHISupportsRayTracingDispatchIndirect", GRHISupportsRayTracingDispatchIndirect);
    InfoHelper("GRHISupportsRayTracingPSOAdditions", GRHISupportsRayTracingPSOAdditions);
    InfoHelper("GRHISupportsRayTracingShaders", GRHISupportsRayTracingShaders);
    InfoHelper("GRHISupportsRectTopology", GRHISupportsRectTopology);
    //InfoHelper("GRHISupportsResolveCubemapFaces", GRHISupportsResolveCubemapFaces);
    InfoHelper("GRHISupportsResummarizeHTile", GRHISupportsResummarizeHTile);
    InfoHelper("GRHISupportsRHIOnTaskThread", GRHISupportsRHIOnTaskThread);
    InfoHelper("GRHISupportsRHIThread", GRHISupportsRHIThread);
    InfoHelper("GRHISupportsRWTextureBuffers", GRHISupportsRWTextureBuffers);
    InfoHelper("GRHISupportsSeparateDepthStencilCopyAccess", GRHISupportsSeparateDepthStencilCopyAccess);
    InfoHelper("GRHISupportsShaderTimestamp", GRHISupportsShaderTimestamp);
    InfoHelper("GRHISupportsStencilRefFromPixelShader", GRHISupportsStencilRefFromPixelShader);
    InfoHelper("GRHISupportsTextureStreaming", GRHISupportsTextureStreaming);
    InfoHelper("GRHISupportsUAVFormatAliasing", GRHISupportsUAVFormatAliasing);
    InfoHelper("GRHISupportsUpdateFromBufferTexture", GRHISupportsUpdateFromBufferTexture);
    InfoHelper("GRHISupportsVariableRateShadingAttachmentArrayTextures", GRHISupportsVariableRateShadingAttachmentArrayTextures);
    InfoHelper("GRHISupportsWaveOperations", GRHISupportsWaveOperations);
    //InfoHelper("GRHIThreadId", GRHIThreadId);   //DEPRECATED
    InfoHelper("GRHIThreadNeedsKicking", GRHIThreadNeedsKicking);
    InfoHelper("GRHIThreadTime", GRHIThreadTime);
    InfoHelper("GRHIValidationEnabled", GRHIValidationEnabled);
    InfoHelper("GRHIVariableRateShadingEnabled", GRHIVariableRateShadingEnabled);
    InfoHelper("GRHIVendorId", GRHIVendorId);

    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("Platform Context")) {
    ImGui::BeginDisabled();
    InfoHelper("AllowAudioThread", FGenericPlatformMisc::AllowAudioThread());
    InfoHelper("AllowLocalCaching", FGenericPlatformMisc::AllowLocalCaching());
    InfoHelper("AllowThreadHeartBeat", FGenericPlatformMisc::AllowThreadHeartBeat());
    InfoHelper("CloudDir", FGenericPlatformMisc::CloudDir());
    InfoHelper("DesktopTouchScreen", FGenericPlatformMisc::DesktopTouchScreen());
    InfoHelper("EngineDir", FGenericPlatformMisc::EngineDir());
    InfoHelper("FullscreenSameAsWindowedFullscreen", FGenericPlatformMisc::FullscreenSameAsWindowedFullscreen());
    InfoHelper("GamePersistentDownloadDir", FGenericPlatformMisc::GamePersistentDownloadDir());
    InfoHelper("GameTemporaryDownloadDir", FGenericPlatformMisc::GameTemporaryDownloadDir());
    InfoHelper("GetBatteryLevel", FGenericPlatformMisc::GetBatteryLevel());
    InfoHelper("GetBrightness", FGenericPlatformMisc::GetBrightness());
    InfoHelper("GetCPUBrand", FGenericPlatformMisc::GetCPUBrand());
    InfoHelper("GetCPUChipset", FGenericPlatformMisc::GetCPUChipset());
    InfoHelper("GetCPUInfo", FGenericPlatformMisc::GetCPUInfo());
    InfoHelper("GetCPUVendor", FGenericPlatformMisc::GetCPUVendor());
    InfoHelper("GetDefaultDeviceProfileName", FGenericPlatformMisc::GetDefaultDeviceProfileName());
    InfoHelper("GetDefaultLanguage", FGenericPlatformMisc::GetDefaultLanguage());
    InfoHelper("GetDefaultLocale", FGenericPlatformMisc::GetDefaultLocale());
    InfoHelper("GetDefaultPathSeparator", FGenericPlatformMisc::GetDefaultPathSeparator());
    InfoHelper("GetDeviceId", FGenericPlatformMisc::GetDeviceId());
    InfoHelper("GetDeviceMakeAndModel", FGenericPlatformMisc::GetDeviceMakeAndModel());
    //InfoHelper("GetDeviceOrientation", FGenericPlatformMisc::GetDeviceOrientation());
    InfoHelper("GetDeviceTemperatureLevel", FGenericPlatformMisc::GetDeviceTemperatureLevel());
    InfoHelper("GetDeviceVolume", FGenericPlatformMisc::GetDeviceVolume());
    InfoHelper("GetEngineMode", FGenericPlatformMisc::GetEngineMode());
    InfoHelper("GetEpicAccountId", FGenericPlatformMisc::GetEpicAccountId());
    InfoHelper("GetFileManagerName", FGenericPlatformMisc::GetFileManagerName());
    //InfoHelper("GetHashedMacAddressString", FGenericPlatformMisc::GetHashedMacAddressString());   //DEPRECATED
    InfoHelper("GetLastError", FGenericPlatformMisc::GetLastError());
    InfoHelper("GetLocalCurrencyCode", FGenericPlatformMisc::GetLocalCurrencyCode());
    InfoHelper("GetLocalCurrencySymbol", FGenericPlatformMisc::GetLocalCurrencySymbol());
    InfoHelper("GetLoginId", FGenericPlatformMisc::GetLoginId());
    //InfoHelper("GetMacAddressString", FGenericPlatformMisc::GetMacAddressString());   //DEPRECATED
    //InfoHelper("GetMachineId", FGenericPlatformMisc::GetMachineId().ToString());   //DEPRECATED
    InfoHelper("GetMaxPathLength", FGenericPlatformMisc::GetMaxPathLength());
    InfoHelper("GetMaxRefreshRate", FGenericPlatformMisc::GetMaxRefreshRate());
    InfoHelper("GetMaxSupportedRefreshRate", FGenericPlatformMisc::GetMaxSupportedRefreshRate());
    InfoHelper("GetMaxSyncInterval", FGenericPlatformMisc::GetMaxSyncInterval());
    InfoHelper("GetMobilePropagateAlphaSetting", FGenericPlatformMisc::GetMobilePropagateAlphaSetting());
    //InfoHelper("GetNetworkConnectionType", FGenericPlatformMisc::GetNetworkConnectionType());
    InfoHelper("GetNullRHIShaderFormat", FGenericPlatformMisc::GetNullRHIShaderFormat());
    InfoHelper("GetOperatingSystemId", FGenericPlatformMisc::GetOperatingSystemId());
    InfoHelper("GetOSVersion", FGenericPlatformMisc::GetOSVersion());
    //InfoHelper("GetOSVersions", FGenericPlatformMisc::GetOSVersions());
    InfoHelper("GetPathVarDelimiter", FGenericPlatformMisc::GetPathVarDelimiter());
    InfoHelper("GetPlatformFeaturesModuleName", FGenericPlatformMisc::GetPlatformFeaturesModuleName());
    InfoHelper("GetPrimaryGPUBrand", FGenericPlatformMisc::GetPrimaryGPUBrand());
    InfoHelper("GetTimeZoneId", FGenericPlatformMisc::GetTimeZoneId());
    InfoHelper("GetUBTPlatform", FGenericPlatformMisc::GetUBTPlatform());
    InfoHelper("GetUniqueAdvertisingId", FGenericPlatformMisc::GetUniqueAdvertisingId());
    //InfoHelper("GetUserIndexForPlatformUser", FGenericPlatformMisc::GetUserIndexForPlatformUser());
    InfoHelper("GetUseVirtualJoysticks", FGenericPlatformMisc::GetUseVirtualJoysticks());
    InfoHelper("GetVolumeButtonsHandledBySystem", FGenericPlatformMisc::GetVolumeButtonsHandledBySystem());
    InfoHelper("HasActiveWiFiConnection", FGenericPlatformMisc::HasActiveWiFiConnection());
    InfoHelper("HasMemoryWarningHandler", FGenericPlatformMisc::HasMemoryWarningHandler());
    InfoHelper("HasNonoptionalCPUFeatures", FGenericPlatformMisc::HasNonoptionalCPUFeatures());
    //InfoHelper("HasPlatformFeature", FGenericPlatformMisc::HasPlatformFeature());
    InfoHelper("HasProjectPersistentDownloadDir", FGenericPlatformMisc::HasProjectPersistentDownloadDir());
    InfoHelper("HasSeparateChannelForDebugOutput", FGenericPlatformMisc::HasSeparateChannelForDebugOutput());
    InfoHelper("HasVariableHardware", FGenericPlatformMisc::HasVariableHardware());
    InfoHelper("Is64bitOperatingSystem", FGenericPlatformMisc::Is64bitOperatingSystem());
    InfoHelper("IsDebuggerPresent", FGenericPlatformMisc::IsDebuggerPresent());
    InfoHelper("IsEnsureAllowed", FGenericPlatformMisc::IsEnsureAllowed());
    InfoHelper("IsInLowPowerMode", FGenericPlatformMisc::IsInLowPowerMode());
    InfoHelper("IsLocalPrintThreadSafe", FGenericPlatformMisc::IsLocalPrintThreadSafe());
    InfoHelper("IsPackagedForDistribution", FGenericPlatformMisc::IsPackagedForDistribution());
    InfoHelper("IsPGOEnabled", FGenericPlatformMisc::IsPGOEnabled());
    InfoHelper("IsRegisteredForRemoteNotifications", FGenericPlatformMisc::IsRegisteredForRemoteNotifications());
    InfoHelper("IsRemoteSession", FGenericPlatformMisc::IsRemoteSession());
    InfoHelper("IsRunningInCloud", FGenericPlatformMisc::IsRunningInCloud());
    InfoHelper("IsRunningOnBattery", FGenericPlatformMisc::IsRunningOnBattery());
    InfoHelper("LaunchDir", FGenericPlatformMisc::LaunchDir());
    InfoHelper("NeedsNonoptionalCPUFeaturesCheck", FGenericPlatformMisc::NeedsNonoptionalCPUFeaturesCheck());
    InfoHelper("NumberOfCores", FGenericPlatformMisc::NumberOfCores());
    InfoHelper("NumberOfCoresIncludingHyperthreads", FGenericPlatformMisc::NumberOfCoresIncludingHyperthreads());
    InfoHelper("NumberOfIOWorkerThreadsToSpawn", FGenericPlatformMisc::NumberOfIOWorkerThreadsToSpawn());
    InfoHelper("NumberOfWorkerThreadsToSpawn", FGenericPlatformMisc::NumberOfWorkerThreadsToSpawn());
    InfoHelper("ProjectDir", FGenericPlatformMisc::ProjectDir());
    InfoHelper("SupportsBackbufferSampling", FGenericPlatformMisc::SupportsBackbufferSampling());
    InfoHelper("SupportsBrightness", FGenericPlatformMisc::SupportsBrightness());
    InfoHelper("SupportsDeviceCheckToken", FGenericPlatformMisc::SupportsDeviceCheckToken());
    InfoHelper("SupportsForceTouchInput", FGenericPlatformMisc::SupportsForceTouchInput());
    InfoHelper("SupportsFullCrashDumps", FGenericPlatformMisc::SupportsFullCrashDumps());
    InfoHelper("SupportsLocalCaching", FGenericPlatformMisc::SupportsLocalCaching());
    InfoHelper("SupportsMessaging", FGenericPlatformMisc::SupportsMessaging());
    InfoHelper("SupportsMultithreadedFileHandles", FGenericPlatformMisc::SupportsMultithreadedFileHandles());
    InfoHelper("SupportsTouchInput", FGenericPlatformMisc::SupportsTouchInput());
    InfoHelper("UseHDRByDefault", FGenericPlatformMisc::UseHDRByDefault());
    InfoHelper("UseRenderThread", FGenericPlatformMisc::UseRenderThread());
    ImGui::EndDisabled();
  }

  ImGui::EndTabItem();
}

void FDFX_StatData::Tab_Shaders()
{
  static ImGuiTableFlags flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | 
    ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | 
    ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
    ImGuiTableFlags_SizingStretchProp;
  ImVec2 outer_size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 15);
  float inner_width = ImGui::CalcTextSize("X").x * 80;
  int ShaderTotal = 0;
  double TimeTotal = 0;
  if (ImGui::BeginTable("ShaderCompilerLog", 4, flags, outer_size, inner_width))
  {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Type");
    ImGui::TableSetupColumn("Hash");
    ImGui::TableSetupColumn("Time (ms)");
    ImGui::TableSetupColumn("Count");
    ImGui::TableHeadersRow();
    for (auto ShaderLog : ShaderCompilerLog)
    {
      if (ShaderLog.Count == 0) 
        continue;

      ImGui::TableNextColumn();
      switch(ShaderLog.Type) {
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
      ImGui::TableNextColumn(); ImGui::Text("%s", TCHAR_TO_ANSI(*ShaderLog.Hash.Left(40)));
      ImGui::TableNextColumn(); ImGui::Text("%.5f", ShaderLog.Time);
      ImGui::TableNextColumn(); ImGui::Text("%i", ShaderLog.Count);
      TimeTotal += ShaderLog.Time;
      ShaderTotal++;
    }
    ImGui::EndTable();
    HelpMarker("CS = ComputeShader, GS = GraphicsShader, RT = RayTracing."); ImGui::SameLine();
    ImGui::Text("Total Shaders : %i", ShaderTotal); ImGui::SameLine();
    ImGui::Text(" | Time : %.5f", TimeTotal);
  }


  if (ImGui::CollapsingHeader("r.ShaderPipelineCache Context")) {
    IConsoleManager& m_ConMng = IConsoleManager::Get();
    static bool m_Enabled = m_ConMng.FindConsoleVariable(TEXT("r.ShaderPipelineCache.Enabled"))->GetBool();
    static int32 m_BatchSize = m_ConMng.FindConsoleVariable(TEXT("r.ShaderPipelineCache.BatchSize"))->GetInt();
    static int32 m_BackgroundBatchSize = m_ConMng.FindConsoleVariable(TEXT("r.ShaderPipelineCache.BackgroundBatchSize"))->GetBool();
    static bool m_LogPSO = m_ConMng.FindConsoleVariable(TEXT("r.ShaderPipelineCache.LogPSO"))->GetBool();
    static bool m_SaveAfterPSOsLogged = m_ConMng.FindConsoleVariable(TEXT("r.ShaderPipelineCache.SaveAfterPSOsLogged"))->GetBool();

    ImGui::BeginDisabled();
    InfoHelper("Enabled", m_Enabled);
    InfoHelper("BatchSize", m_BatchSize);
    InfoHelper("BackgroundBatchSize", m_BackgroundBatchSize);
    InfoHelper("LogPSO", m_LogPSO);
    InfoHelper("SaveAfterPSOsLogged", m_SaveAfterPSOsLogged);
    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("ShaderCodeLibrary Context")) {
    ImGui::BeginDisabled();
    InfoHelper("IsEnabled", FShaderCodeLibrary::IsEnabled());
    InfoHelper("GetRuntimeShaderPlatform", (int32)FShaderCodeLibrary::GetRuntimeShaderPlatform());
    InfoHelper("GetShaderCount", FShaderCodeLibrary::GetShaderCount());
    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("PipelineFileCacheManager Context")) {
    ImGui::BeginDisabled();
    InfoHelper("IsPipelineFileCacheEnabled", FPipelineFileCacheManager::IsPipelineFileCacheEnabled());
    InfoHelper("NumPSOsLogged", FPipelineFileCacheManager::NumPSOsLogged());
    InfoHelper("GetGameUsageMask", FPipelineFileCacheManager::GetGameUsageMask());
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
    ImGui::EndTabItem();
}

void FDFX_StatData::Tab_STAT()
{
  for (auto& elem : aStatCmds) {
    elem.Enable = m_Viewport->IsStatEnabled(elem.Command);
  }

  if (ImGui::CollapsingHeader("Favorites")) {
    ImGui::BeginTable("##tblFavBase", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextColumn(); ImGui::TableNextColumn();
    ImGui::BeginTable("##tblFavToggles", 2, ImGuiTableFlags_SizingStretchProp);
    DrawSTAT(EStatHeader::Fav);
    ImGui::EndTable();
    ImGui::EndTable();
  }

  if (ImGui::CollapsingHeader("Common"))  {
    ImGui::BeginTable("##tblCommonBase", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextColumn(); ImGui::TableNextColumn(); 
    ImGui::BeginTable("##tblCommonToggles", 2, ImGuiTableFlags_SizingStretchProp);
    DrawSTAT(EStatHeader::Common);
    ImGui::EndTable();
    ImGui::EndTable();
  }

  if (ImGui::CollapsingHeader("Performance"))  {
    ImGui::BeginTable("##tblPerfBase", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextColumn(); ImGui::TableNextColumn();
    ImGui::BeginTable("##tblPerfToggles", 2, ImGuiTableFlags_SizingStretchProp);
    DrawSTAT(EStatHeader::Perf);
    ImGui::EndTable();
    ImGui::EndTable();
  }

  if (ImGui::CollapsingHeader("List")) {
    static char s_FilterList[128] = "";
    ImGui::Text("Filter : "); ImGui::SameLine(); ImGui::InputText("##ListFilter", s_FilterList, IM_ARRAYSIZE(s_FilterList));
    ImGui::BeginTable("##tblListBase", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(0, 1, 0, 1), "Add To Favorite");
    ImGui::BeginTable("##tblListAddToFav", 1, ImGuiTableFlags_SizingStretchProp);
    char s_ChkId[5] = "";
    bool tmpFav = false;    
    for (int i = 0; i < aStatCmds.Num(); ++i) {
      EStatHeader bFavFlag = static_cast<EStatHeader>(aStatCmds[i].Header | EStatHeader::Fav);
      tmpFav = aStatCmds[i].Header == bFavFlag;
      snprintf(s_ChkId, 5, "##%i", i);
      ImGui::TableNextColumn(); ImGui::Checkbox(s_ChkId, &tmpFav);
      if (tmpFav) {
        aStatCmds[i].Header = bFavFlag;
      } else {
        if (aStatCmds[i].Header == bFavFlag) {
          aStatCmds[i].Header = static_cast<EStatHeader>(aStatCmds[i].Header ^ EStatHeader::Fav);
        }
      }
    }
    ImGui::EndTable();
    ImGui::TableNextColumn(); ImGui::TextColored(ImVec4(0, 1, 0, 1), "STAT Command");
    ImGui::BeginTable("##tblListToggles", 2, ImGuiTableFlags_SizingStretchProp);
    DrawSTAT(EStatHeader::All, FString(s_FilterList));
    ImGui::EndTable();
    ImGui::EndTable();
  }

  if (ImGui::CollapsingHeader("Capture")) {
    ImGui::Text("Starts a statistics capture, creating a new file in the Profiling directory.");
    if (ImGui::Button("StartFile")) {
      m_Viewport->ConsoleCommand("Stat StartFile");;
    }
    if (ImGui::Button("StopFile")) {
      m_Viewport->ConsoleCommand("Stat StopFile");;
    }
  }

  if (ImGui::CollapsingHeader("Extras")) {
    if (ImGui::Button("Stat Help")) {
      m_Viewport->ConsoleCommand("Stat Help"); ImGui::SameLine();
      FDFX_StatData::HelpMarker("List avaiable STAT commands in the OutputLog window.");
    }
  }
  ImGui::EndTabItem();
}

void FDFX_StatData::Tab_Settings()
{
  if (pwThread.History > StatHistoryGlobal) pwThread.History = StatHistoryGlobal;
  if (pwFrame.History > StatHistoryGlobal) pwFrame.History = StatHistoryGlobal;
  if (pwFPS.History > StatHistoryGlobal) pwFPS.History = StatHistoryGlobal;

  if (ImGui::CollapsingHeader("Graphs")) {
    ImGui::Checkbox("Display Graphs", &bShowPlots);
    ImGui::Text("History :"); ImGui::SameLine(); ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::SliderFloat("##HistoryGlobal", &StatHistoryGlobal, 0.1, 10, "%.1f s");

    if (bShowPlots) {
      ImGui::Indent();
      if (ImGui::CollapsingHeader("Threads")) {
        ImGui::Checkbox("Display Threads", &pwThread.bShowPlot);
        ImGui::Checkbox("Sort plot order", &bPlotsSort); ImGui::SameLine();
        FDFX_StatData::HelpMarker("Sort graphs order to display better colors but can cause flickering if two threads have similar values.");
        ImGui::SliderFloat("History##1", &pwThread.History, 0.1, StatHistoryGlobal, "%.1f s");
        ImGui::SliderFloat("Position X##1", &pwThread.Position.x, 0, ViewSize.X - 1, "%.0f px");
        ImGui::SliderFloat("Position Y##1", &pwThread.Position.y, 0, ViewSize.Y - 1, "%.0f px");
        ImGui::SliderFloat("Size X##1", &pwThread.Size.x, 0, ViewSize.X - 1, "%.0f px");
        ImGui::SliderFloat("Size Y##1", &pwThread.Size.y, 0, ViewSize.Y - 1, "%.0f px");
        ImGui::SliderFloat("Range Min##1", &pwThread.Range.x, 0.1, 60, "%.3f ms");
        ImGui::SliderFloat("Range Max##1", &pwThread.Range.y, 0.1, 60, "%.3f ms");
        ImGui::ColorEdit4("Background##1", &pwThread.BackgroundColor.x);
        ImGui::SliderFloat("Plot Alpha##1", &pwThread.PlotStyleFillAlpha, 0, 1, "%.2f");
        ImGui::ColorEdit4("Plot Background##1", &pwThread.PlotBackgroundColor.x);
        if (ImGui::CollapsingHeader("Game##01")) {
          ImGui::Checkbox("Display Game##01", &pwThreadColor[0].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##01", &pwThreadColor[0].PlotLineColor.x);
          ImGui::ColorEdit4("Plot Shade##01", &pwThreadColor[0].PlotShadeColor.x);
        }
        if (ImGui::CollapsingHeader("Render##11")) {
          ImGui::Checkbox("Display Render##11", &pwThreadColor[1].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##11", &pwThreadColor[1].PlotLineColor.x);
          ImGui::ColorEdit4("Plot Shade##11", &pwThreadColor[1].PlotShadeColor.x);
        }
        if (ImGui::CollapsingHeader("GPU##21")) {
          ImGui::Checkbox("Display GPU##21", &pwThreadColor[2].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##21", &pwThreadColor[2].PlotLineColor.x);
          ImGui::ColorEdit4("Plot Shade##21", &pwThreadColor[2].PlotShadeColor.x);
        }
        if (ImGui::CollapsingHeader("RHI##31")) {
          ImGui::Checkbox("Display RHI##31", &pwThreadColor[3].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##31", &pwThreadColor[3].PlotLineColor.x);
          ImGui::ColorEdit4("Plot Shade##31", &pwThreadColor[3].PlotShadeColor.x);
        }
        if (ImGui::CollapsingHeader("Swap##41")) {
          ImGui::Checkbox("Display Swap##41", &pwThreadColor[4].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##41", &pwThreadColor[4].PlotLineColor.x);
          ImGui::ColorEdit4("Plot Shade##41", &pwThreadColor[4].PlotShadeColor.x);
        }
        if (ImGui::CollapsingHeader("Input##51")) {
          ImGui::Checkbox("Display Input##51", &pwThreadColor[5].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##51", &pwThreadColor[5].PlotLineColor.x);
          ImGui::ColorEdit4("Plot Shade##51", &pwThreadColor[5].PlotShadeColor.x);
        }
        if (ImGui::CollapsingHeader("ImGui##61")) {
          ImGui::Checkbox("Display ImGui##61", &pwThreadColor[6].bShowFramePlot);
          ImGui::ColorEdit4("Plot Line##61", &pwThreadColor[6].PlotLineColor.x);
          ImGui::ColorEdit4("Plot Shade##61", &pwThreadColor[6].PlotShadeColor.x);
        }
        ImGui::SliderFloat("Marker Line##1", &pwThread.MarkerLine, 1, 144, "%.3f");
        ImGui::SliderFloat("Marker Thickness##1", &pwThread.MarkerThick, 1, 10, "%.0f");

      }
      if (ImGui::CollapsingHeader("Frame")) {
        ImGui::Checkbox("Display Frame", &pwFrame.bShowPlot);
        ImGui::SliderFloat("History##2", &pwFrame.History, 0.1, StatHistoryGlobal, "%.1f s");
        ImGui::SliderFloat("Position X##2", &pwFrame.Position.x, 0, ViewSize.X - 1, "%.0f px");
        ImGui::SliderFloat("Position Y##2", &pwFrame.Position.y, 0, ViewSize.Y - 1, "%.0f px");
        ImGui::SliderFloat("Size X##2", &pwFrame.Size.x, 0, ViewSize.X - 1, "%.0f px");
        ImGui::SliderFloat("Size Y##2", &pwFrame.Size.y, 0, ViewSize.Y - 1, "%.0f px");
        ImGui::SliderFloat("Range Min##2", &pwFrame.Range.x, 0.1, 60, "%.3f ms");
        ImGui::SliderFloat("Range Max##2", &pwFrame.Range.y, 0.1, 60, "%.3f ms");
        ImGui::ColorEdit4("Background##2", &pwFrame.BackgroundColor.x);
        ImGui::SliderFloat("Plot Alpha##2", &pwFrame.PlotStyleFillAlpha, 0, 1, "%.2f");
        ImGui::ColorEdit4("Plot Background##2", &pwFrame.PlotBackgroundColor.x);
        ImGui::ColorEdit4("Plot Line##2", &pwFrame.PlotLineColor.x);
        ImGui::ColorEdit4("Plot Shade##2", &pwFrame.PlotShadeColor.x);
        ImGui::SliderFloat("Marker Line##2", &pwFrame.MarkerLine, 1, 144, "%.3f");
        ImGui::SliderFloat("Marker Thickness##2", &pwFrame.MarkerThick, 1, 10, "%.0f");
      }
      if (ImGui::CollapsingHeader("FPS")) {
        ImGui::Checkbox("Display FPS", &pwFPS.bShowPlot);
        ImGui::SliderFloat("History##3", &pwFPS.History, 0.1, StatHistoryGlobal, "%.1f s");
        ImGui::SliderFloat("Position X##3", &pwFPS.Position.x, 0, ViewSize.X - 1, "%.0f px");
        ImGui::SliderFloat("Position Y##3", &pwFPS.Position.y, 0, ViewSize.Y - 1, "%.0f px");
        ImGui::SliderFloat("Size X##3", &pwFPS.Size.x, 0, ViewSize.X - 1, "%.0f px");
        ImGui::SliderFloat("Size Y##3", &pwFPS.Size.y, 0, ViewSize.Y - 1, "%.0f px");
        ImGui::SliderFloat("Range Min##3", &pwFPS.Range.x, 0.1, 220, "%.3f ms");
        ImGui::SliderFloat("Range Max##3", &pwFPS.Range.y, 0.1, 240, "%.3f ms");
        ImGui::ColorEdit4("Background##3", &pwFPS.BackgroundColor.x);
        ImGui::SliderFloat("Plot Alpha##3", &pwFPS.PlotStyleFillAlpha, 0, 1, "%.2f");
        ImGui::ColorEdit4("Plot Background##3", &pwFPS.PlotBackgroundColor.x);
        ImGui::ColorEdit4("Plot Line##3", &pwFPS.PlotLineColor.x);
        ImGui::ColorEdit4("Plot Shade##3", &pwFPS.PlotShadeColor.x);
        ImGui::SliderFloat("Marker Line##3", &pwFPS.MarkerLine, 1, 144, "%.3f");
        ImGui::SliderFloat("Marker Thickness##3", &pwFPS.MarkerThick, 1, 10, "%.0f");
      }
      ImGui::Unindent();
    }
  }

  if (ImGui::CollapsingHeader("Advanced")) {
    //ImGui::Checkbox("Use external window", &bExternalWindow);
    ImGui::Checkbox("Disable in-game controls", &bDisableGameControls);
    ImGui::Checkbox("Show Debug Tab", &bShowDebugTab);
    if (ImGui::Button("Reset DFoundryFX")) {
      ImGui::ClearIniSettings();
      bIsDefaultLoaded = false;
      LoadDefaultValues(ViewSize);
    }
    if (ImGui::Button("Reset ImGui Settings"))
      ImGui::ClearIniSettings();
  }  

  ImGui::EndTabItem();
}

void FDFX_StatData::Tab_Debug()
{
  char s_EnabledStats[32];
  char s_Hitches[32];

  snprintf(s_EnabledStats, 32, "Enabled Stats : %i", m_Viewport->GetEnabledStats()->Num());
  snprintf(s_Hitches, 32, "Hitches : %i", m_Viewport->GetStatHitchesData()->Count);
  
  //ImGui::BeginTabItem("Debug");
    ImGui::Text("FPS : %2d", m_FramesPerSecond);
    ImGui::Text("Frame : %4.3f", m_FrameTime);
    ImGui::Text("Game : %4.3f", m_GameThreadTime);
    ImGui::Text("Render : %4.3f", m_RenderThreadTime);
    ImGui::Text("GPU : %4.3f", m_GPUFrameTime);
    ImGui::Text("RHI : %4.3f", m_RHIThreadTime);
    ImGui::Text("Swap : %4.3f", m_SwapBufferTime);
    ImGui::Text("Input : %4.3f", m_InputLatencyTime);
    ImGui::Text("ImGui : %4.3f", m_ImGuiThreadTime);

    if (ImGui::CollapsingHeader(s_Hitches)) {
      ImGui::Indent();
      ImGui::Text("Last Time : %4.3f", m_Viewport->GetStatHitchesData()->LastTime);
      for (int i=0; i < m_Viewport->GetStatHitchesData()->LastTime; ++i) {
        ImGui::Text("Hitch (ms): %4.3f##%i", m_Viewport->GetStatHitchesData()->Hitches[i], i);
      }
      ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader(s_EnabledStats)) {
      ImGui::Indent();
      //const TArray<FString>* EnabledStats = m_Viewport->GetEnabledStats();
      FString JoinedStr;
      for (auto It = m_Viewport->GetEnabledStats()->CreateConstIterator(); It; ++It) {
        JoinedStr += *It;
        JoinedStr += TEXT(" \n");
      }
      ImGui::Text(TCHAR_TO_ANSI(*JoinedStr));
      ImGui::Unindent();
    }
    ImGui::Text("Frame Count: %d", m_FrameCount);
    ImGui::Text("ImGui Frame Count: %i", static_cast<int32>(ImPlotFrameCount));
    ImGui::Text("Current Time: %f", m_CurrentTime);
    ImGui::Text("Last Time: %f", m_LastTime);
    ImGui::Text("Delta Time: %f", m_DiffTime);
    ImGui::Text("FPS float: %f", 1000 / m_DiffTime);

  ImGui::EndTabItem();
}

void FDFX_StatData::LoadDefaultValues(FVector2D InViewportSize)
{
  ViewSize = InViewportSize;
  static FVector2D oldViewSize;
  if (bIsDefaultLoaded) {
    if (oldViewSize == InViewportSize) {
      const double winSize = InViewportSize.X - InViewportSize.X / 4;
      if (bMainWindowOpen) {
        if (pwFPS.Size.x == InViewportSize.X) {
          pwFPS.Size.x = winSize;
        }
      } else {
        if (pwFPS.Size.x == winSize) {
          pwFPS.Size.x = InViewportSize.X;
        }
      }
      return;
    } else {
      pwThread.Position = ImVec2(InViewportSize.X * (pwThread.Position.x / oldViewSize.X), InViewportSize.Y * (pwThread.Position.y / oldViewSize.Y));
      pwThread.Size = ImVec2(InViewportSize.X * (pwThread.Size.x / oldViewSize.X), InViewportSize.Y * (pwThread.Size.y / oldViewSize.Y));
      pwFrame.Position = ImVec2(InViewportSize.X * (pwFrame.Position.x / oldViewSize.X), InViewportSize.Y * (pwFrame.Position.y / oldViewSize.Y));
      pwFrame.Size = ImVec2(InViewportSize.X * (pwFrame.Size.x / oldViewSize.X), InViewportSize.Y * (pwFrame.Size.y / oldViewSize.Y));
      pwFPS.Position = ImVec2(InViewportSize.X * (pwFPS.Position.x / oldViewSize.X), InViewportSize.Y * (pwFPS.Position.y / oldViewSize.Y));
      pwFPS.Size = ImVec2(InViewportSize.X * (pwFPS.Size.x / oldViewSize.X), InViewportSize.Y * (pwFPS.Size.y / oldViewSize.Y));
      oldViewSize = ViewSize;
      return;
    }
  }

  pwThread.bShowPlot = true;
  pwThread.History = 3.0f;
  pwThread.Range = ImVec2(0, 20);
  pwThread.Position = ImVec2(0, 0);
  pwThread.Size = ImVec2(ViewSize.X / 4, ViewSize.Y / 3);
  pwThread.BackgroundColor = ImVec4(0.21, 0.22, 0.23, 0.05);
  pwThread.PlotBackgroundColor = ImVec4(0.32, 0.50, 0.77, 0.05);
  pwThread.PlotStyleFillAlpha = 0.5;
  pwThreadColor[0].bShowFramePlot = true;
  pwThreadColor[0].PlotLineColor = ImVec4(0.75, 0.196, 0.196, 1.0);
  pwThreadColor[0].PlotShadeColor = ImVec4(1.0, 0.392, 0.392, 1.0);
  pwThreadColor[1].bShowFramePlot = true;
  pwThreadColor[1].PlotLineColor = ImVec4(0.184, 0.184, 0.466, 1.0);
  pwThreadColor[1].PlotShadeColor = ImVec4(0.369, 0.369, 0.933, 1.0);
  pwThreadColor[2].bShowFramePlot = true;
  pwThreadColor[2].PlotLineColor = ImVec4(0.75, 0.75, 0.192, 1.0);
  pwThreadColor[2].PlotShadeColor = ImVec4(1.0, 1.0, 0.392, 1.0);
  pwThreadColor[3].bShowFramePlot = true;
  pwThreadColor[3].PlotLineColor = ImVec4(0.622, 0.184, 0.622, 1.0);
  pwThreadColor[3].PlotShadeColor = ImVec4(0.933, 0.369, 0.933, 1.0);
  pwThreadColor[4].bShowFramePlot = true;
  pwThreadColor[4].PlotLineColor = ImVec4(0.196, 0.75, 0.75, 1.0);
  pwThreadColor[4].PlotShadeColor = ImVec4(0.392, 1.0, 1.0, 1);
  pwThreadColor[5].bShowFramePlot = true;
  pwThreadColor[5].PlotLineColor = ImVec4(0.196, 0.75, 0.333, 1.0);
  pwThreadColor[5].PlotShadeColor = ImVec4(0.392, 1.0, .5, 1);
  pwThreadColor[6].bShowFramePlot = true;
  pwThreadColor[6].PlotLineColor = ImVec4(0.080, 0.145, 0.318, 1.0);
  pwThreadColor[6].PlotShadeColor = ImVec4(0.161, 0.29, 0.478, 1);
  pwThread.MarkerColor = ImVec4(0.0, 0.25, 0.0, 1.0);
  pwThread.MarkerLine = 16.667;
  pwThread.MarkerThick = 1;

  pwFrame.bShowPlot = true;
  pwFrame.History = 3.0f;
  pwFrame.Range = ImVec2(8, 24);
  pwFrame.Position = ImVec2(0, (ViewSize.Y / 3) * 1);
  pwFrame.Size = ImVec2(ViewSize.X / 4, ViewSize.Y / 3);
  pwFrame.BackgroundColor = ImVec4(0.21, 0.22, 0.23, 0.05);
  pwFrame.PlotBackgroundColor = ImVec4(0.32, 0.50, 0.77, 0.05);
  pwFrame.PlotStyleFillAlpha = 0.5;
  pwFrame.PlotLineColor = ImVec4(0.161, 0.29, 0.478, 1);
  pwFrame.PlotShadeColor = ImVec4(0.298, 0.447, 0.69, 1);
  pwFrame.MarkerColor = ImVec4(0.0, 0.25, 0.0, 1.0);
  pwFrame.MarkerLine = 16.667;
  pwFrame.MarkerThick = 1;

  pwFPS.bShowPlot = true;
  pwFPS.History = 3.0f;
  pwFPS.Range = ImVec2(20, 80);
  pwFPS.Position = ImVec2(0, (ViewSize.Y / 3) * 2);
  pwFPS.Size = ImVec2(ViewSize.X, ViewSize.Y / 3);
  pwFPS.BackgroundColor = ImVec4(0.21, 0.22, 0.23, 0.05);
  pwFPS.PlotBackgroundColor = ImVec4(0.32, 0.50, 0.77, 0.05);
  pwFPS.PlotStyleFillAlpha = 0.5;
  pwFPS.PlotLineColor = ImVec4(0.161, 0.29, 0.478, 1);
  pwFPS.PlotShadeColor = ImVec4(0.298, 0.447, 0.69, 1);
  pwFPS.MarkerColor = ImVec4(0.0, 0.25, 0.0, 1.0);
  pwFPS.MarkerLine = 60;
  pwFPS.MarkerThick = 1;

  bShowPlots = true;
  bPlotsSort = false;
  bShowDebugTab = true;
  bDisableGameControls = false;
  StatHistoryGlobal = 10;

  ImPlotFrameCount = 0;
  HistoryTime.Erase();
  FrameCount.Erase();
  FrameTime.Erase();
  FramesPerSecond.Erase();
  GameThreadTime.Erase();
  RenderThreadTime.Erase();
  GPUFrameTime.Erase();
  RHIThreadTime.Erase();
  SwapBufferTime.Erase();
  InputLatencyTime.Erase();
  ImGuiThreadTime.Erase();

  for (auto elem : aStatCmds) {
    if (elem.Header == FDFX_StatData::Fav)
      elem.Header = FDFX_StatData::None;
  }

  UpdateStats();

  bIsDefaultLoaded = true;
  oldViewSize = ViewSize;
}

void FDFX_StatData::LoadSTAT(FDFX_StatData::EStatHeader InHeader, FString InList)
{
  TArray<FString> aList;
  FStatCmd aBase;
  switch (int(InHeader)) {
    case 1: //EStatHeader::None
      aBase.Header = EStatHeader::None;
      aBase.Enable = false;
      aBase.Command = "";
      aStatCmds.Init(aBase, aList.Num());
      InList.ParseIntoArray(aList, TEXT(","), true);
      for (auto& elem : aList) {
        aBase.Command = elem;
        aStatCmds.Add(aBase);
      }
      break;
    case 2: //EStatHeader::Common
    case 3: //EStatHeader::Perf
      InList.ParseIntoArray(aList, TEXT(","), true);
      for (auto elem : aList) {
        for (auto& ecmd : aStatCmds) {
          if (ecmd.Command == elem) {
            ecmd.Header = InHeader;
            break;
          }
        }
      }
      break;
  }
}

void FDFX_StatData::LoadCVAR()
{
}

void FDFX_StatData::DrawSTAT(FDFX_StatData::EStatHeader InHeader, FString InFilter)
{
  bool tmpToggle = false;
  
  for (auto& elem : aStatCmds) {
    EStatHeader bFavFlag = static_cast<EStatHeader>(elem.Header | EStatHeader::Fav);
    if (InFilter != "" && !elem.Command.Contains(InFilter)) continue;
    if (elem.Header == InHeader || 
      elem.Header == bFavFlag ||
      InHeader == EStatHeader::All) {
      char s_StatCmd[32];
      char s_StatId[32];
      char* chrCmd = TCHAR_TO_ANSI(*elem.Command);
      snprintf(s_StatCmd, 32, "Stat %s", chrCmd);
      snprintf(s_StatId, 32, "Stat_%s", chrCmd);
      ImGui::TableNextColumn(); ImGui::Text(s_StatCmd);
      tmpToggle = elem.Enable;
      ImGui::TableNextColumn(); ToggleButton(s_StatId, &tmpToggle);
      ToggleStat(elem.Command, tmpToggle);
      elem.Enable = tmpToggle;
    }
  }
}

void FDFX_StatData::LoadDemos()
{
  if (ImGui::Begin("Hello Widget", nullptr, ImGuiWindowFlags_None)) {
    static int32 ClickCount = 0;
    if (ImGui::Button("Click")) {
      ++ClickCount;
    }
    ImGui::Text("ClickCount: %i", ClickCount);
  }
  ImGui::End();

  ImGui::ShowDemoWindow();
  ImPlot::ShowDemoWindow();
}

void FDFX_StatData::AddShaderLog(int Type, FString Hash, double Time)
{
  bool bShaderLogExist = false;
  for (auto& ShaderLog : ShaderCompilerLog)
  {
    if (ShaderLog.Hash.Left(40) == Hash.Left(40)) {
      ShaderLog.Type = ShaderLog.Type | Type;
      ShaderLog.Time = ShaderLog.Time + Time;
      ShaderLog.Count = ShaderLog.Count + 1;
      bShaderLogExist = true;
      break;
    }
  }
  if (!bShaderLogExist) {
    FShaderCompilerLog NewItem;
    NewItem.Type = Type;
    NewItem.Hash = Hash;
    NewItem.Time = Time;
    NewItem.Count = 1;
    ShaderCompilerLog.Add(NewItem);
  }
}

void FDFX_StatData::ToggleButton(const char* str_id, bool* v)
{
  ImVec4* colors = ImGui::GetStyle().Colors;
  ImVec2 p = ImGui::GetCursorScreenPos();
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  float height = ImGui::GetFrameHeight();
  float width = height * 1.55f;
  float radius = height * 0.50f;
  float rounding = 0.2f;

  ImGui::InvisibleButton(str_id, ImVec2(width, height));
  if (ImGui::IsItemClicked()) *v = !*v;
  ImGuiContext& gg = *GImGui;
  float ANIM_SPEED = 0.085f;
  if (gg.LastActiveId == gg.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
    float t_anim = ImSaturate(gg.LastActiveIdTimer / ANIM_SPEED);
  if (ImGui::IsItemHovered())
    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_ButtonActive] : ImVec4(0.78f, 0.78f, 0.78f, 1.0f)), height * rounding);
  else
    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_Button] : ImVec4(0.85f, 0.85f, 0.85f, 1.0f)), height * rounding);

  ImVec2 center = ImVec2(radius + (*v ? 1 : 0) * (width - radius * 2.0f), radius);
  draw_list->AddRectFilled(ImVec2((p.x + center.x) - 9.0f, p.y + 1.5f),
    ImVec2((p.x + (width / 2) + center.x) - 9.0f, p.y + height - 1.5f), IM_COL32(255, 255, 255, 255), height * rounding);
}

void FDFX_StatData::ToggleStat(FString StatName, bool& bValue)
{
  bool bStatEnable = m_Viewport->IsStatEnabled(StatName);
  if (bValue && !bStatEnable ||
     !bValue && bStatEnable) {
    FString sCmd = FString("Stat ").Append(StatName);
    m_Viewport->ConsoleCommand(sCmd);
  }
}

void FDFX_StatData::HelpMarker(const char* desc)
{
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

void FDFX_StatData::ThreadMarker(int PlotColorId)
{
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  ImVec2 pos = ImGui::GetCursorScreenPos();
  ImVec2 marker_min = ImVec2(pos.x, pos.y - 2);
  ImVec2 marker_max = ImVec2(pos.x + 8, pos.y + ImGui::GetTextLineHeight() + 2);
  draw_list->AddRect(marker_min, marker_max, ImColor(pwThreadColor[PlotColorId].PlotLineColor));
  draw_list->AddRectFilled(marker_min, marker_max, ImColor(pwThreadColor[PlotColorId].PlotShadeColor));
}

void FDFX_StatData::InfoHelper(FString InInfo, bool InValue) 
{
  bool tmpbChk = InValue;
  ImGui::Checkbox(TCHAR_TO_ANSI(*InInfo), &tmpbChk);
}

void FDFX_StatData::InfoHelper(FString InInfo, int32 InValue) 
{
  bool tmpbChk = false;
  int32 tmpiChk = InValue;
  InInfo.Append(" - %d");
  ImGui::Checkbox("", &tmpbChk); ImGui::SameLine();
  ImGui::Text(TCHAR_TO_ANSI(*InInfo), tmpiChk);
}

void FDFX_StatData::InfoHelper(FString InInfo, uint32 InValue) 
{
  bool tmpbChk = false;
  uint32 tmpuiChk = InValue;
  InInfo.Append(" - %u");
  ImGui::Checkbox("", &tmpbChk); ImGui::SameLine();
  ImGui::Text(TCHAR_TO_ANSI(*InInfo), tmpuiChk);
}

void FDFX_StatData::InfoHelper(FString InInfo, uint64 InValue)
{
  bool tmpbChk = false;
  uint64 tmpuiChk = InValue;
  InInfo.Append(" - %u");
  ImGui::Checkbox("", &tmpbChk); ImGui::SameLine();
  ImGui::Text(TCHAR_TO_ANSI(*InInfo), tmpuiChk);
}

void FDFX_StatData::InfoHelper(FString InInfo, float InValue) 
{
  bool tmpbChk = false;
  float tmpfChk = InValue;
  InInfo.Append(" - %f");
  ImGui::Checkbox("", &tmpbChk); ImGui::SameLine();
  ImGui::Text(TCHAR_TO_ANSI(*InInfo), tmpfChk);
}

void FDFX_StatData::InfoHelper(FString InInfo, double InValue)
{
  bool tmpbChk = false;
  double tmpdChk = InValue;
  InInfo.Append(" - %f");
  ImGui::Checkbox("", &tmpbChk); ImGui::SameLine();
  ImGui::Text(TCHAR_TO_ANSI(*InInfo), tmpdChk);
}

void FDFX_StatData::InfoHelper(FString InInfo, FString InValue)
{
  bool tmpbChk = false;
  InInfo.Append(" - %s");
  ImGui::Checkbox("", &tmpbChk); ImGui::SameLine();
  ImGui::Text(TCHAR_TO_ANSI(*InInfo), TCHAR_TO_ANSI(*InValue));
}

void FDFX_StatData::InfoHelper(FString InInfo, FText InValue)
{
  bool tmpbChk = false;
  InInfo.Append(" - %s");
  ImGui::Checkbox("", &tmpbChk); ImGui::SameLine();
  ImGui::Text(TCHAR_TO_ANSI(*InInfo), TCHAR_TO_ANSI(*InValue.ToString()));
}

void FDFX_StatData::InfoHelper(FString InInfo, const TCHAR* InValue)
{
  bool tmpbChk = false;
  InInfo.Append(" - %s");
  ImGui::Checkbox("", &tmpbChk); ImGui::SameLine();
  ImGui::Text(TCHAR_TO_ANSI(*InInfo), TCHAR_TO_ANSI(InValue));
}
#undef LOCTEXT_NAMESPACE