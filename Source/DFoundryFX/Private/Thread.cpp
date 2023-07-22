#include "Thread.h"

#define LOCTEXT_NAMESPACE "DFX_Thread"

DECLARE_CYCLE_STAT(TEXT("DFoundryFX_ThreadProcEvents"), STAT_ThreadProcEvents, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_ThreadNewFrame"), STAT_ThreadNewFrame, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_ThreadRender"), STAT_ThreadRender, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_ThreadDraw"), STAT_ThreadDraw, STATGROUP_DFoundryFX);

FDFX_Thread::FDFX_Thread()
{
  // Events
  hOnGameModeInitialized = FDelegateHandle();
  hOnWorldBeginPlay = FDelegateHandle();
  hOnHUDPostRender = FDelegateHandle();

  // Thread
  UE_LOG(LogDFoundryFX, Log, TEXT("Thread: Initializing DFoundryFX multithread."));
  DFoundryFX_Thread = FRunnableThread::Create(this, TEXT("DFoundryFX_Thread"), 128 * 1024, FPlatformAffinity::GetTaskThreadPriority(),
    FPlatformAffinity::GetNoAffinityMask(), EThreadCreateFlags::None
  );
}

FDFX_Thread::~FDFX_Thread()
{
  UE_LOG(LogDFoundryFX, Log, TEXT("Thread: Destroying DFoundryFX multithread."));

  RemoveDelegates();

  // Thread
  if (DFoundryFX_Thread != nullptr)
  {
    DFoundryFX_Thread->Kill(true);
    delete DFoundryFX_Thread;
  }
}

bool FDFX_Thread::Init()
{
  hOnGameModeInitialized = FGameModeEvents::OnGameModeInitializedEvent().AddRaw(this, &FDFX_Thread::OnGameModeInitialized);
  hOnViewportCreated = UGameViewportClient::OnViewportCreated().AddRaw(this, &FDFX_Thread::OnViewportCreated);

  // Ignore when Engine still loading
  if (GEngine && GEngine->GetWorldContexts().Num() != 0) {
    // Restarting FXThread on a existing viewport.
    int WorldContexts = GEngine->GetWorldContexts().Num();
    for (int i = 0; i < WorldContexts; i++) {
      if (GEngine->GetWorldContexts()[i].World()->IsGameWorld()) {
        OnGameModeInitialized(UGameplayStatics::GetGameMode(GEngine->GetWorldContexts()[i].World()));
        OnWorldBeginPlay();
        break;
      }
    }
  }

  return true;
}

// Events -> GameMode and Viewport
void FDFX_Thread::OnGameModeInitialized(AGameModeBase* aGameMode)
{
  // ImGui
  UE_LOG(LogDFoundryFX, Log, TEXT("Thread: Initializing ImGui resources and context."));
  m_ImGuiContext = ImGui::CreateContext();
  m_ImPlotContext = ImPlot::CreateContext();
  ImGui_ImplUE_CreateDeviceObjects();
  ImGui_ImplUE_Init();

  GameMode = aGameMode;
  uWorld = GameMode->GetWorld();
  GameViewport = uWorld->GetGameViewport();

  // Cleanup if PIE or second window.
  RemoveDelegates();

  hOnWorldBeginPlay = uWorld->OnWorldBeginPlay.AddRaw(this, &FDFX_Thread::OnWorldBeginPlay);
  hOnViewportClose = GameViewport->GetWindow()->GetOnWindowClosedEvent().AddLambda(
  [this](const TSharedRef<SWindow>& Window) {
    FDFX_Thread::OnViewportClose();
  });
  hOnPipelineStateLogged = FPipelineFileCacheManager::OnPipelineStateLogged().AddRaw(this, &FDFX_Thread::OnPipelineStateLogged);
  ShaderLogTime = FApp::GetCurrentTime();
}


