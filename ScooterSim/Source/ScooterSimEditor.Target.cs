using UnrealBuildTool;
using System.Collections.Generic;

public class ScooterSimEditorTarget : TargetRules
{
	public ScooterSimEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ScooterSim");
	}
}
