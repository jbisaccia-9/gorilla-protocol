using UnrealBuildTool;
using System.Collections.Generic;

public class GorillaProtocolEditorTarget : TargetRules
{
    public GorillaProtocolEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("GorillaProtocol");
    }
}
