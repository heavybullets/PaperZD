// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

using UnrealBuildTool;
using System.IO;

public class PaperZD : ModuleRules
{
	public PaperZD(ReadOnlyTargetRules TargetRules) : base (TargetRules)
	{
		PrecompileForTargets = PrecompileTargetsType.Any;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicIncludePaths.AddRange(
			new string[] {
                Path.Combine(ModuleDirectory, "Public"),
                Path.Combine(ModuleDirectory, "Public/AnimNodes"),
				Path.Combine(ModuleDirectory, "Public/Notifies"),
				//"PaperZD/Public",
                //"PaperZD/Public/AnimNodes",
                //"PaperZD/Public/Notifies",
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                Path.Combine(ModuleDirectory, "Private"),
                Path.Combine(ModuleDirectory, "Private/AnimNodes"),
				Path.Combine(ModuleDirectory, "Private/Notifies"),
				/*"PaperZD/Private",
                "PaperZD/Private/AnimNodes",
                "PaperZD/Private/Notifies",*/

				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CoreUObject",
                "Paper2D",
				"Niagara"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
				//"Sequencer",
		        "MovieScene",
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

#if UE_5_9_OR_LATER
		// UE 5.9 moved the Cascade particle runtime (UParticleSystem, SpawnEmitterAttached) into the Cascade plugin.
		PrivateDependencyModuleNames.Add("CascadeRuntime");
#endif

#if UE_5_1_OR_LATER
		PrivateDefinitions.Add("ZD_VERSION_INLINED_CPP_SUPPORT=1");
#else
		PrivateDefinitions.Add("ZD_VERSION_INLINED_CPP_SUPPORT=0");
#endif
	}
}
