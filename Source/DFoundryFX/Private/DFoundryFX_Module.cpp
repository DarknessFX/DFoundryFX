#include "DFoundryFX_Module.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "HAL/PlatformProcess.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Async/Async.h"

DEFINE_LOG_CATEGORY(LogDFoundryFX);
#define LOCTEXT_NAMESPACE "DFX_Module"

TSharedPtr<FDFX_Thread> DFXThread;
static bool GDFX_isModuleEnable = true;

static FAutoConsoleCommandWithWorld DFXToggleCmd(
  TEXT("DFoundryFX.Toggle"),
  TEXT("Toggle DFoundryFX performance UI."),
  FConsoleCommandWithWorldDelegate::CreateLambda([](const UWorld* World) {
    GDFX_isModuleEnable = !GDFX_isModuleEnable;
    if (GDFX_isModuleEnable && !DFXThread.IsValid()) {
      DFXThread  = MakeShared<FDFX_Thread>();
    }
    else if (!GDFX_isModuleEnable && DFXThread.IsValid()) {
      DFXThread->Stop();
      DFXThread.Reset();
    }
    UE_LOG(LogDFoundryFX, Log, TEXT("DFX Enabled: %s"), GDFX_isModuleEnable ? TEXT("true") : TEXT("false"));
  })
);

void FDFX_Module::StartupModule() {
  GConfig->GetBool(TEXT("DFoundryFX"), TEXT("isModuleEnable"), GDFX_isModuleEnable, GEngineIni);

  LoadStatCommands();

  // Keep the Material and Texture loaded independent from FDFXThread
  MasterMaterial = LoadObject<UMaterialInterface>(NULL, TEXT("Material'/DFoundryFX/M_ImGui.M_ImGui'"), NULL, LOAD_None, NULL);
  if (MasterMaterial) {
    MaterialInstance = UMaterialInstanceDynamic::Create(MasterMaterial, NULL);
    MaterialInstance->AllocatePermutationResource();
    MaterialInstance->ClearParameterValues();
  }
  MasterMaterial->AddToRoot();
  MaterialInstance->AddToRoot();

  //FDFX_Thread
  if (!FPlatformProcess::SupportsMultithreading()) {
    UE_LOG(LogDFoundryFX, Warning, TEXT("%s: Platform don't support Multithreads."), ModuleName);
  }
  if (GDFX_isModuleEnable) {
    DFXThread = MakeShared<FDFX_Thread>();
  }

  UE_LOG(LogDFoundryFX, Log, TEXT("%s StartupModule"), ModuleName);
}


void FDFX_Module::ShutdownModule() {
  if (DFXThread.IsValid()) {
    DFXThread->Stop();
    DFXThread.Reset();
  }

  MaterialInstance = nullptr;
  MasterMaterial = nullptr;
  FontTexture = nullptr;
  FontTexture_Updated = false;

  UE_LOG(LogDFoundryFX, Log, TEXT("%s ShutdownModule"), ModuleName);
}

