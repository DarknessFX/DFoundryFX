#include "Module.h"
DEFINE_LOG_CATEGORY(LogDFoundryFX);

#define LOCTEXT_NAMESPACE "DFX_Module"
void FDFX_Module::StartupModule()
{
  UE_LOG(LogDFoundryFX, Log, TEXT("Module: Initializing DFoundryFX module."));

//FDFX_StatData 
  // TODO: Output "STAT HELP to FString"(?) instead of this huge STATList
  // Load STAT Commands
  FString STATList = "Accessibility,AI,AI_EQS,AIBehaviorTree,AICrowd,Anim,AnimationSharing,ARComponent,AsyncIO,AsyncLoad,AsyncLoadGameThread,Audio,AudioMixer,AudioThreadCommands,"
    "BackChannel,BeamParticles,CableComponent,CameraAnimation,Canvas,Chaos,ChaosCloth,ChaosCollision,ChaosCollisionCounters,ChaosConstraintDetails,ChaosCounters,"
    "ChaosDedicated,ChaosEngine,ChaosJoint,ChaosMinEvolution,ChaosNiagara,ChaosThread,ChaosUserDataPT,ChaosWide,Character,Collision,CollisionTags,"
    "CollisionVerbose,ColorList,CommandListMarkers,Component,Compression,ControlRig,Cooking,CPULoad,CPUStalls,D3D11RHI,D3D12BufferDetails,D3D12DescriptorHeap,D3D12Memory,"
    "D3D12MemoryDetails,D3D12PipelineState,D3D12RayTracing,D3D12Resources,D3D12RHI,DDC,Default,Detailed,DrawCount,DumpHitches,Dumpticks,EmittersRT,Engine,Foliage,FPS,FrameCounter,"
    "FunctionalTest,Game,GameplayTags,GameplayTasks,GC,GeometryCache,GPU,GPUParticles,GPUSkinCache,GroupPaintTool,Hitches,HTTPThread,IMEWindows,ImgMediaPlugin,ImmediatePhysics,"
    "ImmediatePhysicsCounters,InitViews,KismetCompiler,KismetReinstancer,Landscape,Levels,LightRendering,LinkerCount,LinkerLoad,List,LLM,LLMFull,LLMOverhead,"
    "LLMPlatform,LoadTime,LoadTimeVerbose,MapBuildData,MathVerbose,MaterialMemory,Media,Memory,MemoryAllocator,MemoryPlatform,MemoryProfiler,MemoryStaticMesh,"
    "MorphTarget,MovieSceneECS,MovieSceneEval,MovieSceneRepl,MRMESH,NamedEvents,Nanite,NaniteCoarseMeshStreaming,NaniteRayTracing,Navigation,Net,"
    "Niagara,NiagaraBaseLines,NiagaraEditor,NiagaraOverview,NiagaraRibbons,NiagaraSystemCounts,Object,ObjectVerbose,Online,OodleNetwork,Packet,Pakfile,Paper2D,"
    "ParallelCommandListMarkers,ParticleMem,ParticlePerf,Particles,ParticlesOverview,ParticlesStats,Physics,PhysicsFields,PhysXTasks,PhysXVehicleManager,Ping,"
    "PipelineStateCache,PlayerController,Profiler,PSCWorldMan,Python,Quick,Raw,RGD,RenderTargetPool,RenderThreadCommands,RHI,RHICMDLIST,RHITransientMemory,"
    "SceneMemory,SceneRendering,SceneUpdate,Script,ScultToolOctree,ServerCPU,SFC,ShaderCompiling,Shaders,ShadowRendering,SignificanceManager,Slate,"
    "SlateMemory,SlateVerbose,SoundCues,SoundMixes,SoundModulators,SoundModulatorsHelp,SoundReverb,Sounds,SoundWaves,SplitScreen,StatSystem,Streaming,"
    "StreamingDetails,StreamingOverview,Summary,TargetPlatform,TaskGraphTasks,Text,TextureGroup,TexturePool,Threading,ThreadPoolAsyncTasks,Threads,"
    "Tickables,TickGroups,Timecode,Trace,TrailParticles,UI,Unit,UnitGraph,UnitMax,UnitTime,UObjectHash,UObjects,VectorVM,VectorVMBackend,VerboseNamedEvents,"
    "Version,VideoRecordingSystem,VirtualTextureMemory,VirtualTexturing,Voice,VTP,Xmpp";
  FDFX_StatData::LoadSTAT(FDFX_StatData::None, STATList);
  FDFX_StatData::LoadSTAT(FDFX_StatData::Common, "FPS,Unit,UnitGraph");
  FDFX_StatData::LoadSTAT(FDFX_StatData::Perf, "Engine,Game,GPU,RHI,InitViews,SceneRendering,Memory,MemoryPlatform,Shaders,ShaderCompiling,UI,Slate,Niagara,DrawCount,Tickables,TickGroups");

  // TODO: Output DumpConsoleCommands to FString
  // Load CVAR
  // FDFX_StatData::LoadCVAR();

//FDFX_Thread
  if (!FPlatformProcess::SupportsMultithreading()) {
    UE_LOG(LogDFoundryFX, Warning, TEXT("Module: Platform don't support Multithreads."));
  }

  DFXThread = MakeShared<FDFX_Thread>();
}


void FDFX_Module::ShutdownModule()
{
  UE_LOG(LogDFoundryFX, Log, TEXT("Module: Closing DFoundryFX module."));
  DFXThread->Stop();
}

IMPLEMENT_MODULE(FDFX_Module, DFoundryFX)
#undef LOCTEXT_NAMESPACE