void FDFX_Thread::OnWorldBeginPlay()
{
  ViewportSize = FVector2D::ZeroVector;
  bExternalOpened = false;

  // Fix for GameMode HUD Class = None
  PlayerController = uWorld->GetFirstPlayerController();
  if (!GameMode->HUDClass)
  {
    PlayerController->SpawnDefaultHUD();
  }

  // Fix for new PIE window
  if (hOnHUDPostRender.IsValid()) {
    if (PlayerController->GetHUD()->OnHUDPostRender.IsBound())
    {
      PlayerController->GetHUD()->OnHUDPostRender.Remove(hOnHUDPostRender);
      hOnHUDPostRender.Reset();
    }
  }

  if(!hOnViewportClose.IsValid()) {
    hOnViewportClose = GameViewport->GetWindow()->GetOnWindowClosedEvent().AddLambda(
      [this](const TSharedRef<SWindow>& Window) {
        FDFX_Thread::OnViewportClose();
      });
  }

  if (PlayerController) {
    hOnHUDPostRender = PlayerController->GetHUD()->OnHUDPostRender.AddRaw(this, &FDFX_Thread::OnHUDPostRender);
  }
}

void FDFX_Thread::OnHUDPostRender(AHUD* HUD, UCanvas* Canvas)
{
  if (bStopping) { return; }

  ViewportSize = FVector2D(Canvas->SizeX, Canvas->SizeY);
  uCanvas = Canvas;
  FDFX_Thread::ImGui_ImplUE_Render();
}

void FDFX_Thread::OnViewportCreated()
{
}

bool FDFX_Thread::OnViewportClose()
{
  //ExternalWindow(true);

  RemoveDelegates();

  // ImGui
  if (m_ImGuiContext) {
    ImPlot::DestroyContext(m_ImPlotContext);
    ImGui::DestroyContext(m_ImGuiContext);

    m_ImPlotContext = nullptr;
    m_ImGuiContext = nullptr;
  }

  ViewportSize = FVector2D::ZeroVector;
  m_ImGuiDiffTime = 0;

  // Hook for the next viewport/PIE
  hOnGameModeInitialized = FGameModeEvents::OnGameModeInitializedEvent().AddRaw(this, &FDFX_Thread::OnGameModeInitialized);
  hOnViewportCreated = UGameViewportClient::OnViewportCreated().AddRaw(this, &FDFX_Thread::OnViewportCreated);

  return true;
}

void FDFX_Thread::OnPipelineStateLogged(FPipelineCacheFileFormatPSO& PipelineCacheFileFormatPSO)
{
  FString AssetName = "";
  double m_ShaderLogDiff = FApp::GetCurrentTime() - ShaderLogTime;
  uint32 m_hash = PipelineCacheFileFormatPSO.Hash;
  uint32 m_type = (int)PipelineCacheFileFormatPSO.Type;

  switch(m_type) {
    case 0:  //Compute
      FDFX_StatData::AddShaderLog(1, 
        *PipelineCacheFileFormatPSO.ComputeDesc.ComputeShader.ToString(), 
        m_ShaderLogDiff);
      break;
    case 1:  //Graphics
      FDFX_StatData::AddShaderLog(2,
        *PipelineCacheFileFormatPSO.GraphicsDesc.ShadersToString(),
        m_ShaderLogDiff);
      break;
    case 2:  //Raytracing
      FDFX_StatData::AddShaderLog(4,
        *PipelineCacheFileFormatPSO.RayTracingDesc.ShaderHash.ToString(),
        m_ShaderLogDiff);
      break;
  }
  ShaderLogTime = FApp::GetCurrentTime();
}


ImGuiIO& FDFX_Thread::GetImGuiIO() const
{
  checkf(m_ImGuiContext, TEXT("ImGuiContext is invalid!"));

  ImGui::SetCurrentContext(m_ImGuiContext);
  ImPlot::SetCurrentContext(m_ImPlotContext);
  return ImGui::GetIO();
}

bool FDFX_Thread::ImGui_ImplUE_Init()
{
  ImGuiIO& io = GetImGuiIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

  ImGui::StyleColorsDark();

  io.SetClipboardTextFn = ImGui_ImplUE_SetClipboardText;
  io.GetClipboardTextFn = ImGui_ImplUE_GetClipboardText;

  return true;
}


