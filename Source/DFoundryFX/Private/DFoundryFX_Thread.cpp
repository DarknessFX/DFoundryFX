#include "DFoundryFX_Thread.h"

#define LOCTEXT_NAMESPACE "DFX_Thread"
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_ThreadProcEvents"), STAT_ThreadProcEvents, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_ThreadNewFrame"), STAT_ThreadNewFrame, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_ThreadRender"), STAT_ThreadRender, STATGROUP_DFoundryFX);
DECLARE_CYCLE_STAT(TEXT("DFoundryFX_ThreadDraw"), STAT_ThreadDraw, STATGROUP_DFoundryFX);

FThreadSafeBool FDFX_Thread::bThreadPaused;
FThreadSafeBool FDFX_Thread::bIsVerifiedSuspended;
FThreadSafeBool FDFX_Thread::bHasStopped;

FDFX_Thread::FDFX_Thread() {
  // Events
  hOnGameModeInitialized = FDelegateHandle();
  hOnWorldBeginPlay = FDelegateHandle();
  hOnHUDPostRender = FDelegateHandle();
  hOnHUDPostRender = FDelegateHandle();
  hOnViewportClose = FDelegateHandle();
  hOnPipelineStateLogged = FDelegateHandle();

  // Thread
  UE_LOG(LogDFoundryFX, Log, TEXT("Thread: Initializing DFoundryFX multithread."));
  DFoundryFX_Thread = FRunnableThread::Create(this, TEXT("DFoundryFX_Thread"), 128 * 1024, TPri_Lowest,
    FPlatformAffinity::GetNoAffinityMask(), EThreadCreateFlags::None );
}

FDFX_Thread::~FDFX_Thread() {
  UE_LOG(LogDFoundryFX, Log, TEXT("Thread: Destroying DFoundryFX multithread."));

  RemoveDelegates();

  if (DFoundryFX_Thread) {
    DFoundryFX_Thread->Kill(true);
    delete DFoundryFX_Thread;
    DFoundryFX_Thread = nullptr;
  }
}

bool FDFX_Thread::Init() {
  hOnGameModeInitialized = FGameModeEvents::OnGameModeInitializedEvent().AddRaw(this, &FDFX_Thread::OnGameModeInitialized);
  hOnViewportCreated = UGameViewportClient::OnViewportCreated().AddRaw(this, &FDFX_Thread::OnViewportCreated);

  // Bind to existing world when engine loaded
  if (GEngine && GEngine->GetWorldContexts().Num() != 0) {
    for (const FWorldContext& Context : GEngine->GetWorldContexts()) {
      if (Context.World() && Context.World()->IsGameWorld()) {
        OnGameModeInitialized(UGameplayStatics::GetGameMode(Context.World()));
        OnWorldBeginPlay();
        break;
      }
    }
  }

  return true;
}

void FDFX_Thread::OnGameModeInitialized(AGameModeBase* aGameMode) {
  UE_LOG(LogDFoundryFX, Log, TEXT("Thread: Initializing ImGui resources and context."));

  // ImGui init
  m_ImGuiContext = ImGui::CreateContext();
  m_ImPlotContext = ImPlot::CreateContext();
  ImGui_ImplUE_CreateDeviceObjects();
  ImGui_ImplUE_Init();

    // UE context
  GameMode = aGameMode;
  uWorld = GameMode.Get()->GetWorld();
  GameViewport = uWorld.Get()->GetGameViewport();

  // Cleanup if PIE or second window.
  RemoveDelegates();

  // Bind events
  hOnWorldBeginPlay = uWorld.Get()->OnWorldBeginPlay.AddRaw(this, &FDFX_Thread::OnWorldBeginPlay);
  hOnViewportResized = GEngine->GameViewport->Viewport->ViewportResizedEvent.AddRaw(this, &FDFX_Thread::OnViewportResized);
  hOnViewportClose = GameViewport.Get()->GetWindow()->GetOnWindowClosedEvent().AddLambda(
  [this](const TSharedRef<SWindow>& Window) {
    OnViewportClose();
  });
  hOnPipelineStateLogged = FPipelineFileCacheManager::OnPipelineStateLogged().AddRaw(this, &FDFX_Thread::OnPipelineStateLogged);
  ShaderLogTime = FApp::GetCurrentTime();
}


