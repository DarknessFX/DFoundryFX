using System.IO;
using UnrealBuildTool;

public class DFoundryFX : ModuleRules
{
	public DFoundryFX(ReadOnlyTargetRules Target) : base(Target)
	{
    PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

    PublicIncludePaths.AddRange(
			new string[] {
        Path.Combine(ModuleDirectory, "Public"),
        Path.Combine(PluginDirectory, "Source/Thirdparty/ImGui/imgui"),
        Path.Combine(PluginDirectory, "Source/Thirdparty/ImGui/implot"),
      }
		);

    PrivateIncludePaths.AddRange(
			new string[] {
        Path.Combine(ModuleDirectory, "Private"),
      }
    );
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
        "CoreUObject",
        "Engine",
        "RenderCore",
        "InputCore",
        "ApplicationCore",
        "RHI",
        "Slate",
        "SlateCore",
      }
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
      }
    );

    if (Target.bBuildEditor)
    {
      PrivateDependencyModuleNames.Add("UnrealEd");
    }

    DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
      }
    );

    if (Target.Configuration == UnrealTargetConfiguration.Shipping || Target.Configuration == UnrealTargetConfiguration.Test)
    {
      PublicAdditionalLibraries.Add(
        Path.Combine(PluginDirectory, "Binaries/Win64/ImGui.lib")
      );
    }
    else
    {
      PublicAdditionalLibraries.Add(
        Path.Combine(PluginDirectory, "Binaries/Win64/ImGui_Debug.lib")
      );
    }

    // EXTRA STUFF
    PublicDefinitions.Add("WITH_IMGUI=1");

    /* 
    PublicDefinitions.Add("STATS=1");
    PublicDefinitions.Add("ALLOW_LOG_FILE=1");
    PublicDefinitions.Add("ALLOW_CONSOLE_IN_SHIPPING=1");
    PublicDefinitions.Add("ALLOW_DUMPGPU_IN_SHIPPING=1");
    PublicDefinitions.Add("UE_EXTERNAL_PROFILING_ENABLED=1");
    PublicDefinitions.Add("ALLOW_WINDOWS_ERROR_REPORT_LIB=0");
    PublicDefinitions.Add("NOINITCRASHREPORTER=1");

    // PublicDefinitions.Add("MEMPRO_ENABLED=1");
    // To enable MemPro (note: MemPro.cpp /.h need to be added to the project for this to work)
    */

    bLegacyPublicIncludePaths = false;
    //ShadowVariableWarningLevel = WarningLevel.Error;
    bTreatAsEngineModule = true;
    PrivateDefinitions.Add("RUNTIME_LOADER_ENABLED=1");
  }
}
