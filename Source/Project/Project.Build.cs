// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Project : ModuleRules
{
    public Project(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            // Core
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",

            // Input
            "EnhancedInput",

            // Gameplay systems
            "GameplayTags",

            // UI
            "UMG",
            "Slate",
            "SlateCore",

            // AI and navigation
            "AIModule",
            "NavigationSystem",

            // Physics
            "PhysicsCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "Project"
            }
        );

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your
        // uproject file with the Enabled attribute set to true.
    }
}
