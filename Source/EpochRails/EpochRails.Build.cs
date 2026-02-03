// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EpochRails : ModuleRules
{
    public EpochRails(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        { 
            // ========== Базовые модули ==========
            "Core",                     // Основные классы UE
            "CoreUObject",              // UObject система
            "Engine",                   // Игровой движок
            "InputCore",                // Базовый ввод
            
            // ========== Enhanced Input ==========
            "EnhancedInput",            // Современная система ввода
            
            // ========== UI Модули ==========
            "UMG",                      // User Widget (UI Blueprint)
            "Slate",                    // UI Framework
            "SlateCore",                // UI Core
            
            // ========== AI и Навигация ==========
            "AIModule",                 // AI система (контроллеры, поведение)
            "NavigationSystem",         // Navmesh, pathfinding
            
            // ========== Физика ==========
            "PhysicsCore"               // Физическая система
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        { 
            // Приватные зависимости (если понадобятся в будущем)
        });

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "EpochRails"
            }
        );

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
