#include "DFoundryFX_StatData.h"

void FDFX_StatData::RenderEngineTab_Data() {
  UWorld* World = ViewportClient->GetWorld();
  IConsoleManager& ConsoleManager = IConsoleManager::Get();
  UGameUserSettings* const GameUserSettings = GEngine->GetGameUserSettings();

  if (ImGui::CollapsingHeader("Memory Stats")) {
    FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
    if (ImGui::BeginTable("##MemoryTable", 5, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
      ImGui::TableSetupColumn("Metric");
      ImGui::TableSetupColumn("Current");
      ImGui::TableSetupColumn("Peak");
      ImGui::TableSetupColumn("Avail");
      ImGui::TableSetupColumn("Total");
      ImGui::TableHeadersRow();

      // Physical Memory
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("Physical");
      ImGui::TableNextColumn();
      ImGui::Text("%s", TCHAR_TO_ANSI(*GetMemoryString(MemoryStats.UsedPhysical)));
      ImGui::TableNextColumn();
      ImGui::Text("%s", TCHAR_TO_ANSI(*GetMemoryString(MemoryStats.PeakUsedPhysical)));
      ImGui::TableNextColumn();
      ImGui::Text("%s", TCHAR_TO_ANSI(*GetMemoryString(MemoryStats.AvailablePhysical)));
      ImGui::TableNextColumn();
      ImGui::Text("%s", TCHAR_TO_ANSI(*GetMemoryString(MemoryStats.TotalPhysical)));

      // Virtual Memory
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("Virtual");
      ImGui::TableNextColumn();
      ImGui::Text("%s", TCHAR_TO_ANSI(*GetMemoryString(MemoryStats.UsedVirtual)));
      ImGui::TableNextColumn();
      ImGui::Text("%s", TCHAR_TO_ANSI(*GetMemoryString(MemoryStats.PeakUsedVirtual)));
      ImGui::TableNextColumn();
      ImGui::Text("%s", TCHAR_TO_ANSI(*GetMemoryString(MemoryStats.AvailableVirtual)));
      ImGui::TableNextColumn();
      ImGui::Text("%s", TCHAR_TO_ANSI(*GetMemoryString(MemoryStats.TotalVirtual)));

      ImGui::EndTable();
    }
    // TODO: Expand section with per-platform specific extras (e.g., pagefile on Windows via FPlatformMisc::GetSystemMemoryStats())
  }

  if (ImGui::CollapsingHeader("Viewport Context")) {
    static bool bIsPlayInEditorViewport = ViewportClient->bIsPlayInEditorViewport;
    static float GetDPIScale = ViewportClient->GetDPIScale();
    static float GetDPIDerivedResolutionFraction = ViewportClient->GetDPIDerivedResolutionFraction();
    static bool bIsCursorVisible = ViewportClient->Viewport->IsCursorVisible();
    static bool bIsForegroundWindow = ViewportClient->Viewport->IsForegroundWindow();
    static bool bIsExclusiveFullscreen = ViewportClient->Viewport->IsExclusiveFullscreen();
    static bool bIsFullscreen = ViewportClient->Viewport->IsFullscreen();
    static bool bIsGameRenderingEnabled = ViewportClient->Viewport->IsGameRenderingEnabled();
    static bool bIsHDRViewport = ViewportClient->Viewport->IsHDRViewport();
    static bool bIsKeyboardAvailable = ViewportClient->Viewport->IsKeyboardAvailable(0);
    static bool bIsMouseAvailable = ViewportClient->Viewport->IsMouseAvailable(0);
    static bool bIsPenActive = ViewportClient->Viewport->IsPenActive();
    static bool bIsPlayInEditorViewport2 = ViewportClient->Viewport->IsPlayInEditorViewport();
    static bool bIsSlateViewport = ViewportClient->Viewport->IsSlateViewport();
    static bool bIsSoftwareCursorVisible = ViewportClient->Viewport->IsSoftwareCursorVisible();
    static bool bIsStereoRenderingAllowed = ViewportClient->Viewport->IsStereoRenderingAllowed();
    static int32 ViewportSizeX = ViewportClient->Viewport->GetSizeXY().X;
    static int32 ViewportSizeY = ViewportClient->Viewport->GetSizeXY().Y;
    static FString GetWorld = ViewportClient->GetWorld() ? ViewportClient->GetWorld()->GetName() : TEXT("None");
    static FString GetGameInstance = ViewportClient->GetGameInstance() ? ViewportClient->GetGameInstance()->GetName() : TEXT("None");
    static FString GetGameViewport = ViewportClient->GetGameViewport() ? TEXT("Valid") : TEXT("Invalid");

    ImGui::BeginDisabled();
    RenderInfoHelper(TEXT("bIsPlayInEditorViewport"), bIsPlayInEditorViewport);
    RenderInfoHelper(TEXT("GetDPIScale"), GetDPIScale);
    RenderInfoHelper(TEXT("GetDPIDerivedResolutionFraction"), GetDPIDerivedResolutionFraction);
    RenderInfoHelper(TEXT("IsCursorVisible"), bIsCursorVisible);
    RenderInfoHelper(TEXT("IsForegroundWindow"), bIsForegroundWindow);
    RenderInfoHelper(TEXT("IsExclusiveFullscreen"), bIsExclusiveFullscreen);
    RenderInfoHelper(TEXT("IsFullscreen"), bIsFullscreen);
    RenderInfoHelper(TEXT("IsGameRenderingEnabled"), bIsGameRenderingEnabled);
    RenderInfoHelper(TEXT("IsHDRViewport"), bIsHDRViewport);
    RenderInfoHelper(TEXT("IsKeyboardAvailable"), bIsKeyboardAvailable);
    RenderInfoHelper(TEXT("IsMouseAvailable"), bIsMouseAvailable);
    RenderInfoHelper(TEXT("IsPenActive"), bIsPenActive);
    RenderInfoHelper(TEXT("IsPlayInEditorViewport"), bIsPlayInEditorViewport2);
    RenderInfoHelper(TEXT("IsSlateViewport"), bIsSlateViewport);
    RenderInfoHelper(TEXT("IsSoftwareCursorVisible"), bIsSoftwareCursorVisible);
    RenderInfoHelper(TEXT("IsStereoRenderingAllowed"), bIsStereoRenderingAllowed);
    RenderInfoHelper(TEXT("ViewportSizeX"), ViewportSizeX);
    RenderInfoHelper(TEXT("ViewportSizeY"), ViewportSizeY);
    RenderInfoHelper(TEXT("GetWorld"), GetWorld);
    RenderInfoHelper(TEXT("GetGameInstance"), GetGameInstance);
    RenderInfoHelper(TEXT("GetGameViewport"), GetGameViewport);
    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("GEngine Context")) {
    static bool bIsEditor = GEngine->IsEditor();
    static bool bIsAllowedFramerateSmoothing = GEngine->IsAllowedFramerateSmoothing();
    static bool bForceDisableFrameRateSmoothing = GEngine->bForceDisableFrameRateSmoothing;
    static bool bSmoothFrameRate = GEngine->bSmoothFrameRate;
    static float MinDesiredFrameRate = GEngine->MinDesiredFrameRate;
    static bool bUseFixedFrameRate = GEngine->bUseFixedFrameRate;
    static float FixedFrameRate = GEngine->FixedFrameRate;
    static bool bCanBlueprintsTickByDefault = GEngine->bCanBlueprintsTickByDefault;
    static bool bIsControllerIdUsingPlatformUserId = GEngine->IsControllerIdUsingPlatformUserId();
    static bool bIsStereoscopic3D = GEngine->IsStereoscopic3D(ViewportClient->Viewport);
    static bool bIsVanillaProduct = GEngine->IsVanillaProduct();
    static bool bHasMultipleLocalPlayers = GEngine->HasMultipleLocalPlayers(World);
    static bool bAreEditorAnalyticsEnabled = GEngine->AreEditorAnalyticsEnabled();
    static bool bAllowMultiThreadedAnimationUpdate = GEngine->bAllowMultiThreadedAnimationUpdate;
    static bool bDisableAILogging = GEngine->bDisableAILogging;
    static bool bEnableOnScreenDebugMessages = GEngine->bEnableOnScreenDebugMessages;
    static bool bEnableOnScreenDebugMessagesDisplay = GEngine->bEnableOnScreenDebugMessagesDisplay;
    static bool bEnableEditorPSysRealtimeLOD = GEngine->bEnableEditorPSysRealtimeLOD;
    static uint32 bEnableVisualLogRecordingOnStart = GEngine->bEnableVisualLogRecordingOnStart;
    static bool bGenerateDefaultTimecode = GEngine->bGenerateDefaultTimecode;
    static bool bIsInitialized = GEngine->bIsInitialized;
    static bool bLockReadOnlyLevels = GEngine->bLockReadOnlyLevels;
    static bool bOptimizeAnimBlueprintMemberVariableAccess = GEngine->bOptimizeAnimBlueprintMemberVariableAccess;
    static bool bPauseOnLossOfFocus = GEngine->bPauseOnLossOfFocus;
    static bool bRenderLightMapDensityGrayscale = GEngine->bRenderLightMapDensityGrayscale;
    static bool bShouldGenerateLowQualityLightmaps_DEPRECATED = GEngine->bShouldGenerateLowQualityLightmaps_DEPRECATED;
    static float BSPSelectionHighlightIntensity = GEngine->BSPSelectionHighlightIntensity;
    static bool bStartedLoadMapMovie = GEngine->bStartedLoadMapMovie;
    static bool bSubtitlesEnabled = GEngine->bSubtitlesEnabled;
    static bool bSubtitlesForcedOff = GEngine->bSubtitlesForcedOff;
    static bool bSuppressMapWarnings = GEngine->bSuppressMapWarnings;
    static float DisplayGamma = GEngine->DisplayGamma;
    static bool bIsAutosaving = GEngine->IsAutosaving();
    static int32 MaximumLoopIterationCount = GEngine->MaximumLoopIterationCount;
    static float MaxLightMapDensity = GEngine->MaxLightMapDensity;
    static float MaxOcclusionPixelsFraction = GEngine->MaxOcclusionPixelsFraction;
    static float MaxParticleResize = GEngine->MaxParticleResize;
    static float MaxParticleResizeWarn = GEngine->MaxParticleResizeWarn;
    static int32 MaxPixelShaderAdditiveComplexityCount = GEngine->MaxPixelShaderAdditiveComplexityCount;
    static bool bUseSkeletalMeshMinLODPerQualityLevels = GEngine->UseSkeletalMeshMinLODPerQualityLevels;
    static bool bUseStaticMeshMinLODPerQualityLevels = GEngine->UseStaticMeshMinLODPerQualityLevels;

    ImGui::BeginDisabled();

    RenderInfoHelper(TEXT("IsEditor"), bIsEditor);
    RenderInfoHelper(TEXT("IsAllowedFramerateSmoothing"), bIsAllowedFramerateSmoothing);
    RenderInfoHelper(TEXT("bForceDisableFrameRateSmoothing"), bForceDisableFrameRateSmoothing);
    RenderInfoHelper(TEXT("bSmoothFrameRate"), bSmoothFrameRate);
    RenderInfoHelper(TEXT("MinDesiredFrameRate"), MinDesiredFrameRate);
    RenderInfoHelper(TEXT("bUseFixedFrameRate"), bUseFixedFrameRate);
    RenderInfoHelper(TEXT("FixedFrameRate"), FixedFrameRate);
    RenderInfoHelper(TEXT("bCanBlueprintsTickByDefault"), bCanBlueprintsTickByDefault);
    RenderInfoHelper(TEXT("IsControllerIdUsingPlatformUserId"), bIsControllerIdUsingPlatformUserId);
    RenderInfoHelper(TEXT("IsStereoscopic3D"), bIsStereoscopic3D);
    RenderInfoHelper(TEXT("IsVanillaProduct"), bIsVanillaProduct);
    RenderInfoHelper(TEXT("HasMultipleLocalPlayers"), bHasMultipleLocalPlayers);
    RenderInfoHelper(TEXT("AreEditorAnalyticsEnabled"), bAreEditorAnalyticsEnabled);
    RenderInfoHelper(TEXT("bAllowMultiThreadedAnimationUpdate"), bAllowMultiThreadedAnimationUpdate);
    RenderInfoHelper(TEXT("bDisableAILogging"), bDisableAILogging);
    RenderInfoHelper(TEXT("bEnableOnScreenDebugMessages"), bEnableOnScreenDebugMessages);
    RenderInfoHelper(TEXT("bEnableOnScreenDebugMessagesDisplay"), bEnableOnScreenDebugMessagesDisplay);
    RenderInfoHelper(TEXT("bEnableEditorPSysRealtimeLOD"), bEnableEditorPSysRealtimeLOD);
    RenderInfoHelper(TEXT("bEnableVisualLogRecordingOnStart"), bEnableVisualLogRecordingOnStart);
    RenderInfoHelper(TEXT("bGenerateDefaultTimecode"), bGenerateDefaultTimecode);
    RenderInfoHelper(TEXT("bIsInitialized"), bIsInitialized);
    RenderInfoHelper(TEXT("bLockReadOnlyLevels"), bLockReadOnlyLevels);
    RenderInfoHelper(TEXT("bOptimizeAnimBlueprintMemberVariableAccess"), bOptimizeAnimBlueprintMemberVariableAccess);
    RenderInfoHelper(TEXT("bPauseOnLossOfFocus"), bPauseOnLossOfFocus);
    RenderInfoHelper(TEXT("bRenderLightMapDensityGrayscale"), bRenderLightMapDensityGrayscale);
    RenderInfoHelper(TEXT("bShouldGenerateLowQualityLightmaps_DEPRECATED"), bShouldGenerateLowQualityLightmaps_DEPRECATED);
    RenderInfoHelper(TEXT("BSPSelectionHighlightIntensity"), BSPSelectionHighlightIntensity);
    RenderInfoHelper(TEXT("bStartedLoadMapMovie"), bStartedLoadMapMovie);
    RenderInfoHelper(TEXT("bSubtitlesEnabled"), bSubtitlesEnabled);
    RenderInfoHelper(TEXT("bSubtitlesForcedOff"), bSubtitlesForcedOff);
    RenderInfoHelper(TEXT("bSuppressMapWarnings"), bSuppressMapWarnings);
    RenderInfoHelper(TEXT("DisplayGamma"), DisplayGamma);
    RenderInfoHelper(TEXT("IsAutosaving"), bIsAutosaving);
    RenderInfoHelper(TEXT("MaximumLoopIterationCount"), MaximumLoopIterationCount);
    RenderInfoHelper(TEXT("MaxLightMapDensity"), MaxLightMapDensity);
    RenderInfoHelper(TEXT("MaxOcclusionPixelsFraction"), MaxOcclusionPixelsFraction);
    RenderInfoHelper(TEXT("MaxParticleResize"), MaxParticleResize);
    RenderInfoHelper(TEXT("MaxParticleResizeWarn"), MaxParticleResizeWarn);
    RenderInfoHelper(TEXT("MaxPixelShaderAdditiveComplexityCount"), MaxPixelShaderAdditiveComplexityCount);
    RenderInfoHelper(TEXT("UseSkeletalMeshMinLODPerQualityLevels"), bUseSkeletalMeshMinLODPerQualityLevels);
    RenderInfoHelper(TEXT("UseStaticMeshMinLODPerQualityLevels"), bUseStaticMeshMinLODPerQualityLevels);

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

    ImGui::EndDisabled();
    ImGui::Text("RHI Render Verts: %d", ImGui::GetIO().MetricsRenderVertices);

  }

  if (ImGui::CollapsingHeader("Platform Context")) {
    static bool bAllowAudioThread = FGenericPlatformMisc::AllowAudioThread();
    static bool bAllowLocalCaching = FGenericPlatformMisc::AllowLocalCaching();
    static bool bAllowThreadHeartBeat = FGenericPlatformMisc::AllowThreadHeartBeat();
    static FString CloudDir = FGenericPlatformMisc::CloudDir();
    static bool bDesktopTouchScreen = FGenericPlatformMisc::DesktopTouchScreen();
    static FString EngineDir = FGenericPlatformMisc::EngineDir();
    static bool bFullscreenSameAsWindowedFullscreen = FGenericPlatformMisc::FullscreenSameAsWindowedFullscreen();
    static FString GamePersistentDownloadDir = FGenericPlatformMisc::GamePersistentDownloadDir();
    static FString GameTemporaryDownloadDir = FGenericPlatformMisc::GameTemporaryDownloadDir();
    static float BatteryLevel = FGenericPlatformMisc::GetBatteryLevel();
    static float Brightness = FGenericPlatformMisc::GetBrightness();
    static FString CPUBrand = FGenericPlatformMisc::GetCPUBrand();
    static FString CPUChipset = FGenericPlatformMisc::GetCPUChipset();
    static uint32 CPUInfo = FGenericPlatformMisc::GetCPUInfo();
    static FString CPUVendor = FGenericPlatformMisc::GetCPUVendor();
    static FString DefaultDeviceProfileName = FGenericPlatformMisc::GetDefaultDeviceProfileName();
    static FString DefaultLanguage = FGenericPlatformMisc::GetDefaultLanguage();
    static FString DefaultLocale = FGenericPlatformMisc::GetDefaultLocale();
    static const TCHAR* DefaultPathSeparator = FGenericPlatformMisc::GetDefaultPathSeparator();
    static FString DeviceId = FGenericPlatformMisc::GetDeviceId();
    static FString DeviceMakeAndModel = FGenericPlatformMisc::GetDeviceMakeAndModel();
    static int32 DeviceTemperatureLevel = FGenericPlatformMisc::GetDeviceTemperatureLevel();
    static float DeviceVolume = FGenericPlatformMisc::GetDeviceVolume();
    static const TCHAR* EngineMode = FGenericPlatformMisc::GetEngineMode();
    static FString EpicAccountId = FGenericPlatformMisc::GetEpicAccountId();
    static FString FileManagerName = FGenericPlatformMisc::GetFileManagerName().ToString();
    static int32 LastError = FGenericPlatformMisc::GetLastError();
    static FString LocalCurrencyCode = FGenericPlatformMisc::GetLocalCurrencyCode();
    static FString LocalCurrencySymbol = FGenericPlatformMisc::GetLocalCurrencySymbol();
    static FString LoginId = FGenericPlatformMisc::GetLoginId();
    static int32 MaxPathLength = FGenericPlatformMisc::GetMaxPathLength();
    static float MaxRefreshRate = FGenericPlatformMisc::GetMaxRefreshRate();
    static float MaxSupportedRefreshRate = FGenericPlatformMisc::GetMaxSupportedRefreshRate();
    static float MaxSyncInterval = FGenericPlatformMisc::GetMaxSyncInterval();
    static int bMobilePropagateAlphaSetting = FGenericPlatformMisc::GetMobilePropagateAlphaSetting();
    static FString NullRHIShaderFormat = FGenericPlatformMisc::GetNullRHIShaderFormat();
    static FString OperatingSystemId = FGenericPlatformMisc::GetOperatingSystemId();
    static FString OSVersion = FGenericPlatformMisc::GetOSVersion();
    static const TCHAR* PathVarDelimiter = FGenericPlatformMisc::GetPathVarDelimiter();
    static FString PlatformFeaturesModuleName = FGenericPlatformMisc::GetPlatformFeaturesModuleName();
    static FString PrimaryGPUBrand = FGenericPlatformMisc::GetPrimaryGPUBrand();
    static FString TimeZoneId = FGenericPlatformMisc::GetTimeZoneId();
    static FString UBTPlatform = FGenericPlatformMisc::GetUBTPlatform();
    static FString UniqueAdvertisingId = FGenericPlatformMisc::GetUniqueAdvertisingId();
    static bool bUseVirtualJoysticks = FGenericPlatformMisc::GetUseVirtualJoysticks();
    static bool bVolumeButtonsHandledBySystem = FGenericPlatformMisc::GetVolumeButtonsHandledBySystem();
    static bool bHasActiveWiFiConnection = FGenericPlatformMisc::HasActiveWiFiConnection();
    static bool bHasMemoryWarningHandler = FGenericPlatformMisc::HasMemoryWarningHandler();
    static bool bHasNonoptionalCPUFeatures = FGenericPlatformMisc::HasNonoptionalCPUFeatures();
    static bool bHasProjectPersistentDownloadDir = FGenericPlatformMisc::HasProjectPersistentDownloadDir();
    static bool bHasSeparateChannelForDebugOutput = FGenericPlatformMisc::HasSeparateChannelForDebugOutput();
    static bool bHasVariableHardware = FGenericPlatformMisc::HasVariableHardware();
    static bool bIs64bitOperatingSystem = FGenericPlatformMisc::Is64bitOperatingSystem();
    static bool bIsDebuggerPresent = FGenericPlatformMisc::IsDebuggerPresent();
    static bool bIsEnsureAllowed = FGenericPlatformMisc::IsEnsureAllowed();
    static bool bIsInLowPowerMode = FGenericPlatformMisc::IsInLowPowerMode();
    static bool bIsLocalPrintThreadSafe = FGenericPlatformMisc::IsLocalPrintThreadSafe();
    static bool bIsPackagedForDistribution = FGenericPlatformMisc::IsPackagedForDistribution();
    static bool bIsPGOEnabled = FGenericPlatformMisc::IsPGOEnabled();
    static bool bIsRegisteredForRemoteNotifications = FGenericPlatformMisc::IsRegisteredForRemoteNotifications();
    static bool bIsRemoteSession = FGenericPlatformMisc::IsRemoteSession();
    static bool bIsRunningInCloud = FGenericPlatformMisc::IsRunningInCloud();
    static bool bIsRunningOnBattery = FGenericPlatformMisc::IsRunningOnBattery();
    static FString LaunchDir = FGenericPlatformMisc::LaunchDir();
    static bool bNeedsNonoptionalCPUFeaturesCheck = FGenericPlatformMisc::NeedsNonoptionalCPUFeaturesCheck();
    static int32 NumberOfCores = FGenericPlatformMisc::NumberOfCores();
    static int32 NumberOfCoresIncludingHyperthreads = FGenericPlatformMisc::NumberOfCoresIncludingHyperthreads();
    static int32 NumberOfIOWorkerThreadsToSpawn = FGenericPlatformMisc::NumberOfIOWorkerThreadsToSpawn();
    static int32 NumberOfWorkerThreadsToSpawn = FGenericPlatformMisc::NumberOfWorkerThreadsToSpawn();
    static FString ProjectDir = FGenericPlatformMisc::ProjectDir();
    static bool bSupportsBackbufferSampling = FGenericPlatformMisc::SupportsBackbufferSampling();
    static bool bSupportsBrightness = FGenericPlatformMisc::SupportsBrightness();
    static bool bSupportsDeviceCheckToken = FGenericPlatformMisc::SupportsDeviceCheckToken();
    static bool bSupportsForceTouchInput = FGenericPlatformMisc::SupportsForceTouchInput();
    static bool bSupportsFullCrashDumps = FGenericPlatformMisc::SupportsFullCrashDumps();
    static bool bSupportsLocalCaching = FGenericPlatformMisc::SupportsLocalCaching();
    static bool bSupportsMessaging = FGenericPlatformMisc::SupportsMessaging();
    static bool bSupportsMultithreadedFileHandles = FGenericPlatformMisc::SupportsMultithreadedFileHandles();
    static bool bSupportsTouchInput = FGenericPlatformMisc::SupportsTouchInput();
    static bool bUseHDRByDefault = FGenericPlatformMisc::UseHDRByDefault();
    static bool bUseRenderThread = FGenericPlatformMisc::UseRenderThread();

    ImGui::BeginDisabled();
    RenderInfoHelper(TEXT("AllowAudioThread"), bAllowAudioThread);
    RenderInfoHelper(TEXT("AllowLocalCaching"), bAllowLocalCaching);
    RenderInfoHelper(TEXT("AllowThreadHeartBeat"), bAllowThreadHeartBeat);
    RenderInfoHelper(TEXT("CloudDir"), CloudDir);
    RenderInfoHelper(TEXT("DesktopTouchScreen"), bDesktopTouchScreen);
    RenderInfoHelper(TEXT("EngineDir"), EngineDir);
    RenderInfoHelper(TEXT("FullscreenSameAsWindowedFullscreen"), bFullscreenSameAsWindowedFullscreen);
    RenderInfoHelper(TEXT("GamePersistentDownloadDir"), GamePersistentDownloadDir);
    RenderInfoHelper(TEXT("GameTemporaryDownloadDir"), GameTemporaryDownloadDir);
    RenderInfoHelper(TEXT("GetBatteryLevel"), BatteryLevel);
    RenderInfoHelper(TEXT("GetBrightness"), Brightness);
    RenderInfoHelper(TEXT("GetCPUBrand"), CPUBrand);
    RenderInfoHelper(TEXT("GetCPUChipset"), CPUChipset);
    RenderInfoHelper(TEXT("GetCPUInfo"), CPUInfo);
    RenderInfoHelper(TEXT("GetCPUVendor"), CPUVendor);
    RenderInfoHelper(TEXT("GetDefaultDeviceProfileName"), DefaultDeviceProfileName);
    RenderInfoHelper(TEXT("GetDefaultLanguage"), DefaultLanguage);
    RenderInfoHelper(TEXT("GetDefaultLocale"), DefaultLocale);
    RenderInfoHelper(TEXT("GetDefaultPathSeparator"), DefaultPathSeparator);
    RenderInfoHelper(TEXT("GetDeviceId"), DeviceId);
    RenderInfoHelper(TEXT("GetDeviceMakeAndModel"), DeviceMakeAndModel);
    RenderInfoHelper(TEXT("GetDeviceTemperatureLevel"), DeviceTemperatureLevel);
    RenderInfoHelper(TEXT("GetDeviceVolume"), DeviceVolume);
    RenderInfoHelper(TEXT("GetEngineMode"), EngineMode);
    RenderInfoHelper(TEXT("GetEpicAccountId"), EpicAccountId);
    RenderInfoHelper(TEXT("GetFileManagerName"), FileManagerName);
    RenderInfoHelper(TEXT("GetLastError"), LastError);
    RenderInfoHelper(TEXT("GetLocalCurrencyCode"), LocalCurrencyCode);
    RenderInfoHelper(TEXT("GetLocalCurrencySymbol"), LocalCurrencySymbol);
    RenderInfoHelper(TEXT("GetLoginId"), LoginId);
    RenderInfoHelper(TEXT("GetMaxPathLength"), MaxPathLength);
    RenderInfoHelper(TEXT("GetMaxRefreshRate"), MaxRefreshRate);
    RenderInfoHelper(TEXT("GetMaxSupportedRefreshRate"), MaxSupportedRefreshRate);
    RenderInfoHelper(TEXT("GetMaxSyncInterval"), MaxSyncInterval);
    RenderInfoHelper(TEXT("GetMobilePropagateAlphaSetting"), bMobilePropagateAlphaSetting);
    RenderInfoHelper(TEXT("GetNullRHIShaderFormat"), NullRHIShaderFormat);
    RenderInfoHelper(TEXT("GetOperatingSystemId"), OperatingSystemId);
    RenderInfoHelper(TEXT("GetOSVersion"), OSVersion);
    RenderInfoHelper(TEXT("GetPathVarDelimiter"), PathVarDelimiter);
    RenderInfoHelper(TEXT("GetPlatformFeaturesModuleName"), PlatformFeaturesModuleName);
    RenderInfoHelper(TEXT("GetPrimaryGPUBrand"), PrimaryGPUBrand);
    RenderInfoHelper(TEXT("GetTimeZoneId"), TimeZoneId);
    RenderInfoHelper(TEXT("GetUBTPlatform"), UBTPlatform);
    RenderInfoHelper(TEXT("GetUniqueAdvertisingId"), UniqueAdvertisingId);
    RenderInfoHelper(TEXT("GetUseVirtualJoysticks"), bUseVirtualJoysticks);
    RenderInfoHelper(TEXT("GetVolumeButtonsHandledBySystem"), bVolumeButtonsHandledBySystem);
    RenderInfoHelper(TEXT("HasActiveWiFiConnection"), bHasActiveWiFiConnection);
    RenderInfoHelper(TEXT("HasMemoryWarningHandler"), bHasMemoryWarningHandler);
    RenderInfoHelper(TEXT("HasNonoptionalCPUFeatures"), bHasNonoptionalCPUFeatures);
    RenderInfoHelper(TEXT("HasProjectPersistentDownloadDir"), bHasProjectPersistentDownloadDir);
    RenderInfoHelper(TEXT("HasSeparateChannelForDebugOutput"), bHasSeparateChannelForDebugOutput);
    RenderInfoHelper(TEXT("HasVariableHardware"), bHasVariableHardware);
    RenderInfoHelper(TEXT("Is64bitOperatingSystem"), bIs64bitOperatingSystem);
    RenderInfoHelper(TEXT("IsDebuggerPresent"), bIsDebuggerPresent);
    RenderInfoHelper(TEXT("IsEnsureAllowed"), bIsEnsureAllowed);
    RenderInfoHelper(TEXT("IsInLowPowerMode"), bIsInLowPowerMode);
    RenderInfoHelper(TEXT("IsLocalPrintThreadSafe"), bIsLocalPrintThreadSafe);
    RenderInfoHelper(TEXT("IsPackagedForDistribution"), bIsPackagedForDistribution);
    RenderInfoHelper(TEXT("IsPGOEnabled"), bIsPGOEnabled);
    RenderInfoHelper(TEXT("IsRegisteredForRemoteNotifications"), bIsRegisteredForRemoteNotifications);
    RenderInfoHelper(TEXT("IsRemoteSession"), bIsRemoteSession);
    RenderInfoHelper(TEXT("IsRunningInCloud"), bIsRunningInCloud);
    RenderInfoHelper(TEXT("IsRunningOnBattery"), bIsRunningOnBattery);
    RenderInfoHelper(TEXT("LaunchDir"), LaunchDir);
    RenderInfoHelper(TEXT("NeedsNonoptionalCPUFeaturesCheck"), bNeedsNonoptionalCPUFeaturesCheck);
    RenderInfoHelper(TEXT("NumberOfCores"), NumberOfCores);
    RenderInfoHelper(TEXT("NumberOfCoresIncludingHyperthreads"), NumberOfCoresIncludingHyperthreads);
    RenderInfoHelper(TEXT("NumberOfIOWorkerThreadsToSpawn"), NumberOfIOWorkerThreadsToSpawn);
    RenderInfoHelper(TEXT("NumberOfWorkerThreadsToSpawn"), NumberOfWorkerThreadsToSpawn);
    RenderInfoHelper(TEXT("ProjectDir"), ProjectDir);
    RenderInfoHelper(TEXT("SupportsBackbufferSampling"), bSupportsBackbufferSampling);
    RenderInfoHelper(TEXT("SupportsBrightness"), bSupportsBrightness);
    RenderInfoHelper(TEXT("SupportsDeviceCheckToken"), bSupportsDeviceCheckToken);
    RenderInfoHelper(TEXT("SupportsForceTouchInput"), bSupportsForceTouchInput);
    RenderInfoHelper(TEXT("SupportsFullCrashDumps"), bSupportsFullCrashDumps);
    RenderInfoHelper(TEXT("SupportsLocalCaching"), bSupportsLocalCaching);
    RenderInfoHelper(TEXT("SupportsMessaging"), bSupportsMessaging);
    RenderInfoHelper(TEXT("SupportsMultithreadedFileHandles"), bSupportsMultithreadedFileHandles);
    RenderInfoHelper(TEXT("SupportsTouchInput"), bSupportsTouchInput);
    RenderInfoHelper(TEXT("UseHDRByDefault"), bUseHDRByDefault);
    RenderInfoHelper(TEXT("UseRenderThread"), bUseRenderThread);

    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("Rendering Context")) {
    static bool RenderingCached = false;
    static int32 ShadowCSMMaxCascades = 0;
    static int32 ShadowMaxResolution = 0;
    static int32 ShadowQuality = 0;
    static bool bMobileEnableStaticAndCSMShadowReceivers = false;
    static bool bMobileEnableMovableLightCSMShadowsMobile = false;
    static bool bAllowGlobalClipPlane = false;
    static bool bEnableForwardRenderer = false;
    static int32 GPUSkinSkinVertexInfluences = 0;
    static int32 MaterialQualityLevel = 0;
    static int32 PostProcessAAQuality = 0;
    static int32 TranslucencyLightingVolumeDim = 0;

    if (!RenderingCached) {
      IConsoleVariable* CVar = nullptr;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("r.Shadow.CSM.MaxCascades"));
      ShadowCSMMaxCascades = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("r.Shadow.MaxResolution"));
      ShadowMaxResolution = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("r.ShadowQuality"));
      ShadowQuality = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("r.Mobile.EnableStaticAndCSMShadowReceivers"));
      bMobileEnableStaticAndCSMShadowReceivers = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("r.Mobile.EnableMovableLightCSMShadowsMobile"));
      bMobileEnableMovableLightCSMShadowsMobile = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("r.AllowGlobalClipPlane"));
      bAllowGlobalClipPlane = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("r.EnableForwardRenderer"));
      bEnableForwardRenderer = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("r.GPUSkin.SkinVertexInfluences"));
      GPUSkinSkinVertexInfluences = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("r.MaterialQualityLevel"));
      MaterialQualityLevel = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("r.PostProcessAAQuality"));
      PostProcessAAQuality = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("r.TranslucencyLightingVolumeDim"));
      TranslucencyLightingVolumeDim = CVar ? CVar->GetInt() : 0;

      RenderingCached = true;
    }

    ImGui::BeginDisabled();

    RenderInfoHelper(TEXT("GFrameNumber"), GFrameNumber);
    RenderInfoHelper(TEXT("r.Shadow.CSM.MaxCascades"), ShadowCSMMaxCascades);
    RenderInfoHelper(TEXT("r.Shadow.MaxResolution"), ShadowMaxResolution);
    RenderInfoHelper(TEXT("r.ShadowQuality"), ShadowQuality);
    RenderInfoHelper(TEXT("r.Mobile.EnableStaticAndCSMShadowReceivers"), bMobileEnableStaticAndCSMShadowReceivers);
    RenderInfoHelper(TEXT("r.Mobile.EnableMovableLightCSMShadowsMobile"), bMobileEnableMovableLightCSMShadowsMobile);
    RenderInfoHelper(TEXT("r.AllowGlobalClipPlane"), bAllowGlobalClipPlane);
    RenderInfoHelper(TEXT("r.EnableForwardRenderer"), bEnableForwardRenderer);
    RenderInfoHelper(TEXT("r.GPUSkin.SkinVertexInfluences"), GPUSkinSkinVertexInfluences);
    RenderInfoHelper(TEXT("r.MaterialQualityLevel"), MaterialQualityLevel);
    RenderInfoHelper(TEXT("r.PostProcessAAQuality"), PostProcessAAQuality);
    RenderInfoHelper(TEXT("r.TranslucencyLightingVolumeDim"), TranslucencyLightingVolumeDim);

    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("Audio Context")) {
    static bool AudioCached = false;
    static int32 AudioQuality = 0;
    static int32 ReverbQuality = 0;
    static bool bHRTF = false;
    static bool bAllowBackgroundAudio = false;
    static bool bAuDebug = false;
    static bool bAuDebugSoundMixes = false;
    static bool bAuDebugMetasounds = false;
    static int32 AuDebugNumActiveVoices = 0;
    static int32 AuDebugNumActiveSounds = 0;
    static int32 AuDebugNumActiveWaves = 0;
    static int32 AuDebugNumActiveSoundMixes = 0;
    static int32 AuDebugNumActiveSoundClasses = 0;

    if (!AudioCached) {
      IConsoleVariable* CVar = nullptr;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("a.AudioQuality"));
      AudioQuality = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("a.ReverbQuality"));
      ReverbQuality = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("a.HRTF"));
      bHRTF = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("a.AllowBackgroundAudio"));
      bAllowBackgroundAudio = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("au.Debug"));
      bAuDebug = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("au.Debug.SoundMixes"));
      bAuDebugSoundMixes = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("au.Debug.Metasounds"));
      bAuDebugMetasounds = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("au.Debug.NumActiveVoices"));
      AuDebugNumActiveVoices = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("au.Debug.NumActiveSounds"));
      AuDebugNumActiveSounds = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("au.Debug.NumActiveWaves"));
      AuDebugNumActiveWaves = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("au.Debug.NumActiveSoundMixes"));
      AuDebugNumActiveSoundMixes = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("au.Debug.NumActiveSoundClasses"));
      AuDebugNumActiveSoundClasses = CVar ? CVar->GetInt() : 0;

      AudioCached = true;
    }

    ImGui::BeginDisabled();

    RenderInfoHelper(TEXT("a.AudioQuality"), AudioQuality);
    RenderInfoHelper(TEXT("a.ReverbQuality"), ReverbQuality);
    RenderInfoHelper(TEXT("a.HRTF"), bHRTF);
    RenderInfoHelper(TEXT("a.AllowBackgroundAudio"), bAllowBackgroundAudio);
    RenderInfoHelper(TEXT("au.Debug"), bAuDebug);
    RenderInfoHelper(TEXT("au.Debug.SoundMixes"), bAuDebugSoundMixes);
    RenderInfoHelper(TEXT("au.Debug.Metasounds"), bAuDebugMetasounds);
    RenderInfoHelper(TEXT("au.Debug.NumActiveVoices"), AuDebugNumActiveVoices);
    RenderInfoHelper(TEXT("au.Debug.NumActiveSounds"), AuDebugNumActiveSounds);
    RenderInfoHelper(TEXT("au.Debug.NumActiveWaves"), AuDebugNumActiveWaves);
    RenderInfoHelper(TEXT("au.Debug.NumActiveSoundMixes"), AuDebugNumActiveSoundMixes);
    RenderInfoHelper(TEXT("au.Debug.NumActiveSoundClasses"), AuDebugNumActiveSoundClasses);

    ImGui::EndDisabled();
  }

  static APlayerController* PC = ViewportClient->GetWorld() ? ViewportClient->GetWorld()->GetFirstPlayerController() : nullptr;
  if (PC) {
    if (ImGui::CollapsingHeader("Input Context")) {
      ImGui::BeginDisabled();

      static bool bShowMouseCursor = PC->bShowMouseCursor;
      static bool bEnableClickEvents = PC->bEnableClickEvents;
      static bool bEnableMouseOverEvents = PC->bEnableMouseOverEvents;
      static bool bEnableTouchEvents = PC->bEnableTouchEvents;
      static bool bIsInputKeyDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
      static bool bAutoManageActiveCameraTarget = PC->bAutoManageActiveCameraTarget;
      static FString AcknowledgedPawn = PC->AcknowledgedPawn ? PC->AcknowledgedPawn->GetName() : TEXT("None");
      static FString PlayerCameraManager = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetName() : TEXT("None");

      RenderInfoHelper(TEXT("bShowMouseCursor"), bShowMouseCursor);
      RenderInfoHelper(TEXT("bEnableClickEvents"), bEnableClickEvents);
      RenderInfoHelper(TEXT("bEnableMouseOverEvents"), bEnableMouseOverEvents);
      RenderInfoHelper(TEXT("bEnableTouchEvents"), bEnableTouchEvents);
      RenderInfoHelper(TEXT("bIsInputKeyDown (LMB)"), bIsInputKeyDown);
      RenderInfoHelper(TEXT("bAutoManageActiveCameraTarget"), bAutoManageActiveCameraTarget);
      RenderInfoHelper(TEXT("AcknowledgedPawn"), AcknowledgedPawn);
      RenderInfoHelper(TEXT("PlayerCameraManager"), PlayerCameraManager);

      ImGui::EndDisabled();
    }
  }

  if (ImGui::CollapsingHeader("Network Context")) {
    static bool NetworkCached = false;
    static int32 NetServerMaxTickRate = 0;
    static int32 NetClientMaxTickRate = 0;
    static float NetPacketLag = 0.0f;
    static float NetPacketLoss = 0.0f;
    static int32 NetPacketDup = 0;
    static bool NetPacketNotify = false;
    static bool NetServerFlushNetDirty = false;
    static float NetServerFlushNetDirtyTime = 0.0f;
    static bool NetServerFlushNetDirtyBuffers = false;
    static bool NetServerFlushNetDirtyActors = false;
    static bool NetServerFlushNetDirtyProperties = false;
    static bool NetServerFlushNetDirtyComponents = false;
    static bool NetClientFlushNetDirty = false;
    static float NetClientFlushNetDirtyTime = 0.0f;
    static bool NetClientFlushNetDirtyBuffers = false;
    static bool NetClientFlushNetDirtyActors = false;
    static bool NetClientFlushNetDirtyProperties = false;
    static bool NetClientFlushNetDirtyComponents = false;
    static bool NetDebugNet = false;
    static bool NetDebugNetReplay = false;
    static int32 NetDormancy = 0;
    static float NetDormancyThreshold = 0.0f;
    static bool NetReplayGraph = false;

    if (!NetworkCached) {
      IConsoleVariable* CVar = nullptr;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ServerMaxTickRate"));
      NetServerMaxTickRate = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ClientMaxTickRate"));
      NetClientMaxTickRate = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.PacketLag"));
      NetPacketLag = CVar ? CVar->GetFloat() : 0.0f;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.PacketLoss"));
      NetPacketLoss = CVar ? CVar->GetFloat() : 0.0f;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.PacketDup"));
      NetPacketDup = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.PacketNotify"));
      NetPacketNotify = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ServerFlushNetDirty"));
      NetServerFlushNetDirty = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ServerFlushNetDirtyTime"));
      NetServerFlushNetDirtyTime = CVar ? CVar->GetFloat() : 0.0f;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ServerFlushNetDirtyBuffers"));
      NetServerFlushNetDirtyBuffers = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ServerFlushNetDirtyActors"));
      NetServerFlushNetDirtyActors = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ServerFlushNetDirtyProperties"));
      NetServerFlushNetDirtyProperties = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ServerFlushNetDirtyComponents"));
      NetServerFlushNetDirtyComponents = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ClientFlushNetDirty"));
      NetClientFlushNetDirty = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ClientFlushNetDirtyTime"));
      NetClientFlushNetDirtyTime = CVar ? CVar->GetFloat() : 0.0f;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ClientFlushNetDirtyBuffers"));
      NetClientFlushNetDirtyBuffers = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ClientFlushNetDirtyActors"));
      NetClientFlushNetDirtyActors = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ClientFlushNetDirtyProperties"));
      NetClientFlushNetDirtyProperties = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ClientFlushNetDirtyComponents"));
      NetClientFlushNetDirtyComponents = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.DebugNet"));
      NetDebugNet = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.DebugNetReplay"));
      NetDebugNetReplay = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.Dormancy"));
      NetDormancy = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.DormancyThreshold"));
      NetDormancyThreshold = CVar ? CVar->GetFloat() : 0.0f;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("net.ReplayGraph"));
      NetReplayGraph = CVar ? CVar->GetBool() : false;

      NetworkCached = true;
    }

    ImGui::BeginDisabled();

    RenderInfoHelper(TEXT("net.ServerMaxTickRate"), NetServerMaxTickRate);
    RenderInfoHelper(TEXT("net.ClientMaxTickRate"), NetClientMaxTickRate);
    RenderInfoHelper(TEXT("net.PacketLag"), NetPacketLag);
    RenderInfoHelper(TEXT("net.PacketDup"), NetPacketDup);
    RenderInfoHelper(TEXT("net.PacketLoss"), NetPacketLoss);
    RenderInfoHelper(TEXT("net.PacketNotify"), NetPacketNotify);
    RenderInfoHelper(TEXT("net.ServerFlushNetDirty"), NetServerFlushNetDirty);
    RenderInfoHelper(TEXT("net.ServerFlushNetDirtyTime"), NetServerFlushNetDirtyTime);
    RenderInfoHelper(TEXT("net.ServerFlushNetDirtyBuffers"), NetServerFlushNetDirtyBuffers);
    RenderInfoHelper(TEXT("net.ServerFlushNetDirtyActors"), NetServerFlushNetDirtyActors);
    RenderInfoHelper(TEXT("net.ServerFlushNetDirtyProperties"), NetServerFlushNetDirtyProperties);
    RenderInfoHelper(TEXT("net.ServerFlushNetDirtyComponents"), NetServerFlushNetDirtyComponents);
    RenderInfoHelper(TEXT("net.ClientFlushNetDirty"), NetClientFlushNetDirty);
    RenderInfoHelper(TEXT("net.ClientFlushNetDirtyTime"), NetClientFlushNetDirtyTime);
    RenderInfoHelper(TEXT("net.ClientFlushNetDirtyBuffers"), NetClientFlushNetDirtyBuffers);
    RenderInfoHelper(TEXT("net.ClientFlushNetDirtyActors"), NetClientFlushNetDirtyActors);
    RenderInfoHelper(TEXT("net.ClientFlushNetDirtyProperties"), NetClientFlushNetDirtyProperties);
    RenderInfoHelper(TEXT("net.ClientFlushNetDirtyComponents"), NetClientFlushNetDirtyComponents);
    RenderInfoHelper(TEXT("net.DebugNet"), NetDebugNet);
    RenderInfoHelper(TEXT("net.DebugNetReplay"), NetDebugNetReplay);
    RenderInfoHelper(TEXT("net.Dormancy"), NetDormancy);
    RenderInfoHelper(TEXT("net.DormancyThreshold"), NetDormancyThreshold);
    RenderInfoHelper(TEXT("net.ReplayGraph"), NetReplayGraph);

    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("Physics Context")) {
    static bool PhysicsCached = false;
    static float PhysicsSubstepDeltaTime = 0.0f;
    static int32 NumSubsteps = 0;
    static float MaxSubstepDeltaTime = 0.0f;
    static int32 MaxSubsteps = 0;
    static bool bEnable2DPhysics = false;
    static int32 ChaosThreadGroupCount = 0;
    static int32 ChaosThreadGroupSize = 0;
    static bool bChaosSolverActorEnableCCD = false;
    static bool bChaosSolverActorEnableStabilization = false;
    static bool bChaosEnableGPUSolver = false;
    static bool bChaosEnableGPUSolverForSingleThreaded = false;
    static int32 ChaosGPUSolverBufferSize = 0;
    static int32 ChaosGPUSolverMaxActiveKernels = 0;
    static bool bChaosUseImguiDebugDraw = false;
    static bool bChaosDebugDrawEnabled = false;
    static float ChaosDebugDrawScale = 1.0f;
    static bool bChaosSolverActorShowDepth = false;
    static float ChaosSolverActorShowDepthCutoff = 0.0f;
    static bool bChaosSolverActorShowIslandGroup = false;
    static int32 ChaosSolverActorShowIslandGroupCutoff = 0;
    static bool bChaosSolverActorShowEvents = false;
    static bool bChaosSolverActorShowCollisionData = false;
    static bool bChaosSolverActorShowConstraintBreaking = false;
    static bool bChaosSolverActorShowConstraintRules = false;
    static bool bChaosSolverActorShowGeometryCollectionData = false;
    static bool bChaosSolverActorShowGeometryCollectionConstraintData = false;
    static bool bChaosSolverActorShowGeometryCollectionConstraintBreaking = false;

    if (!PhysicsCached) {
      IConsoleVariable* CVar = nullptr;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.PhysicsSubstepDeltaTime"));
      PhysicsSubstepDeltaTime = CVar ? CVar->GetFloat() : 0.0f;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.NumSubsteps"));
      NumSubsteps = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.MaxSubstepDeltaTime"));
      MaxSubstepDeltaTime = CVar ? CVar->GetFloat() : 0.0f;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.MaxSubsteps"));
      MaxSubsteps = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.Enable2DPhysics"));
      bEnable2DPhysics = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosThreadGroupCount"));
      ChaosThreadGroupCount = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosThreadGroupSize"));
      ChaosThreadGroupSize = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorEnableCCD"));
      bChaosSolverActorEnableCCD = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorEnableStabilization"));
      bChaosSolverActorEnableStabilization = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosEnableGPUSolver"));
      bChaosEnableGPUSolver = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosEnableGPUSolverForSingleThreaded"));
      bChaosEnableGPUSolverForSingleThreaded = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosGPUSolverBufferSize"));
      ChaosGPUSolverBufferSize = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosGPUSolverMaxActiveKernels"));
      ChaosGPUSolverMaxActiveKernels = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosUseImguiDebugDraw"));
      bChaosUseImguiDebugDraw = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosDebugDrawEnabled"));
      bChaosDebugDrawEnabled = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosDebugDrawScale"));
      ChaosDebugDrawScale = CVar ? CVar->GetFloat() : 1.0f;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorShowDepth"));
      bChaosSolverActorShowDepth = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorShowDepthCutoff"));
      ChaosSolverActorShowDepthCutoff = CVar ? CVar->GetFloat() : 0.0f;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorShowIslandGroup"));
      bChaosSolverActorShowIslandGroup = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorShowIslandGroupCutoff"));
      ChaosSolverActorShowIslandGroupCutoff = CVar ? CVar->GetInt() : 0;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorShowEvents"));
      bChaosSolverActorShowEvents = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorShowCollisionData"));
      bChaosSolverActorShowCollisionData = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorShowConstraintBreaking"));
      bChaosSolverActorShowConstraintBreaking = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorShowConstraintRules"));
      bChaosSolverActorShowConstraintRules = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorShowGeometryCollectionData"));
      bChaosSolverActorShowGeometryCollectionData = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorShowGeometryCollectionConstraintData"));
      bChaosSolverActorShowGeometryCollectionConstraintData = CVar ? CVar->GetBool() : false;

      CVar = ConsoleManager.FindConsoleVariable(TEXT("p.ChaosSolverActorShowGeometryCollectionConstraintBreaking"));
      bChaosSolverActorShowGeometryCollectionConstraintBreaking = CVar ? CVar->GetBool() : false;

      PhysicsCached = true;
    }

    ImGui::BeginDisabled();

    RenderInfoHelper(TEXT("p.PhysicsSubstepDeltaTime"), PhysicsSubstepDeltaTime);
    RenderInfoHelper(TEXT("p.NumSubsteps"), NumSubsteps);
    RenderInfoHelper(TEXT("p.MaxSubstepDeltaTime"), MaxSubstepDeltaTime);
    RenderInfoHelper(TEXT("p.MaxSubsteps"), MaxSubsteps);
    RenderInfoHelper(TEXT("p.Enable2DPhysics"), bEnable2DPhysics);
    RenderInfoHelper(TEXT("p.ChaosThreadGroupCount"), ChaosThreadGroupCount);
    RenderInfoHelper(TEXT("p.ChaosThreadGroupSize"), ChaosThreadGroupSize);
    RenderInfoHelper(TEXT("p.ChaosSolverActorEnableCCD"), bChaosSolverActorEnableCCD);
    RenderInfoHelper(TEXT("p.ChaosSolverActorEnableStabilization"), bChaosSolverActorEnableStabilization);
    RenderInfoHelper(TEXT("p.ChaosEnableGPUSolver"), bChaosEnableGPUSolver);
    RenderInfoHelper(TEXT("p.ChaosEnableGPUSolverForSingleThreaded"), bChaosEnableGPUSolverForSingleThreaded);
    RenderInfoHelper(TEXT("p.ChaosGPUSolverBufferSize"), ChaosGPUSolverBufferSize);
    RenderInfoHelper(TEXT("p.ChaosGPUSolverMaxActiveKernels"), ChaosGPUSolverMaxActiveKernels);
    RenderInfoHelper(TEXT("p.ChaosUseImguiDebugDraw"), bChaosUseImguiDebugDraw);
    RenderInfoHelper(TEXT("p.ChaosDebugDrawEnabled"), bChaosDebugDrawEnabled);
    RenderInfoHelper(TEXT("p.ChaosDebugDrawScale"), ChaosDebugDrawScale);
    RenderInfoHelper(TEXT("p.ChaosSolverActorShowDepth"), bChaosSolverActorShowDepth);
    RenderInfoHelper(TEXT("p.ChaosSolverActorShowDepthCutoff"), ChaosSolverActorShowDepthCutoff);
    RenderInfoHelper(TEXT("p.ChaosSolverActorShowIslandGroup"), bChaosSolverActorShowIslandGroup);
    RenderInfoHelper(TEXT("p.ChaosSolverActorShowIslandGroupCutoff"), ChaosSolverActorShowIslandGroupCutoff);
    RenderInfoHelper(TEXT("p.ChaosSolverActorShowEvents"), bChaosSolverActorShowEvents);
    RenderInfoHelper(TEXT("p.ChaosSolverActorShowCollisionData"), bChaosSolverActorShowCollisionData);
    RenderInfoHelper(TEXT("p.ChaosSolverActorShowConstraintBreaking"), bChaosSolverActorShowConstraintBreaking);
    RenderInfoHelper(TEXT("p.ChaosSolverActorShowConstraintRules"), bChaosSolverActorShowConstraintRules);
    RenderInfoHelper(TEXT("p.ChaosSolverActorShowGeometryCollectionData"), bChaosSolverActorShowGeometryCollectionData);
    RenderInfoHelper(TEXT("p.ChaosSolverActorShowGeometryCollectionConstraintData"), bChaosSolverActorShowGeometryCollectionConstraintData);
    RenderInfoHelper(TEXT("p.ChaosSolverActorShowGeometryCollectionConstraintBreaking"), bChaosSolverActorShowGeometryCollectionConstraintBreaking);

    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("World Context")) {
    static int32 ActorCount = World->GetActorCount();
    static int32 NumLevels = World->GetNumLevels();
    static uint8 bPlayersOnly = World->bPlayersOnly;
    static uint8 bStartup = World->bStartup;
    static uint8 bShouldSimulatePhysics = World->bShouldSimulatePhysics;
    static uint8 bEnableWorldBoundsChecks = World->GetWorldSettings()->bEnableWorldBoundsChecks;
    static uint8 bPlaceCellsOnlyAlongCameraTracks = World->GetWorldSettings()->bPlaceCellsOnlyAlongCameraTracks;
    static float GlobalGravityZ = World->GetWorldSettings()->GlobalGravityZ;
    static float WorldToMeters = World->GetWorldSettings()->WorldToMeters;
    static float EffectiveTimeDilation = World->GetWorldSettings()->GetEffectiveTimeDilation();
    static float DemoPlayTimeDilation = World->GetWorldSettings()->DemoPlayTimeDilation;

    ImGui::BeginDisabled();

    RenderInfoHelper(TEXT("ActorCount"), ActorCount);
    RenderInfoHelper(TEXT("NumLevels"), NumLevels);
    RenderInfoHelper(TEXT("bPlayersOnly"), bPlayersOnly);
    RenderInfoHelper(TEXT("bStartup"), bStartup);
    RenderInfoHelper(TEXT("bShouldSimulatePhysics"), bShouldSimulatePhysics);
    RenderInfoHelper(TEXT("bEnableWorldBoundsChecks"), bEnableWorldBoundsChecks);
    RenderInfoHelper(TEXT("bPlaceCellsOnlyAlongCameraTracks"), bPlaceCellsOnlyAlongCameraTracks);
    RenderInfoHelper(TEXT("GlobalGravityZ"), GlobalGravityZ);
    RenderInfoHelper(TEXT("WorldToMeters"), WorldToMeters);
    RenderInfoHelper(TEXT("EffectiveTimeDilation"), EffectiveTimeDilation);
    RenderInfoHelper(TEXT("DemoPlayTimeDilation"), DemoPlayTimeDilation);

    ImGui::EndDisabled();
  }

// Platform specific context.
}