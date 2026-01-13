// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "PaperZDRuntimeEditorProxy.h"
#include "PaperZDEditorSettings.h"
#include "AssetToolsModule.h"
#include "Factories/PaperZDAnimSequenceFactory.h"
#include "Factories/PaperZDAnimationSourceFactory.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"

#include "AnimSequences/PaperZDAnimSequence_Flipbook.h"
#include "AnimSequences/Sources/PaperZDAnimationSource_Flipbook.h"
#include "Notifies/PaperZDAnimNotify_Base.h"
#include "Graphs/PaperZDAnimGraphSchema.h"
#include "Graphs/PaperZDAnimGraph.h"
#include "Graphs/PaperZDNotifyGraph.h"
#include "Graphs/PaperZDNotifyGraphSchema.h"
#include "Graphs/PaperZDStateMachineGraph.h"
#include "Graphs/Nodes/PaperZDAnimGraphNode_StateMachine.h"
#include "Graphs/Nodes/PaperZDAnimGraphNode_Sink.h"
#include "Graphs/Nodes/PaperZDAnimGraphNode_PlaySequence.h"
#include "AnimNodes/PaperZDAnimNode_PlaySequence.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "K2Node_FunctionEntry.h"

//Deprecated, but still required for version support
#include "Graphs/Nodes/PaperZDStateGraphNode_State.h"
#include "Graphs/Nodes/PaperZDStateGraphNode_Transition.h"

void FPaperZDRuntimeEditorProxy::Register()
{
	if (!IPaperZDEditorProxy::Get().IsValid())
	{
		IPaperZDEditorProxy::SetEditorProxy(TSharedPtr<IPaperZDEditorProxy>(new FPaperZDRuntimeEditorProxy));
	}
}

UEdGraph* FPaperZDRuntimeEditorProxy::CreateNewAnimationGraph(UPaperZDAnimBP* InAnimBP)
{
	UPaperZDAnimGraph* AnimGraph = CastChecked<UPaperZDAnimGraph>(FBlueprintEditorUtils::CreateNewGraph(InAnimBP, UPaperZDAnimGraphSchema::GN_AnimGraph, UPaperZDAnimGraph::StaticClass(), UPaperZDAnimGraphSchema::StaticClass()));
	return Cast<UEdGraph>(AnimGraph);
}

TArray<FAssetData> FPaperZDRuntimeEditorProxy::GetAnimSequencesForSource(UPaperZDAnimationSource* InAnimSource)
{
	//Setup filter
	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.ClassPaths.Add(UPaperZDAnimSequence::StaticClass()->GetClassPathName());
	Filter.TagsAndValues.Add(UPaperZDAnimSequence::GetAnimSourceMemberName(), FAssetData(InAnimSource).GetExportTextName());

	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> OutData;
	AssetRegistryModule.Get().GetAssets(Filter, OutData);
	return OutData;
}

void FPaperZDRuntimeEditorProxy::UpdateNotifyFunctions(UPaperZDAnimationSource* InAnimSource)
{
	//Setup filter
	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.ClassPaths.Add(UPaperZDAnimBP::StaticClass()->GetClassPathName());
	Filter.TagsAndValues.Add(UPaperZDAnimBP::GetAnimationSourceMemberName(), FAssetData(InAnimSource).GetExportTextName());

	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	
	TArray<FAssetData> BoundAnimBPs;
	AssetRegistryModule.Get().GetAssets(Filter, BoundAnimBPs);
	for (const FAssetData& AssetData : BoundAnimBPs)
	{
		UPaperZDAnimBP* AnimBP = Cast<UPaperZDAnimBP>(AssetData.GetAsset());
		UpdateNotifyFunctions(AnimBP);
	}
}

