using UnrealBuildTool;
using System.Collections.Generic;

public class GorillaProtocolTarget : TargetRules
{
    public GorillaProtocolTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("GorillaProtocol");
    }
}