void FDFX_Thread::OnWorldBeginPlay() {
  ViewportSize = FVector2D::ZeroVector;

  // Fix for GameMode HUD Class = None
  PlayerController = uWorld->GetFirstPlayerController();
  if (!GameMode.Get()->HUDClass) {
    PlayerController.Get()->SpawnDefaultHUD();
  }

  // Fix for new PIE window
  if (hOnHUDPostRender.IsValid() && PlayerController.Get()->GetHUD()->OnHUDPostRender.IsBound()) {
    PlayerController.Get()->GetHUD()->OnHUDPostRender.Remove(hOnHUDPostRender);
    hOnHUDPostRender.Reset();
  }

  if(!hOnViewportClose.IsValid()) {
    hOnViewportClose = GameViewport.Get()->GetWindow()->GetOnWindowClosedEvent().AddLambda(
      [this](const TSharedRef<SWindow>& Window) {
        FDFX_Thread::OnViewportClose();
      });
  }

  if (PlayerController.Get()) {
    hOnHUDPostRender = PlayerController.Get()->GetHUD()->OnHUDPostRender.AddRaw(this, &FDFX_Thread::OnHUDPostRender);
  }
}

void FDFX_Thread::OnHUDPostRender(AHUD* HUD, UCanvas* Canvas) {
  if (bStopping) { return; }

  ViewportSize = FVector2D(Canvas->SizeX, Canvas->SizeY);
  uCanvas = Canvas;
  FDFX_Thread::ImGui_ImplUE_Render();
}

void FDFX_Thread::OnViewportCreated() {
}

void FDFX_Thread::OnViewportResized(FViewport* Viewport, uint32 /*Unused*/) {
  FDFX_StatData::LoadDefaultValues(GameViewport.Get());
}

bool FDFX_Thread::OnViewportClose() {
  RemoveDelegates();

  // ImGui
  if (m_ImPlotContext) {
    ImPlot::DestroyContext(m_ImPlotContext);
    m_ImPlotContext = nullptr;
  }
  if (m_ImGuiContext) {
    ImGui::DestroyContext(m_ImGuiContext);
    m_ImGuiContext = nullptr;
  }

  ViewportSize = FVector2D::ZeroVector;
  m_ImGuiDiffTime = 0;

  // Hook for the next viewport/PIE
  hOnGameModeInitialized = FGameModeEvents::OnGameModeInitializedEvent().AddRaw(this, &FDFX_Thread::OnGameModeInitialized);
  hOnViewportCreated = UGameViewportClient::OnViewportCreated().AddRaw(this, &FDFX_Thread::OnViewportCreated);

  return true;
}

void FDFX_Thread::OnPipelineStateLogged(const FPipelineCacheFileFormatPSO& PipelineCacheFileFormatPSO) {
  double ShaderLogDiff = FApp::GetCurrentTime() - ShaderLogTime;
  uint32 Type = static_cast<uint32>(PipelineCacheFileFormatPSO.Type);

  FString ShaderDesc;
  uint32 psoLogType = 0;

  switch (Type) {
    case 0:  // Compute
      psoLogType = 1;
      ShaderDesc = PipelineCacheFileFormatPSO.ComputeDesc.ComputeShader.ToString();
      break;
    case 1:  // Graphics
      psoLogType = 2;
      ShaderDesc = PipelineCacheFileFormatPSO.GraphicsDesc.ShadersToString();
      break;
    case 2:  // RayTracing
      psoLogType = 4;
      ShaderDesc = PipelineCacheFileFormatPSO.RayTracingDesc.ShaderHash.ToString();
      break;
    default:
      return;  // Ignore unknown types
  }

  FDFX_StatData::AddShaderLog(psoLogType, *ShaderDesc, ShaderLogDiff);
  ShaderLogTime = FApp::GetCurrentTime();
}


ImGuiIO& FDFX_Thread::GetImGuiIO() const {
  checkf(m_ImGuiContext, TEXT("ImGuiContext is invalid!"));

  ImGui::SetCurrentContext(m_ImGuiContext);
  ImPlot::SetCurrentContext(m_ImPlotContext);
  return ImGui::GetIO();
}