void FPaperZDRuntimeEditorProxy::UpdateNotifyFunctions(UPaperZDAnimBP* AnimBP)
{
	//If for some reason this AnimBP doesn't have an animation source connected to it, bail-out.
	if (AnimBP->GetSupportedAnimationSource() == nullptr)
	{
		return;
	}

	//Mark the AnimBP to be modified, in case we're inside a transaction
	AnimBP->Modify(false);

	//We will create all the AnimNotify functions that don't exist, and remove any that need to be removed.
	TSet<FName> ProcessedNotifies;
	for (int32 i = AnimBP->FunctionGraphs.Num() - 1; i >= 0; i--)
	{
		UPaperZDNotifyGraph* AnimNotifyGraph = Cast<UPaperZDNotifyGraph>(AnimBP->FunctionGraphs[i]);
		if (AnimNotifyGraph)
		{
			//To allow for UNDO
			AnimNotifyGraph->Modify(false);

			if (AnimBP->GetSupportedAnimationSource()->GetRegisteredNotifyNames().Find(AnimNotifyGraph->NotifyName) != INDEX_NONE)
			{
				//Notify still exists, pass the actual function name to the generated class
				TArray<UK2Node_FunctionEntry*> EntryPoints;
				AnimNotifyGraph->GetNodesOfClass(EntryPoints);
				check(EntryPoints.Num() == 1);

				//Notify is valid, mark as such
				AnimNotifyGraph->bRegistered = true;
				AnimNotifyGraph->bAllowDeletion = false;

				//Already processed, mark as such
				ProcessedNotifies.Add(AnimNotifyGraph->NotifyName);
			}
			else
			{
				//Either automatically purge the graph, or only purge it if its empty
				const bool bDeleteAutomatically = GetDefault<UPaperZDEditorSettings>()->bAutomaticallyPurgeUnregisteredNotifyFunctions;
				if (bDeleteAutomatically || (AnimNotifyGraph->Nodes.Num() <= 1))
				{
					AnimBP->FunctionGraphs.RemoveAt(i);
				}
				else
				{
					AnimNotifyGraph->bRegistered = false;
					AnimNotifyGraph->bAllowDeletion = true;
				}
			}
		}
	}

	//Create any function graph that needs to be generated
	for (FName NotifyName : AnimBP->GetSupportedAnimationSource()->GetRegisteredNotifyNames())
	{
		if (!ProcessedNotifies.Contains(NotifyName))
		{
			//We need to create a custom function for this graph
			UPaperZDNotifyGraph* NewNotifyGraph = CastChecked<UPaperZDNotifyGraph>(FBlueprintEditorUtils::CreateNewGraph(AnimBP, NotifyName, UPaperZDNotifyGraph::StaticClass(), UPaperZDNotifyGraphSchema::StaticClass()));
			NewNotifyGraph->bAllowDeletion = false;
			NewNotifyGraph->bAllowRenaming = false;
			NewNotifyGraph->NotifyName = NotifyName;
			NewNotifyGraph->bRegistered = true;

			//Create an entry point for the function
			const FString ProposedNotifyName = FString(TEXT("ReceiveNotify_")).Append(NotifyName.ToString());
			FGraphNodeCreator<UK2Node_FunctionEntry> NodeCreator(*NewNotifyGraph);
			UK2Node_FunctionEntry* FunctionEntry = NodeCreator.CreateNode(false);
			FunctionEntry->CustomGeneratedFunctionName = FBlueprintEditorUtils::FindUniqueKismetName(AnimBP, ProposedNotifyName);
			FunctionEntry->AllocateDefaultPins();
			NodeCreator.Finalize();

			//Add to the AnimBP (so it appears as part of the function list), and register to the generated class
			AnimBP->FunctionGraphs.Add(NewNotifyGraph);
		}
	}

	//Make sure the system knows it should rebuild
	FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
	AnimBP->GetOutermost()->SetDirtyFlag(true);
}

void FPaperZDRuntimeEditorProxy::RenameCustomNotifyGraph(UPaperZDAnimBP* InAnimBP, FName OldName, FName NewName)
{
	//Iterate on the function graphs, trying to find the custom notify
	for (UEdGraph* Graph : InAnimBP->FunctionGraphs)
	{
		UPaperZDNotifyGraph* NotifyGraph = Cast<UPaperZDNotifyGraph>(Graph);
		if (NotifyGraph && NotifyGraph->NotifyName == OldName)
		{
			//Rename and mark as modified
			NotifyGraph->Modify();
			NotifyGraph->NotifyName = NewName;
			FBlueprintEditorUtils::MarkBlueprintAsModified(InAnimBP);
		}
	}
}