bool FDFX_Thread::ImGui_ImplUE_CreateDeviceObjects()
{
  // Build texture atlas
  ImGuiIO& io = GetImGuiIO();
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  if (!FDFX_Module::FontTexture_Updated) {
    AsyncTask(ENamedThreads::GameThread, [=](){
      UE_LOG(LogDFoundryFX, Log, TEXT("ImGui FontTexture : TexData %d x %d."), width, height);
  
      FDFX_Module::FontTexture->UnlinkStreaming();
      FTexture2DMipMap& mip = FDFX_Module::FontTexture->GetPlatformData()->Mips[0];
      void* data = mip.BulkData.Lock(LOCK_READ_WRITE);
      int size = width * height * 4; // Fix lnt-arithmetic-overflow warning
      FMemory::Memcpy(data, pixels, size);
      mip.BulkData.Unlock();

      FDFX_Module::FontTexture->UpdateResource();
      FDFX_Module::MaterialInstance->SetTextureParameterValue(FName("param"), FDFX_Module::FontTexture);
      FDFX_Module::MaterialInstance->CacheShaders(EMaterialShaderPrecompileMode::Synchronous);
      //FDFX_Module::MaterialInstance->AllMaterialsCacheResourceShadersForRendering(); DX12 ValidateBoundUniformBuffer Crash
      FDFX_Module::FontTexture_Updated = true;
    });
  }

  // Store our identifier
  io.Fonts->TexID = (void*)FDFX_Module::FontTexture;

  UE_LOG(LogDFoundryFX, Log, TEXT("ImGui FontTexture loaded %d x %d.."), width, height);

  return true;
}

void FDFX_Thread::ImGui_ImplUE_Render()
{
  const uint64 m_ImGuiBeginTime = FPlatformTime::Cycles64();
  { 
    SCOPE_CYCLE_COUNTER(STAT_ThreadProcEvents);
    ImGui_ImplUE_ProcessEvent();
  }
  { 
    SCOPE_CYCLE_COUNTER(STAT_ThreadNewFrame);
    ImGui_ImplUE_NewFrame();
  }
  FDFX_StatData::RunDFoundryFX(GameViewport, m_ImGuiDiffTime * 1000);
  { 
    SCOPE_CYCLE_COUNTER(STAT_ThreadRender);
    ImGui::Render();
  }
  { 
    SCOPE_CYCLE_COUNTER(STAT_ThreadDraw);
    ImGui_ImplUE_RenderDrawLists();
  }
  const uint64 m_ImGuiEndTime = FPlatformTime::Cycles64();
  m_ImGuiDiffTime = (m_ImGuiEndTime - m_ImGuiBeginTime);
}

void FDFX_Thread::ImGui_ImplUE_ProcessEvent()
{
  if (!ControllerInput())
    return;

  ImGuiIO& io = GetImGuiIO();

  io.KeyShift = PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift);
  io.KeyCtrl = PlayerController->IsInputKeyDown(EKeys::LeftControl) || PlayerController->IsInputKeyDown(EKeys::RightControl);
  io.KeyAlt = PlayerController->IsInputKeyDown(EKeys::LeftAlt) || PlayerController->IsInputKeyDown(EKeys::RightAlt);
  io.KeySuper = false;

  TArray<FKey> keys;
  EKeys::GetAllKeys(keys);
  for (int i = 0; i < keys.Num(); i++)
  {
    const ImGuiKey m_ImGuiKey = FKeyToImGuiKey(keys[i].GetFName());
    if (PlayerController->IsInputKeyDown(keys[i])) {
      if (m_ImGuiKey != ImGuiKey_None) {
        io.AddKeyEvent(m_ImGuiKey, true);
      }
    } else {
      io.AddKeyEvent(m_ImGuiKey, false);
    }

    bool keyboard = !keys[i].IsMouseButton() &&
      !keys[i].IsModifierKey() &&
      !keys[i].IsGamepadKey() &&
      !keys[i].IsAxis1D() &&
      !keys[i].IsAxis2D();
    if (!keyboard)
      continue;

    if (PlayerController->WasInputKeyJustPressed(keys[i]))
    {
      const uint32* key_code = NULL;
      const uint32* char_code = NULL;
      FInputKeyManager::Get().GetCodesFromKey(keys[i], key_code, char_code);

      if (char_code)
      {
        int c = tolower((int)*char_code);
        if (PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift))
          c = toupper(c);
        io.AddInputCharacter((ImWchar)c);
      }
    }
  }

  ControllerInput();
  //ExternalWindow();
}

void FDFX_Thread::ImGui_ImplUE_NewFrame()
{
  ImGuiIO& io = GetImGuiIO();
  
  io.DisplaySize = ImVec2((float)ViewportSize.X, (float)ViewportSize.Y);
  io.DisplayFramebufferScale = ImVec2(1, 1);

  PlayerController->GetMousePosition(io.MousePos.x, io.MousePos.y);
  io.MouseDown[0] = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
  io.MouseDown[1] = PlayerController->IsInputKeyDown(EKeys::RightMouseButton);
  io.MouseDown[2] = PlayerController->IsInputKeyDown(EKeys::MiddleMouseButton);

  //TODO : Add MouseWheelAxis
  //io.AddMouseWheelEvent(0.f, PlayerController->IsInputKeyDown(EKeys::MouseWheelAxis));

  ImGui::NewFrame();
}

