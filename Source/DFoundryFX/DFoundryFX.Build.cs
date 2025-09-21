using UnrealBuildTool;
using System.IO;

#pragma warning disable IDE0079 // Remove unnecessary suppression
#pragma warning disable CA1050  // Types should be in namespaces

public class DFoundryFX : ModuleRules {
  public DFoundryFX(ReadOnlyTargetRules Target) : base(Target) {
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

    PublicDependencyModuleNames.AddRange([
      "Core",
      "CoreUObject",
      "Engine",
      "RHI",
      "RenderCore",
      "Renderer",
      "InputCore",
      "ApplicationCore",
      "Slate",
      "SlateCore",
    ]);

    if (Target.bBuildEditor) {
      PrivateDependencyModuleNames.Add("UnrealEd");
    }

    // ImGui definitions
    PrivateDefinitions.Add("IMGUI_DEFINE_MATH_OPERATORS=1");
    PrivateDefinitions.Add("IMGUI_DISABLE_DEMO_WINDOWS=1");
  }
}

#pragma warning restore CA1050
#pragma warning restore IDE0079