void FPaperZDRuntimeEditorProxy::UpdateVersionToAnimSequences(UPaperZDAnimBP *InAnimBP)
{
	//Unsupported version
	check(false);
}

void FPaperZDRuntimeEditorProxy::UpdateVersionToAnimNodeOuterFix(UPaperZDAnimBP *InAnimBP)
{
	//Unsupported version
	check(false);
}

void FPaperZDRuntimeEditorProxy::UpdateVersionToAnimSequenceCategoryAdded(UPaperZDAnimBP *InAnimBP)
{
	//Unsupported version
	check(false);
}

void FPaperZDRuntimeEditorProxy::UpdateVersionToAnimSequenceAsStandaloneAsset(UPaperZDAnimBP* InAnimBP)
{
	//Unsupported version
	check(false);
}

void FPaperZDRuntimeEditorProxy::UpdateVersionToAnimBlueprintRework(UPaperZDAnimBP* InAnimBP)
{
	UEdGraph* AnimationGraph = InAnimBP->AnimationGraph;
	const UPaperZDAnimGraphSchema* Schema = GetDefault<UPaperZDAnimGraphSchema>();

	//Need to enforce the loading flag otherwise some RENAME operations will crash due to loaders being reset, this flag will make the rename use the flag REN_ForceNoResetLoaders instead.
	const bool bGuardLoading = InAnimBP->bIsRegeneratingOnLoad;
	InAnimBP->bIsRegeneratingOnLoad = true;

	//~~~~
	//Stage 0, Setup the metadata
	AnimationGraph->Rename(*UPaperZDAnimGraphSchema::GN_AnimGraph.ToString(), nullptr, REN_ForceNoResetLoaders);

	//~~~~
	//Stage 1, create a transient EdGraph, and pass all the nodes to it.
	const bool bIsLoading = true;
	const bool bIsCompiling = InAnimBP->bBeingCompiled;
	UEdGraph* TempGraph = NewObject<UEdGraph>(GetTransientPackage(), UEdGraph::StaticClass(), NAME_None, RF_Transactional);
	AnimationGraph->MoveNodesToAnotherGraph(TempGraph, bIsLoading, bIsCompiling);
	
	//~~~~
	//Stage 2, create the minimum required nodes for the AnimGraph to work (sink + state machine).
	Schema->CreateDefaultNodesForGraph(*AnimationGraph);
	check(AnimationGraph->Nodes.Num() == 1);
	UPaperZDAnimGraphNode_Sink* SinkNode = CastChecked<UPaperZDAnimGraphNode_Sink>(AnimationGraph->Nodes[0]);

	//Create the state machine node
	FGraphNodeCreator<UPaperZDAnimGraphNode_StateMachine> NodeCreator(*AnimationGraph);
	UPaperZDAnimGraphNode_StateMachine* StateMachineNode = NodeCreator.CreateNode();
	NodeCreator.Finalize();

	const FVector2D NodePosition = AnimationGraph->GetGoodPlaceForNewNode();
	StateMachineNode->NodePosX = NodePosition.X;
	StateMachineNode->NodePosY = NodePosition.Y;

	//Need to remove the root node (which should be the only node that exist on the state machine), as the temp graph already contains it (with linkage information)
	UPaperZDStateMachineGraph* StateMachineGraph = StateMachineNode->GetStateMachineGraph();
	check(StateMachineGraph->Nodes.Num() == 1);
	StateMachineGraph->RemoveNode(StateMachineNode->GetStateMachineGraph()->Nodes[0]);
	
	//~~~~
	//Stage 3, pass all the nodes to the new state machine and wire up the connection
	TempGraph->MoveNodesToAnotherGraph(StateMachineNode->GetStateMachineGraph(), bIsLoading, bIsCompiling);
	UEdGraphPin* StateMachineOutPin = StateMachineNode->Pins[0];
	UEdGraphPin* SinkInputPin = SinkNode->Pins[0];
	check(StateMachineOutPin && SinkInputPin);
	StateMachineOutPin->MakeLinkTo(SinkInputPin);

	//Stage 4, move all the sub-graphs from the AnimGraph onto the StateMachine and create new AnimGraphs as needed
	for (UEdGraphNode* Node : StateMachineGraph->Nodes)
	{
		if (UPaperZDStateGraphNode_State* StateNode = Cast<UPaperZDStateGraphNode_State>(Node))
		{
			//Need to trigger the post creation method call, that way it will configure and create its graphs as expected
			StateNode->PostPlacedNewNode();
			check(StateNode->GetBoundGraph());

			//The name of the state is no longer held on a variable inside the state node, but instead is on the graph itself, rename the graph to maintain the old name
			TSharedPtr<INameValidatorInterface> NameValidator = FNameValidatorFactory::MakeValidator(StateNode);
			FBlueprintEditorUtils::RenameGraphWithSuggestion(StateNode->GetBoundGraph(), NameValidator, StateNode->Name_DEPRECATED.ToString());

			//With the newly created AnimGraph, set the sequence
			if (StateNode->AnimSequence_DEPRECATED)
			{
				FGraphNodeCreator<UPaperZDAnimGraphNode_PlaySequence> GraphNodeCreator(*StateNode->GetBoundGraph());
				UPaperZDAnimGraphNode_PlaySequence* PlaySequenceNode = GraphNodeCreator.CreateNode();
				PlaySequenceNode->AnimNode.AnimSequence = StateNode->AnimSequence_DEPRECATED;
				PlaySequenceNode->AnimNode.bLoopAnimation = StateNode->bShouldLoop_DEPRECATED;
				GraphNodeCreator.Finalize();

				const FVector2D NewPosition = StateNode->GetBoundGraph()->GetGoodPlaceForNewNode();
				PlaySequenceNode->NodePosX = NewPosition.X;
				PlaySequenceNode->NodePosY = NewPosition.Y;

				//Wire up
				UPaperZDAnimGraphNode_Sink* BoundGraphSinkNode = CastChecked<UPaperZDAnimGraphNode_Sink>(StateNode->GetBoundGraph()->Nodes[0]);
				UEdGraphPin* OutputPin = PlaySequenceNode->Pins[0];
				UEdGraphPin* InputPin = BoundGraphSinkNode->Pins[0];
				check(OutputPin && InputPin);
				OutputPin->MakeLinkTo(InputPin);
			}
		}
		else if (UPaperZDStateGraphNode_TransBase* TransNode = Cast<UPaperZDStateGraphNode_TransBase>(Node))
		{
			//Move the transition graph from the AnimGraph onto the state machine graph
			AnimationGraph->SubGraphs.Remove(TransNode->GetBoundGraph());
			StateMachineGraph->SubGraphs.Add(TransNode->GetBoundGraph());

			//Before this version, the names of the transition graphs weren't enforced to be unique, it now needs to be
			//We will use the exact same code for naming as the transition nodes do now
			TSharedPtr<INameValidatorInterface> NameValidator = FNameValidatorFactory::MakeValidator(TransNode);
			FBlueprintEditorUtils::RenameGraphWithSuggestion(TransNode->GetBoundGraph(), NameValidator, TransNode->GetDesiredNewNodeName());

			//Optionally, if this is a transition node, we could have a "transitional animation" in place, which now should be tackled by an internal AnimGraph
			UPaperZDStateGraphNode_Transition* TransitionNode = Cast<UPaperZDStateGraphNode_Transition>(Node);
			if (TransitionNode && TransitionNode->AnimSequence_DEPRECATED != nullptr)
			{
				TransitionNode->bPlayTransitionalAnimations = true;
				TransitionNode->CreateAnimTransitionalGraph();
				
				//Create a PLAY node with the sequence inside
				FGraphNodeCreator<UPaperZDAnimGraphNode_PlaySequence> GraphNodeCreator(*TransitionNode->GetAnimTransitionalGraph());
				UPaperZDAnimGraphNode_PlaySequence* PlaySequenceNode = GraphNodeCreator.CreateNode();
				PlaySequenceNode->AnimNode.AnimSequence = TransitionNode->AnimSequence_DEPRECATED;
				PlaySequenceNode->AnimNode.bLoopAnimation = false;
				GraphNodeCreator.Finalize();

				const FVector2D NewPosition = TransitionNode->GetAnimTransitionalGraph()->GetGoodPlaceForNewNode();
				PlaySequenceNode->NodePosX = NewPosition.X;
				PlaySequenceNode->NodePosY = NewPosition.Y;

				//Wire up
				UPaperZDAnimGraphNode_Sink* BoundGraphSinkNode = CastChecked<UPaperZDAnimGraphNode_Sink>(TransitionNode->GetAnimTransitionalGraph()->Nodes[0]);
				UEdGraphPin* OutputPin = PlaySequenceNode->Pins[0];
				UEdGraphPin* InputPin = BoundGraphSinkNode->Pins[0];
				check(OutputPin && InputPin);
				OutputPin->MakeLinkTo(InputPin);
			}
		}
	}

	//Stage 5, Patch-up: 
	//By this point, the AnimationBlueprint has already tried to compile once (with no Animation Graph support due to the PostLoad of the AnimBP happening after the first compilation).
	//Because the old AnimBlueprint generated class gets "Consigned to Oblivion", the compiler generates a new class for the AnimBP which breaks some of the behavior from the K2 nodes (like Get & Set variables).
	//The easiest fix for this is to ask the nodes to refresh themselves now that the new "definitive" class has been generated.
	RefreshK2Nodes(InAnimBP);

	//Restore the loading flag
	InAnimBP->bIsRegeneratingOnLoad = bGuardLoading;
	
	//We must now request the Animation Blueprint to be compiled, as it now has support for the new compiler version.
	FBlueprintEditorUtils::MarkBlueprintAsModified(InAnimBP);
	InAnimBP->GetOutermost()->SetDirtyFlag(true);
}