void FDFX_Thread::ImGui_ImplUE_RenderDrawLists()
{
  // Avoid rendering when minimized
  ImGuiIO& io = GetImGuiIO();
  int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
  int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
  if ((fb_width == 0) || (fb_height == 0))
    return;

  ImDrawData* draw_data = ImGui::GetDrawData();
  draw_data->ScaleClipRects(io.DisplayFramebufferScale);

  // Render command lists
  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
    const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
    {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      TArray<FCanvasUVTri> triangles;
      for (unsigned int elem = 0; elem < pcmd->ElemCount / 3; elem++)
      {
        ImDrawVert v[] =
        {
          cmd_list->VtxBuffer[idx_buffer[elem * 3]],
          cmd_list->VtxBuffer[idx_buffer[elem * 3 + 1]],
          cmd_list->VtxBuffer[idx_buffer[elem * 3 + 2]]
        };

        ImVec4 col[] =
        {
          ImGui::ColorConvertU32ToFloat4(v[0].col),
          ImGui::ColorConvertU32ToFloat4(v[1].col),
          ImGui::ColorConvertU32ToFloat4(v[2].col)
        };

        ImVec2 min_pos = v[0].pos;
        ImVec2 max_pos = v[0].pos;
        for (int i = 0; i < 3; i++)
        {
          if (v[i].pos.x < min_pos.x)
            min_pos.x = v[i].pos.x;
          if (v[i].pos.y < min_pos.y)
            min_pos.y = v[i].pos.y;
          if (v[i].pos.x > max_pos.x)
            max_pos.x = v[i].pos.x;
          if (v[i].pos.y > max_pos.y)
            max_pos.y = v[i].pos.y;
        }

        ImVec2 min_uv = v[0].uv;
        ImVec2 max_uv = v[0].uv;
        for (int i = 0; i < 3; i++)
        {
          if (v[i].uv.x < min_uv.x)
            min_uv.x = v[i].uv.x;
          if (v[i].uv.y < min_uv.y)
            min_uv.y = v[i].uv.y;
          if (v[i].uv.x > max_uv.x)
            max_uv.x = v[i].uv.x;
          if (v[i].uv.y > max_uv.y)
            max_uv.y = v[i].uv.y;
        }

        for (int i = 0; i < 3; i++)
        {
          if (v[i].pos.x < pcmd->ClipRect.x)
          {
            v[i].uv.x += (max_uv.x - v[i].uv.x) * (pcmd->ClipRect.x - v[i].pos.x) / (max_pos.x - v[i].pos.x);
            v[i].pos.x = pcmd->ClipRect.x;
          }
          else if (v[i].pos.x > pcmd->ClipRect.z)
          {
            v[i].uv.x -= (v[i].uv.x - min_uv.x) * (v[i].pos.x - pcmd->ClipRect.z) / (v[i].pos.x - min_pos.x);
            v[i].pos.x = pcmd->ClipRect.z;
          }
          if (v[i].pos.y < pcmd->ClipRect.y)
          {
            v[i].uv.y += (max_uv.y - v[i].uv.y) * (pcmd->ClipRect.y - v[i].pos.y) / (max_pos.y - v[i].pos.y);
            v[i].pos.y = pcmd->ClipRect.y;
          }
          else if (v[i].pos.y > pcmd->ClipRect.w)
          {
            v[i].uv.y -= (v[i].uv.y - min_uv.y) * (v[i].pos.y - pcmd->ClipRect.w) / (v[i].pos.y - min_pos.y);
            v[i].pos.y = pcmd->ClipRect.w;
          }
        }

        FCanvasUVTri triangle;
        triangle.V0_Pos = FVector2D(v[0].pos.x, v[0].pos.y);
        triangle.V1_Pos = FVector2D(v[1].pos.x, v[1].pos.y);
        triangle.V2_Pos = FVector2D(v[2].pos.x, v[2].pos.y);
        triangle.V0_UV = FVector2D(v[0].uv.x, v[0].uv.y);
        triangle.V1_UV = FVector2D(v[1].uv.x, v[1].uv.y);
        triangle.V2_UV = FVector2D(v[2].uv.x, v[2].uv.y);
        triangle.V0_Color = FLinearColor(col[0].x, col[0].y, col[0].z, col[0].w);
        triangle.V1_Color = FLinearColor(col[1].x, col[1].y, col[1].z, col[1].w);
        triangle.V2_Color = FLinearColor(col[2].x, col[2].y, col[2].z, col[2].w);
        triangles.Push(triangle);
      }

      // Draw triangles
      uCanvas->K2_DrawMaterialTriangle(FDFX_Module::MaterialInstance, triangles);
      idx_buffer += pcmd->ElemCount;
    }
  }
}