bool FDFX_Thread::ImGui_ImplUE_Init() {
  ImGuiIO& io = GetImGuiIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable
    | ImGuiConfigFlags_NavEnableKeyboard
    | ImGuiConfigFlags_NavEnableGamepad;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

  ImGui::StyleColorsDark();

  io.SetClipboardTextFn = ImGui_ImplUE_SetClipboardText;
  io.GetClipboardTextFn = ImGui_ImplUE_GetClipboardText;

  io.MouseDrawCursor = false;

  return true;
}


bool FDFX_Thread::ImGui_ImplUE_CreateDeviceObjects() {
  // Build texture atlas
  ImGuiIO& io = GetImGuiIO();
  io.Fonts->Clear();

  ImFontConfig cfg;
  cfg.OversampleH = 2;
  cfg.OversampleV = 2;
  cfg.SizePixels = 14.0f; // * DPIscale;
  io.Fonts->AddFontDefault(&cfg);
  io.Fonts->Build();

  unsigned char* pixels;
  int width, height, bytes_per_pixel;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

  if (!FDFX_Module::FontTexture_Updated) {
    FDFX_Module::FontTexture = UTexture2D::CreateTransient(width, height, PF_R8G8B8A8);
    FDFX_Module::FontTexture->SRGB = false;
    FDFX_Module::FontTexture->Filter = TF_Nearest;
    FDFX_Module::FontTexture->LODGroup = TEXTUREGROUP_Pixels2D;

    UE_LOG(LogDFoundryFX, Log, TEXT("ImGui FontTexture : TexData %d x %d."), width, height);
    AsyncTask(ENamedThreads::GameThread, [width, height, bytes_per_pixel, pixels]() {
      FDFX_Module::FontTexture->UnlinkStreaming();
      FTexture2DMipMap& mip = FDFX_Module::FontTexture->GetPlatformData()->Mips[0];
      void* data = mip.BulkData.Lock(LOCK_READ_WRITE);
      int size = width * height * bytes_per_pixel ; //* 4;
      FMemory::Memcpy(data, pixels, size);
      mip.BulkData.Unlock();

      FDFX_Module::FontTexture->UpdateResource();
      FDFX_Module::MaterialInstance->SetTextureParameterValue(FName("param"), FDFX_Module::FontTexture);
      FDFX_Module::MaterialInstance->CacheShaders(EMaterialShaderPrecompileMode::Synchronous);
      //FDFX_Module::MaterialInstance->AllMaterialsCacheResourceShadersForRendering(); DX12 ValidateBoundUniformBuffer Crash

      FDFX_Module::FontTexture_Updated = true;
      FDFX_Module::FontTexture->AddToRoot();
    });
  }
  // Store our identifier
  io.Fonts->TexID = (void*)FDFX_Module::FontTexture;

  return true;
}

void FDFX_Thread::ImGui_ImplUE_Render() {
  const uint64 m_ImGuiBeginTime = FPlatformTime::Cycles64();
  { SCOPE_CYCLE_COUNTER(STAT_ThreadProcEvents);
    ImGui_ImplUE_ProcessEvent(); }
  { SCOPE_CYCLE_COUNTER(STAT_ThreadNewFrame);
    ImGui_ImplUE_NewFrame(); }

  FDFX_StatData::RunDFoundryFX(m_ImGuiDiffTime * 1000);

  { SCOPE_CYCLE_COUNTER(STAT_ThreadRender);
    ImGui::Render(); }
  { SCOPE_CYCLE_COUNTER(STAT_ThreadDraw);
    ImGui_ImplUE_RenderDrawLists(); }

  const uint64 m_ImGuiEndTime = FPlatformTime::Cycles64();
  m_ImGuiDiffTime = (m_ImGuiEndTime - m_ImGuiBeginTime);
}

