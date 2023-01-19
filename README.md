     .----------------.  .----------------.  .----------------. 
    | .--------------. || .--------------. || .--------------. |
    | |  ________    | || |  _________   | || |  ____  ____  | |
    | | |_   ___ `.  | || | |_   ___  |  | || | |_  _||_  _| | |
    | |   | |   `. \ | || |   | |_  \_|  | || |   \ \  / /   | |
    | |   | |    | | | || |   |  _|      | || |    > `' <    | |
    | |  _| |___.' / | || |  _| |_       | || |  _/ /'`\ \_  | |
    | | |________.'  | || | |_____|      | || | |____||____| | |
    | |              | || |              | || |              | |
    | '--------------' || '--------------' || '--------------' |
     '----------------'  '----------------'  '----------------' 

           DarknessFX @ https://dfx.lv | Twitter: @DrkFX

# DFoundryFX plugin For Unreal Engine

<img src="https://github.com/DarknessFX/DFoundryFX/raw/eea015c01c242c5107f6b47a4e32e807e9e6de8d/.git_img/screenshot01.png" width="640px" /> <br/>

DFoundryFX plugin feature customizable performance metric charts (including Shipping builds), 
Shader compiler monitoring and STAT commands control panel for Unreal Engine GameViewports. 
While the plugin is enabled you always have access to the charts, controls and settings 
directly in your game viewport, this plugin don't need actors or objects to be created 
in your project.<br/>

## About

(WIP) 

## Installation

### Use precompiled plugin (Unreal Engine 5.1 only)

. Download the latest <a href="https://github.com/DarknessFX/DFoundryFX/releases" target="_blank">release</a>. <br/>
. Unzip the file in your UEFolder\Engine\Plugins. <br/>
. Start Unreal Engine. <br/>
. Menu Tools > Plugins. <br/>
. Enable in Installed > Perfomance > DFoundryFX . <br/>
. Restart Unreal Engine. <br/>
. Play your project, DFoundryFX window and charts will show up.<br/>

### Build from Example

. Visit <a href="https://github.com/DarknessFX/DFoundryFX_Example" target="_blank">DFoundryFX_Example</a> repo , download Setup.bat and execute.<br/>
. Play your project, DFoundryFX window and charts will show up.<br/>

### Build from Source
. Create a new folder.  ex: *Plugins\* <br/>
. Git clone this project with sub-modules.  ex: *git clone --recursive https://github.com/DarknessFX/DFoundryFX.git*<br/>
. Open your "**Developer Command Prompt for VisualStudio**" (*Program Files\Microsoft Visual Studio\Common7\Tools\VsDevCmd.bat*).<br/>
. Execute *Plugins\DFoundryFX\Source\ThirdParty\ImGui\Build.bat* .<br/>
. Copy Plugins\ folder to your project folder (or to your Engine\Plugins folder).<br/>
. Open your project in VisualStudio and build.<br/>
. Open your project, enabled DFoundryFX in Plugins tab (Installed > Perfomance > DFoundryFX). <br/>
. Play your project, DFoundryFX window and charts will show up.<br/>

## Compatibility with Unreal Engine 5.0

(WIP)

## Credits

Unreal Engine from Epic Games - https://www.unrealengine.com/ <br/>
DearImGui from @ocornut - https://github.com/ocornut/imgui <br/>
ImPlot from @epezent - https://github.com/epezent/implot <br/>
MetalWorking Icon from TheNounProject - https://thenounproject.com/icon/metalworking-1563039/ <br/>
ImGuiPlugin from @amuTBKT - https://github.com/amuTBKT/ImGuiPlugin <br/>
UnrealEngine_ImGui from @sronsse - https://github.com/sronsse/UnrealEngine_ImGui <br/>
Rama code multi threading from Rama - https://dev.epicgames.com/community/learning/tutorials/7Rz/rama-code-multi-threading-in-ue5-c <br/>

## License

@MIT - Free for everyone and any use. <br/><br/>
DarknessFX @ <a href="https://dfx.lv" target="_blank">https://dfx.lv</a> | Twitter: <a href="https://twitter.com/DrkFX" target="_blank">@DrkFX</a> <br/>https://github.com/DarknessFX/DFoundryFX