const char* FDFX_Thread::ImGui_ImplUE_GetClipboardText(void* user_data)
{
  FString text;
  FGenericPlatformApplicationMisc::ClipboardPaste(text);
  FTCHARToUTF8 result(*text);
  return result.Get();
}

void FDFX_Thread::ImGui_ImplUE_SetClipboardText(void* user_data, const char* text)
{
  FString Str(strlen(text), text);
  FGenericPlatformApplicationMisc::ClipboardCopy(*Str);
}

bool FDFX_Thread::ControllerInput()
{
  static bool bControllerDisabled = false;
  static bool bMainWindowStillOpen = false;
  APawn* aPawn = PlayerController->GetPawn();

  if (!FDFX_StatData::bMainWindowOpen && !bMainWindowStillOpen)
    return false;

  if (!FDFX_StatData::bMainWindowOpen && bControllerDisabled) {
    aPawn->EnableInput(PlayerController);
    bControllerDisabled = false;
    bMainWindowStillOpen = false;
    return true;
  }

  if (FDFX_StatData::bMainWindowOpen && !FDFX_StatData::bDisableGameControls) {
    if (aPawn) {
        aPawn->EnableInput(PlayerController);
    }
    bControllerDisabled = false;
    return true;
  }

  if (FDFX_StatData::bDisableGameControls) {
    if (FDFX_StatData::bMainWindowOpen) {
      aPawn->DisableInput(PlayerController);
      bMainWindowStillOpen = true;
      bControllerDisabled = true;
    }
  }
  return true;
}

void FDFX_Thread::ExternalWindow(bool IsExiting)
{
// TODO: Open external window and move charts.
// FAILED: To create an UCanvas on external window

  if (!FDFX_StatData::bExternalWindow && !bExternalOpened)
    return;

  bool extWinValid = m_extWindow.IsValid();

  if (IsExiting && extWinValid) {
    m_extWindow->RequestDestroyWindow();
    return;
  }

  if (FDFX_StatData::bExternalWindow && !bExternalOpened && !extWinValid) {
    FVector2D winPos = FVector2D(GameViewport->GetWindow()->GetPositionInScreen().X + ViewportSize.X, GameViewport->GetWindow()->GetPositionInScreen().Y);
    m_extWindow = SNew(SWindow)
      .Title(FText::FromString("DFoundryFX"))
      .Type(EWindowType::GameWindow)
      .ClientSize(FVector2D(ViewportSize.X / 4, ViewportSize.Y))
      .ScreenPosition(winPos)
      .FocusWhenFirstShown(true)
      .SupportsMaximize(true)
      .SupportsMinimize(true)
      .UseOSWindowBorder(false)
      .SizingRule(ESizingRule::UserSized);
    m_extWindow->SetAllowFastUpdate(true);
    m_extWindow->GetOnWindowClosedEvent().AddLambda(
      [this](const TSharedRef<SWindow>& Window) {
        m_extWindow = nullptr;
      }
    );
    m_extWindow = FSlateApplication::Get().AddWindow(m_extWindow.ToSharedRef(), true);
    m_extWindow->ShowWindow();
    m_extWindow->MoveWindowTo(winPos);
    FSlateApplication::Get().Tick();
    
    bExternalOpened = true;
    return;
  }

  if (!FDFX_StatData::bExternalWindow && bExternalOpened && extWinValid) {
    m_extWindow->RequestDestroyWindow();
    bExternalOpened = false;
    return;
  }

  if (FDFX_StatData::bExternalWindow && bExternalOpened && !extWinValid) {
    FDFX_StatData::bExternalWindow = false;
    bExternalOpened = false;
  }
}