void FDFX_Thread::ImGui_ImplUE_ProcessEvent() {
  if (!PlayerController.IsValid() || !HandleControllerInput()) {
    return;
  }

  ImGuiIO& io = GetImGuiIO();

  // Modifiers
  io.KeyShift = PlayerController.Get()->IsInputKeyDown(EKeys::LeftShift) || PlayerController.Get()->IsInputKeyDown(EKeys::RightShift);
  io.KeyCtrl = PlayerController.Get()->IsInputKeyDown(EKeys::LeftControl) || PlayerController.Get()->IsInputKeyDown(EKeys::RightControl);
  io.KeyAlt = PlayerController.Get()->IsInputKeyDown(EKeys::LeftAlt) || PlayerController.Get()->IsInputKeyDown(EKeys::RightAlt);
  io.KeySuper = false;

  // Keys
  TArray<FKey> keys;
  EKeys::GetAllKeys(keys);
  for (const FKey& key : keys) {
    ImGuiKey imguiKey = FKeyToImGuiKey(key.GetFName());
    if (imguiKey == ImGuiKey_None) {
      continue;
    }

    bool isDown = PlayerController.Get()->IsInputKeyDown(key);
    io.AddKeyEvent(imguiKey, isDown);

    if (key.IsTouch()) {
      float touchX, touchY;
      bool touchPressed;
      PlayerController.Get()->GetInputTouchState(ETouchIndex::Touch1, touchY, touchX, touchPressed);
      io.AddMousePosEvent(touchX, touchY);
      io.AddMouseButtonEvent(0, touchPressed);
      continue;
    }

    // Skip non-keyboard
    if (key.IsMouseButton() || key.IsModifierKey() || key.IsGamepadKey() || key.IsAxis1D() || key.IsAxis2D()) {
      continue;
    }

    // Character input on press
    if (PlayerController.Get()->WasInputKeyJustPressed(key)) {
      const uint32* keyCode = nullptr;
      const uint32* charCode = nullptr;
      FInputKeyManager::Get().GetCodesFromKey(key, keyCode, charCode);
      if (charCode) {
        int c = tolower(static_cast<int>(*charCode));
        if (io.KeyShift) {
          c = toupper(c);
        }
        io.AddInputCharacter(static_cast<ImWchar>(c));
      }
    }
  }

  HandleControllerInput();
}

void FDFX_Thread::ImGui_ImplUE_NewFrame() {
  ImGuiIO& io = GetImGuiIO();

  FVector2D MousePos;
  PlayerController.Get()->GetMousePosition(MousePos.X, MousePos.Y);
  io.MousePos = ImVec2(MousePos.X, MousePos.Y);

  static bool bPrevLeftDown = false;
  static bool bPrevRightDown = false;
  static bool bPrevMiddleDown = false;

  bool bLeftDown = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
  bool bRightDown = PlayerController->IsInputKeyDown(EKeys::RightMouseButton);
  bool bMiddleDown = PlayerController->IsInputKeyDown(EKeys::MiddleMouseButton);

  // Left
  if (bLeftDown != bPrevLeftDown) { io.AddMouseButtonEvent(0, bLeftDown); }
  bPrevLeftDown = bLeftDown;

  // Right
  if (bRightDown != bPrevRightDown) { io.AddMouseButtonEvent(1, bRightDown); }
  bPrevRightDown = bRightDown;

  // Middle
  if (bMiddleDown != bPrevMiddleDown) { io.AddMouseButtonEvent(2, bMiddleDown); }
  bPrevMiddleDown = bMiddleDown;

  float WheelDelta = PlayerController.Get()->GetInputAnalogKeyState(EKeys::MouseWheelAxis);
  if (FMath::Abs(WheelDelta) > KINDA_SMALL_NUMBER) { io.AddMouseWheelEvent(0.0f, WheelDelta); } // X=0, Y=WheelDelta

  io.DisplaySize = ImVec2((float)ViewportSize.X, (float)ViewportSize.Y);
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f); // HACK to disable ImGui DPI scale, more details at FDFX_StatData::LoadDefaultValues

  ImGui::NewFrame();
}

void FDFX_Thread::ImGui_ImplUE_RenderDrawLists() {
  if (!uCanvas.IsValid()) { return; }

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
      uCanvas.Get()->K2_DrawMaterialTriangle(FDFX_Module::MaterialInstance, triangles);
      idx_buffer += pcmd->ElemCount;
    }
  }
}

const char* FDFX_Thread::ImGui_ImplUE_GetClipboardText(void* user_data) {
// Removed for Android builds
/*
  FString text;
  FGenericPlatformApplicationMisc::ClipboardPaste(text);
  FTCHARToUTF8 result(*text);
  return result.Get();
*/
  return nullptr;
}

