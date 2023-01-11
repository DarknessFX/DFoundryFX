using System.IO;
using UnrealBuildTool;

public class ImGui : ModuleRules
{
    public ImGui(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		PublicDefinitions.Add("WITH_IMGUI");

		PublicIncludePaths.Add(Path.Combine(PluginDirectory, "Source/Thirdparty/ImGui/imgui"));
		PublicIncludePaths.Add(Path.Combine(PluginDirectory, "Source/Thirdparty/ImGui/implot"));

		if (Target.Configuration == UnrealTargetConfiguration.Shipping || Target.Configuration == UnrealTargetConfiguration.Test)
		{
			PublicAdditionalLibraries.Add(Path.Combine(PluginDirectory, "Binaries/Win64/ImGui.lib"));
		}
		else
    {
			PublicAdditionalLibraries.Add(Path.Combine(PluginDirectory, "Binaries/Win64/ImGui_Debug.lib"));
		}
	}
}