void FPaperZDRuntimeEditorProxy::RefreshK2Nodes(UPaperZDAnimBP* InAnimBP)
{
	//This could be done in a more specific way, but by doing this we make sure no possible issues arise later.
	TArray<UEdGraph*> AllGraphs;
	InAnimBP->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		TArray<UK2Node*> K2Nodes;
		Graph->GetNodesOfClass(K2Nodes);

		for (UK2Node* KNode : K2Nodes)
		{
			KNode->ReconstructNode();
		}
	}
}

void FPaperZDRuntimeEditorProxy::UpdateVersionToAnimationSourceAdded(UPaperZDAnimBP* InAnimBP)
{
	//Step 0: Obtain all the animation sequences that were held by this animation blueprint (via the asset searchable property that got deprecated)
	TArray<FAssetData> AnimSequenceAssets;
	{
		FARFilter Filter;
		Filter.bRecursiveClasses = true;
		Filter.ClassPaths.Add(UPaperZDAnimSequence::StaticClass()->GetClassPathName());
		Filter.TagsAndValues.Add(TEXT("AnimBP"), FAssetData(InAnimBP).GetExportTextName());

		// Load the asset registry module
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().GetAssets(Filter, AnimSequenceAssets);
	}

	//Obtain the list of possible animation source class that currently exist
	TArray<UClass*> SourceClasses;
	for (TObjectIterator< UClass > ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;
		if (Class->IsChildOf(UPaperZDAnimationSource::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
		{
			SourceClasses.Add(Class);
		}
	}

	//Select the animation source class we will be using throughout, we default to the first compatible class that can manage the anim sequences the AnimBP had before conversion
	TSubclassOf<UPaperZDAnimationSource> AnimationSourceClass = UPaperZDAnimationSource_Flipbook::StaticClass();
	if (AnimSequenceAssets.Num())
	{
		//Grab the first one and check its supported type
		UClass* AnimSequenceClass = AnimSequenceAssets[0].GetClass();
		for (UClass* TestClass : SourceClasses)
		{
			UPaperZDAnimationSource* CDO = TestClass->GetDefaultObject<UPaperZDAnimationSource>();
			if (CDO->GetSupportedAnimSequenceClass() == AnimSequenceClass)
			{
				AnimationSourceClass = TestClass;
			}
		}
	}

	//We should never have an invalid animation source class by this point
	check(AnimationSourceClass);
	
	//Step 1. Create the animation source
	UPaperZDAnimationSource* AnimationSource = nullptr;
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const FAssetData AnimBPAssetData = FAssetData(InAnimBP);
		const FString PackagePath = AnimBPAssetData.PackagePath.ToString();
		const FString AssetName = TEXT("AnimSource_") + AnimBPAssetData.AssetName.ToString();
		
		//The AnimSource factory will manage the creation of the animation source
		UPaperZDAnimationSourceFactory* Factory = NewObject<UPaperZDAnimationSourceFactory>(GetTransientPackage());
		Factory->SelectedClass = AnimationSourceClass;
		Factory->AddToRoot();

		//Ask the asset module to create the animation source
		FString OutPackageName;
		FString OutAssetName;
		AssetToolsModule.Get().CreateUniqueAssetName(PackagePath + TEXT("/") + AssetName, TEXT(""), OutPackageName, OutAssetName);
		UObject* CreatedObject = AssetToolsModule.Get().CreateAsset(OutAssetName, PackagePath, AnimationSourceClass, Factory);
		AnimationSource = CastChecked<UPaperZDAnimationSource>(CreatedObject);
		AnimationSource->GetOutermost()->SetDirtyFlag(true);

		Factory->RemoveFromRoot();
	}

	//Step 2: Mark the AnimBP this AnimationSource and pass the data to it
	InAnimBP->SupportedAnimationSource = AnimationSource;
	InAnimBP->GetOutermost()->SetDirtyFlag(true);
	FBlueprintEditorUtils::MarkBlueprintAsModified(InAnimBP);

	//Pass the notify data to it
	AnimationSource->RegisteredNotifyNames = InAnimBP->RegisteredNotifyNames_DEPRECATED;

	//Step 3: Make sure the data gets passed to every AnimSequence
	//Due to load order, the PostLoad of AnimSequences can happen before we have a chance to create the animation source and link it
	//if that happens we just need to re-link
	for (const FAssetData& AnimSequenceData : AnimSequenceAssets)
	{
		UPaperZDAnimSequence* AnimSequence = CastChecked<UPaperZDAnimSequence>(AnimSequenceData.GetAsset());
		AnimSequence->AnimSource = AnimationSource;
		AnimSequence->GetOutermost()->SetDirtyFlag(true);
	}
}

void FPaperZDRuntimeEditorProxy::UpdateVersionToAnimationSourceAdded(UPaperZDAnimSequence* InAnimSequence)
{
	//Mostly done for safety, if by any reason the AnimBP has already been fixed, but we lack the animation source object ourselves... fixup
	//This could happen if the AnimBP got fixed and saved, but the user failed to save this AnimSequence before unloading it... by this point we can just re-link without issues and avoid any loss of data
	if (InAnimSequence->AnimBP_DEPRECATED && InAnimSequence->AnimBP_DEPRECATED->GetSupportedAnimationSource() && InAnimSequence->GetAnimSource() == nullptr)
	{
		InAnimSequence->AnimSource = InAnimSequence->AnimBP_DEPRECATED->GetSupportedAnimationSource();
	}

	//Iterate tracks and rename them to have the index of the track
	for (int32 i = 0; i < InAnimSequence->GetNumTracks(); i++)
	{
		FPaperZDAnimTrackMetadata& TrackData = InAnimSequence->GetMutableTrackMetadataChecked(i);
		TrackData.DisplayName = *FString::FormatAsNumber(i + 1);
	}

	//Force mark dirty
	InAnimSequence->GetOutermost()->SetDirtyFlag(true);
}