void FDFX_Thread::ImGui_ImplUE_SetClipboardText(void* user_data, const char* text) {
/*
  int new_size = strlen(text) + 1;
  TCHAR* new_str = new TCHAR[new_size];
  size_t convertedChars = 0;
  mbstowcs_s(&convertedChars, new_str, new_size, text, _TRUNCATE);
  FGenericPlatformApplicationMisc::ClipboardCopy(new_str);
  delete[] new_str;
*/
}

bool FDFX_Thread::HandleControllerInput() {
  if (!PlayerController.IsValid()) { return false; }
  return true;
}

void FDFX_Thread::Stop() {
  SetPaused(true);
  bStopping = true;

  RemoveDelegates();

  if (m_ImPlotContext) {
    ImPlot::DestroyContext(m_ImPlotContext);
    m_ImPlotContext = nullptr;
  }
  if (m_ImGuiContext) {
    ImGui::DestroyContext(m_ImGuiContext);
    m_ImGuiContext = nullptr;
  }
}

void FDFX_Thread::RemoveDelegates() {
  // Static delegates
  if (hOnViewportResized.IsValid()) {
    GEngine->GameViewport->Viewport->ViewportResizedEvent.Remove(hOnViewportResized);
    hOnViewportResized.Reset();
  }
  if (hOnViewportCreated.IsValid()) {
    UGameViewportClient::OnViewportCreated().Remove(hOnViewportCreated);
    hOnViewportCreated.Reset();
  }
  if (hOnGameModeInitialized.IsValid()) {
    FGameModeEvents::OnGameModeInitializedEvent().Remove(hOnGameModeInitialized);
    hOnGameModeInitialized.Reset();
  }
  if (hOnPipelineStateLogged.IsValid()) {
    FPipelineFileCacheManager::OnPipelineStateLogged().Remove(hOnPipelineStateLogged);
    hOnPipelineStateLogged.Reset();
  }

  // Instance delegates
  if (hOnWorldBeginPlay.IsValid() && uWorld.IsValid()) {
    uWorld.Get()->OnWorldBeginPlay.Remove(hOnWorldBeginPlay);
    hOnWorldBeginPlay.Reset();
  }
  if (hOnViewportClose.IsValid() && GameViewport.IsValid() && GameViewport.Get()->GetWindow().IsValid()) {
    GameViewport.Get()->GetWindow().Get()->GetOnWindowClosedEvent().Remove(hOnViewportClose);
    hOnViewportClose.Reset();
  }
  if (hOnHUDPostRender.IsValid() && PlayerController.IsValid()) {
    AHUD* HUD = PlayerController.Get()->GetHUD();
    if (HUD && HUD->OnHUDPostRender.IsBound()) {
      HUD->OnHUDPostRender.Remove(hOnHUDPostRender);
    }
    hOnHUDPostRender.Reset();
  }
}

ImGuiKey FDFX_Thread::FKeyToImGuiKey(FName KeyName) {
  #define LITERAL_TRANSLATION(Key) { EKeys::Key.GetFName(), ImGuiKey_##Key }
  static const TMap<FName, ImGuiKey> KeyMap = {
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
    { EKeys::Decimal.GetFName(), ImGuiKey_KeypadDecimal }, { EKeys::Equals.GetFName(), ImGuiKey_Equal }
  };
  #undef LITERAL_TRANSLATION

  const ImGuiKey* FoundKey = KeyMap.Find(KeyName);
  return FoundKey ? *FoundKey : ImGuiKey_None;
}

// *******************
// Thread
// *******************
void FDFX_Thread::Wait(float Seconds) {
  FPlatformProcess::Sleep(Seconds);
}

uint32 FDFX_Thread::Run() {
  return 0;
}

void FDFX_Thread::Tick() { }

void FDFX_Thread::Exit() { }

void FDFX_Thread::SetPaused(bool MakePaused) {
  bThreadPaused.AtomicSet(MakePaused);
  if (!MakePaused) {
    bIsVerifiedSuspended.AtomicSet(false);
  }
}

bool FDFX_Thread::IsThreadPaused() const {
  return bThreadPaused;
}

bool FDFX_Thread::IsThreadVerifiedSuspended() const {
  return bIsVerifiedSuspended;
}

bool FDFX_Thread::HasThreadStopped() const {
  return bHasStopped;
}

#undef LOCTEXT_NAMESPACE