void FDFX_Thread::Stop()
{
  SetPaused(true);
  bStopping = true;

  RemoveDelegates();

  DFoundryFX_Thread->WaitForCompletion();

  if (m_ImGuiContext) {
    //ImPlot::DestroyContext(m_ImPlotContext);
    //ImGui::DestroyContext(m_ImGuiContext);

    m_ImPlotContext = nullptr;
    m_ImGuiContext = nullptr;
  }
}

void FDFX_Thread::RemoveDelegates() {
  // Release all active delegates
  if (hOnViewportCreated.IsValid()) { 
    GameViewport->OnViewportCreated().Remove(hOnViewportCreated);
    hOnViewportCreated.Reset();
  }
  if (hOnViewportClose.IsValid()) { 
    GameViewport->GetWindow()->GetOnWindowClosedEvent().Remove(hOnViewportClose);
    hOnViewportClose.Reset();
  }
  if (hOnHUDPostRender.IsValid()) { 
    PlayerController->GetHUD()->OnHUDPostRender.Remove(hOnHUDPostRender);
    hOnHUDPostRender.Reset();
  }
  if (hOnGameModeInitialized.IsValid()) {
    FGameModeEvents::OnGameModeInitializedEvent().Remove(hOnGameModeInitialized);
    hOnGameModeInitialized.Reset();
  }
  if (hOnPipelineStateLogged.IsValid()) {
    FPipelineFileCacheManager::OnPipelineStateLogged().Remove(hOnPipelineStateLogged);
    hOnPipelineStateLogged.Reset();
  }
  if (hOnWorldBeginPlay.IsValid()) {
    uWorld->OnWorldBeginPlay.Remove(hOnWorldBeginPlay);
    hOnWorldBeginPlay.Reset();
  }
}

// *******************
// Whatever
// *******************
void FDFX_Thread::Wait(float Seconds)
{
  FPlatformProcess::Sleep(Seconds);
}

void FDFX_Thread::Tick()
{
}

uint32 FDFX_Thread::Run()
{
  return 0;
}

void FDFX_Thread::Exit()
{
}

void FDFX_Thread::SetPaused(bool MakePaused)
{
  bPaused.AtomicSet(MakePaused);
  if (!MakePaused)
  {
    bIsVerifiedSuspended.AtomicSet(false);
  }
}

bool FDFX_Thread::IsThreadPaused()
{
  return bPaused;
}

bool FDFX_Thread::IsThreadVerifiedSuspended()
{
  return bIsVerifiedSuspended;
}

bool FDFX_Thread::HasThreadStopped()
{
  return bHasStopped;
}

