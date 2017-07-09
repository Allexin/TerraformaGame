// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TerraformaLandscapeLoaderImporter : ModuleRules
{
	public TerraformaLandscapeLoaderImporter(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"SlateCore",
                "Slate",
				"TerraformaLandscapeLoader",
				"UnrealEd"
			});

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"AssetRegistry"
			});

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"AssetRegistry"
			});

		Definitions.AddRange(
			new string[] {
					"BUILDING_STATIC"
				}
		);

		// Link with managed Perforce wrapper assemblies
		AddEngineThirdPartyPrivateStaticDependencies(Target, "coremod");
	
	}
}
