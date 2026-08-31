using UnrealBuildTool;
using System.Collections.Generic;

public class DashtSadosiTarget : TargetRules
{
    public DashtSadosiTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("DashtSadosi");
    }
}