ImGuiKey FDFX_Thread::FKeyToImGuiKey(FName Keyname)
{
#define LITERAL_TRANSLATION(Key) { EKeys::Key.GetFName(), ImGuiKey_##Key }
  // not an exhaustive mapping, some keys are missing :^|
  static const TMap<FName, ImGuiKey> FKeyToImGuiKey =
  {
    LITERAL_TRANSLATION(A), LITERAL_TRANSLATION(B), LITERAL_TRANSLATION(C), LITERAL_TRANSLATION(D), LITERAL_TRANSLATION(E), LITERAL_TRANSLATION(F),
    LITERAL_TRANSLATION(G), LITERAL_TRANSLATION(H), LITERAL_TRANSLATION(I), LITERAL_TRANSLATION(J), LITERAL_TRANSLATION(K), LITERAL_TRANSLATION(L),
    LITERAL_TRANSLATION(M), LITERAL_TRANSLATION(N), LITERAL_TRANSLATION(O), LITERAL_TRANSLATION(P), LITERAL_TRANSLATION(Q), LITERAL_TRANSLATION(R),
    LITERAL_TRANSLATION(S), LITERAL_TRANSLATION(T), LITERAL_TRANSLATION(U), LITERAL_TRANSLATION(V), LITERAL_TRANSLATION(W), LITERAL_TRANSLATION(X),
    LITERAL_TRANSLATION(Y), LITERAL_TRANSLATION(Z),
    LITERAL_TRANSLATION(F1), LITERAL_TRANSLATION(F2), LITERAL_TRANSLATION(F3), LITERAL_TRANSLATION(F4),
    LITERAL_TRANSLATION(F5), LITERAL_TRANSLATION(F6), LITERAL_TRANSLATION(F7), LITERAL_TRANSLATION(F8),
    LITERAL_TRANSLATION(F9), LITERAL_TRANSLATION(F10), LITERAL_TRANSLATION(F11), LITERAL_TRANSLATION(F12),
    LITERAL_TRANSLATION(Enter), LITERAL_TRANSLATION(Insert), LITERAL_TRANSLATION(Delete), LITERAL_TRANSLATION(Escape), LITERAL_TRANSLATION(Tab),
    LITERAL_TRANSLATION(PageUp), LITERAL_TRANSLATION(PageDown), LITERAL_TRANSLATION(Home), LITERAL_TRANSLATION(End),
    LITERAL_TRANSLATION(NumLock), LITERAL_TRANSLATION(ScrollLock), LITERAL_TRANSLATION(CapsLock),
    LITERAL_TRANSLATION(RightBracket), LITERAL_TRANSLATION(LeftBracket), LITERAL_TRANSLATION(Backslash), LITERAL_TRANSLATION(Slash),
    LITERAL_TRANSLATION(Semicolon), LITERAL_TRANSLATION(Period), LITERAL_TRANSLATION(Comma), LITERAL_TRANSLATION(Apostrophe), LITERAL_TRANSLATION(Pause),
    { EKeys::Zero.GetFName(), ImGuiKey_0 }, { EKeys::One.GetFName(), ImGuiKey_1 }, { EKeys::Two.GetFName(), ImGuiKey_2 },
    { EKeys::Three.GetFName(), ImGuiKey_3 }, { EKeys::Four.GetFName(), ImGuiKey_4 }, { EKeys::Five.GetFName(), ImGuiKey_5 },
    { EKeys::Six.GetFName(), ImGuiKey_6 }, { EKeys::Seven.GetFName(), ImGuiKey_7 }, { EKeys::Eight.GetFName(), ImGuiKey_8 }, { EKeys::Nine.GetFName(), ImGuiKey_9 },
    { EKeys::NumPadZero.GetFName(), ImGuiKey_Keypad0 }, { EKeys::NumPadOne.GetFName(), ImGuiKey_Keypad1 }, { EKeys::NumPadTwo.GetFName(), ImGuiKey_Keypad2 },
    { EKeys::NumPadThree.GetFName(), ImGuiKey_Keypad3 }, { EKeys::NumPadFour.GetFName(), ImGuiKey_Keypad4 }, { EKeys::NumPadFive.GetFName(), ImGuiKey_Keypad5 },
    { EKeys::NumPadSix.GetFName(), ImGuiKey_Keypad6 }, { EKeys::NumPadSeven.GetFName(), ImGuiKey_Keypad7 }, { EKeys::NumPadEight.GetFName(), ImGuiKey_Keypad8 },
    { EKeys::NumPadNine.GetFName(), ImGuiKey_Keypad9 },
    { EKeys::LeftShift.GetFName(), ImGuiKey_LeftShift }, { EKeys::LeftControl.GetFName(), ImGuiKey_LeftCtrl }, { EKeys::LeftAlt.GetFName(), ImGuiKey_LeftAlt },
    { EKeys::RightShift.GetFName(), ImGuiKey_RightShift }, { EKeys::RightControl.GetFName(), ImGuiKey_RightCtrl }, { EKeys::RightAlt.GetFName(), ImGuiKey_RightAlt },
    { EKeys::SpaceBar.GetFName(), ImGuiKey_Space }, { EKeys::BackSpace.GetFName(), ImGuiKey_Backspace },
    { EKeys::Up.GetFName(), ImGuiKey_UpArrow }, { EKeys::Down.GetFName(), ImGuiKey_DownArrow },
    { EKeys::Left.GetFName(), ImGuiKey_LeftArrow }, { EKeys::Right.GetFName(), ImGuiKey_RightArrow },
    { EKeys::Subtract.GetFName(), ImGuiKey_KeypadSubtract }, { EKeys::Add.GetFName(), ImGuiKey_KeypadAdd },
    { EKeys::Multiply.GetFName(), ImGuiKey_KeypadMultiply }, { EKeys::Divide.GetFName(), ImGuiKey_KeypadDivide },
    { EKeys::Decimal.GetFName(), ImGuiKey_KeypadDecimal }, { EKeys::Equals.GetFName(), ImGuiKey_Equal },
  };

  const ImGuiKey* Key = FKeyToImGuiKey.Find(Keyname);
  return Key ? *Key : ImGuiKey_None;
}
#undef LOCTEXT_NAMESPACE