void FDFX_Module::LoadStatCommands() {
  if (StatCommands.Num() > 0) {
    return;
  }

  // STAT Names
  static constexpr const TCHAR* StatAllNames[] = {
    TEXT("Accessibility"), TEXT("AI"), TEXT("AI_EQS"), TEXT("AIBehaviorTree"), TEXT("AICrowd"),
    TEXT("Anim"), TEXT("AnimationSharing"), TEXT("ARComponent"), TEXT("AsyncIO"), TEXT("AsyncLoad"),
    TEXT("AsyncLoadGameThread"), TEXT("Audio"), TEXT("AudioMixer"), TEXT("AudioThreadCommands"),
    TEXT("BackChannel"), TEXT("BeamParticles"), TEXT("CableComponent"), TEXT("CameraAnimation"), TEXT("Canvas"),
    TEXT("Chaos"), TEXT("ChaosCloth"), TEXT("ChaosCollision"), TEXT("ChaosCollisionCounters"), TEXT("ChaosConstraintDetails"),
    TEXT("ChaosCounters"), TEXT("ChaosDedicated"), TEXT("ChaosEngine"), TEXT("ChaosJoint"), TEXT("ChaosMinEvolution"),
    TEXT("ChaosNiagara"), TEXT("ChaosThread"), TEXT("ChaosUserDataPT"), TEXT("ChaosWide"), TEXT("Character"),
    TEXT("Collision"), TEXT("CollisionTags"), TEXT("CollisionVerbose"), TEXT("ColorList"), TEXT("CommandListMarkers"),
    TEXT("Component"), TEXT("Compression"), TEXT("ControlRig"), TEXT("Cooking"), TEXT("CPULoad"),
    TEXT("CPUStalls"), TEXT("D3D11RHI"), TEXT("D3D12BufferDetails"), TEXT("D3D12DescriptorHeap"), TEXT("D3D12Memory"),
    TEXT("D3D12MemoryDetails"), TEXT("D3D12PipelineState"), TEXT("D3D12RayTracing"), TEXT("D3D12Resources"), TEXT("D3D12RHI"),
    TEXT("DDC"), TEXT("Default"), TEXT("Detailed"), TEXT("DrawCount"), TEXT("DumpHitches"),
    TEXT("Dumpticks"), TEXT("EmittersRT"), TEXT("Engine"), TEXT("Foliage"), TEXT("FPS"),
    TEXT("FrameCounter"), TEXT("FunctionalTest"), TEXT("Game"), TEXT("GameplayTags"), TEXT("GameplayTasks"),
    TEXT("GC"), TEXT("GeometryCache"), TEXT("GPU"), TEXT("GPUParticles"), TEXT("GPUSkinCache"),
    TEXT("GroupPaintTool"), TEXT("Hitches"), TEXT("HTTPThread"), TEXT("IMEWindows"), TEXT("ImgMediaPlugin"),
    TEXT("ImmediatePhysics"), TEXT("ImmediatePhysicsCounters"), TEXT("InitViews"), TEXT("KismetCompiler"), TEXT("KismetReinstancer"),
    TEXT("Landscape"), TEXT("Levels"), TEXT("LightRendering"), TEXT("LinkerCount"), TEXT("LinkerLoad"),
    TEXT("List"), TEXT("LLM"), TEXT("LLMFull"), TEXT("LLMOverhead"), TEXT("LLMPlatform"),
    TEXT("LoadTime"), TEXT("LoadTimeVerbose"), TEXT("MapBuildData"), TEXT("MathVerbose"), TEXT("MaterialMemory"),
    TEXT("Media"), TEXT("Memory"), TEXT("MemoryAllocator"), TEXT("MemoryPlatform"), TEXT("MemoryProfiler"),
    TEXT("MemoryStaticMesh"), TEXT("MorphTarget"), TEXT("MovieSceneECS"), TEXT("MovieSceneEval"), TEXT("MovieSceneRepl"),
    TEXT("MRMESH"), TEXT("NamedEvents"), TEXT("Nanite"), TEXT("NaniteCoarseMeshStreaming"), TEXT("NaniteRayTracing"),
    TEXT("Navigation"), TEXT("Net"), TEXT("Niagara"), TEXT("NiagaraBaseLines"), TEXT("NiagaraEditor"),
    TEXT("NiagaraOverview"), TEXT("NiagaraRibbons"), TEXT("NiagaraSystemCounts"), TEXT("Object"), TEXT("ObjectVerbose"),
    TEXT("Online"), TEXT("OodleNetwork"), TEXT("Packet"), TEXT("Pakfile"), TEXT("Paper2D"),
    TEXT("ParallelCommandListMarkers"), TEXT("ParticleMem"), TEXT("ParticlePerf"), TEXT("Particles"), TEXT("ParticlesOverview"),
    TEXT("ParticlesStats"), TEXT("Physics"), TEXT("PhysicsFields"), TEXT("PhysXTasks"), TEXT("PhysXVehicleManager"),
    TEXT("Ping"), TEXT("PipelineStateCache"), TEXT("PlayerController"), TEXT("Profiler"), TEXT("PSCWorldMan"),
    TEXT("Python"), TEXT("Quick"), TEXT("Raw"), TEXT("RGD"), TEXT("RenderTargetPool"),
    TEXT("RenderThreadCommands"), TEXT("RHI"), TEXT("RHICMDLIST"), TEXT("RHITransientMemory"), TEXT("SceneMemory"),
    TEXT("SceneRendering"), TEXT("SceneUpdate"), TEXT("Script"), TEXT("ScultToolOctree"), TEXT("ServerCPU"),
    TEXT("SFC"), TEXT("ShaderCompiling"), TEXT("Shaders"), TEXT("ShadowRendering"), TEXT("SignificanceManager"),
    TEXT("Slate"), TEXT("SlateMemory"), TEXT("SlateVerbose"), TEXT("SoundCues"), TEXT("SoundMixes"),
    TEXT("SoundModulators"), TEXT("SoundModulatorsHelp"), TEXT("SoundReverb"), TEXT("Sounds"), TEXT("SoundWaves"),
    TEXT("SplitScreen"), TEXT("StatSystem"), TEXT("Streaming"), TEXT("StreamingDetails"), TEXT("StreamingOverview"),
    TEXT("Summary"), TEXT("TargetPlatform"), TEXT("TaskGraphTasks"), TEXT("Text"), TEXT("TextureGroup"),
    TEXT("TexturePool"), TEXT("Threading"), TEXT("ThreadPoolAsyncTasks"), TEXT("Threads"), TEXT("Tickables"),
    TEXT("TickGroups"), TEXT("Timecode"), TEXT("Trace"), TEXT("TrailParticles"), TEXT("UI"),
    TEXT("Unit"), TEXT("UnitGraph"), TEXT("UnitMax"), TEXT("UnitTime"), TEXT("UObjectHash"),
    TEXT("UObjects"), TEXT("VectorVM"), TEXT("VectorVMBackend"), TEXT("VerboseNamedEvents"), TEXT("Version"),
    TEXT("VideoRecordingSystem"), TEXT("VirtualTextureMemory"), TEXT("VirtualTexturing"), TEXT("Voice"), TEXT("VTP"),
    TEXT("Xmpp")
  };
  static constexpr const TCHAR* StatCommonNames[] = {
    TEXT("FPS"), TEXT("Unit"), TEXT("UnitGraph")
  };
  static constexpr const TCHAR* StatPerfNames[] = {
    TEXT("Engine"), TEXT("Game"), TEXT("GPU"), TEXT("RHI"), TEXT("InitViews"),
    TEXT("SceneRendering"), TEXT("Memory"), TEXT("MemoryPlatform"), TEXT("Shaders"), TEXT("ShaderCompiling"),
    TEXT("UI"), TEXT("Slate"), TEXT("Niagara"), TEXT("DrawCount"), TEXT("Tickables"), TEXT("TickGroups")
  };

  constexpr size_t AllCount = UE_ARRAY_COUNT(StatAllNames);
  constexpr size_t CommonCount = UE_ARRAY_COUNT(StatCommonNames);
  constexpr size_t PerfCount = UE_ARRAY_COUNT(StatPerfNames);
  StatCommands.Reserve(AllCount + CommonCount + PerfCount);

  // Populate all with default
  for (size_t i = 0; i < AllCount; ++i) {
    FDFXStatCmd Cmd;
    Cmd.Category = EDFXStatCategory::None;
    Cmd.bEnabled = true;
    Cmd.Command = StatAllNames[i];
    StatCommands.Add(Cmd);
  }

  // Override for Common
  for (int32 i = 0; i < CommonCount; ++i) {
    FString CommonStr(StatCommonNames[i]);
    int32 Index = StatCommands.IndexOfByPredicate([&](const FDFXStatCmd& Cmd) { return Cmd.Command == CommonStr; });
    if (Index != INDEX_NONE) {
      StatCommands[Index].Category = EDFXStatCategory::Common;
    } else {
      FDFXStatCmd Cmd;
      Cmd.Category = EDFXStatCategory::Common;
      Cmd.bEnabled = true;
      Cmd.Command = CommonStr;
      StatCommands.Add(Cmd);
    }
  }

  // Override for Perf
  for (int32 i = 0; i < PerfCount; ++i) {
    FString PerfStr(StatPerfNames[i]);
    int32 Index = StatCommands.IndexOfByPredicate([&](const FDFXStatCmd& Cmd) { return Cmd.Command == PerfStr; });
    if (Index != INDEX_NONE) {
      StatCommands[Index].Category = EDFXStatCategory::Performance;
    } else {
      FDFXStatCmd Cmd;
      Cmd.Category = EDFXStatCategory::Performance;
      Cmd.bEnabled = true;
      Cmd.Command = PerfStr;
      StatCommands.Add(Cmd);
    }
  }
}

IMPLEMENT_MODULE(FDFX_Module, DFoundryFX)
#undef LOCTEXT_NAMESPACE
