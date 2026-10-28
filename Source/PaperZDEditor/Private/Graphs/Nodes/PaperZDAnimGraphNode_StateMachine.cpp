// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "Graphs/Nodes/PaperZDAnimGraphNode_StateMachine.h"
#include "Graphs/Nodes/PaperZDAnimGraphNode_Sink.h"
#include "Graphs/Nodes/PaperZDStateGraphNode_Root.h"
#include "Graphs/Nodes/PaperZDStateGraphNode_Jump.h"
#include "Graphs/Nodes/PaperZDStateGraphNode_Transition.h"
#include "Graphs/Nodes/PaperZDStateGraphNode_Conduit.h"
#include "Graphs/Nodes/PaperZDStateGraphNode_State.h"
#include "Graphs/Nodes/PaperZDTransitionGraphNode_Result.h"
#include "Graphs/PaperZDStateMachineGraph.h"
#include "Graphs/PaperZDStateMachineSchema.h"
#include "Graphs/PaperZDAnimGraph.h"
#include "Graphs/PaperZDAnimGraphSchema_Transitional.h"
#include "Compilers/Access/PaperZDAnimBPCompilerAccess.h"
#include "Compilers/Access/PaperZDAnimBPGeneratedClassAccess.h"
#include "Compilers/Handles/PaperZDAnimBPCompilerHandle_StateMachine.h"
#include "Editors/Util/PaperZDEditorUtils.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimGraphNode_StateMachine)
#endif

/////////////////////////////////////////////////////
// Node Name validator

class FPaperZDAnimStateMachineNodeNameValidator : public FStringSetNameValidator
{
public:
	FPaperZDAnimStateMachineNodeNameValidator(const UPaperZDAnimGraphNode_StateMachine* InStateMachineNode)
		: FStringSetNameValidator(FString())
	{
		UPaperZDAnimGraph* AnimGraph = CastChecked<UPaperZDAnimGraph>(InStateMachineNode->GetOuter());
		TArray<UPaperZDAnimGraphNode_StateMachine*> Nodes;
		AnimGraph->GetNodesOfClassEx<UPaperZDAnimGraphNode_StateMachine, UPaperZDAnimGraphNode_StateMachine>(Nodes);

		//Validate that the name doesn't already exist
		for (auto Node : Nodes)
		{
			if (Node != InStateMachineNode)
			{
				Names.Add(Node->GetStateMachineName());
			}
		}
	}
};

/////////////////////////////////////////////////////
// State machine implementation

#define LOCTEXT_NAMESPACE "PaperZD_AnimGraphNodes"

UPaperZDAnimGraphNode_StateMachine::UPaperZDAnimGraphNode_StateMachine()
	: Super()
{
	bCanRenameNode = true;
}

FText UPaperZDAnimGraphNode_StateMachine::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((TitleType == ENodeTitleType::MenuTitle || TitleType == ENodeTitleType::ListView) && (StateMachineGraph == nullptr))
	{
		return LOCTEXT("AddNewStateMachine", "Add New State Machine...");
	}
	else if(StateMachineGraph == nullptr)
	{
		return LOCTEXT("ErrorNoGraph", "Error: No Graph");
	}
	else if (TitleType == ENodeTitleType::FullTitle)
	{
		if (CachedFullTitle.IsOutOfDate(this))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Title"), FText::FromName(StateMachineGraph->GetFName()));
			// FText::Format() is slow, so we cache this to save on performance
			CachedFullTitle.SetCachedText(FText::Format(LOCTEXT("StateMachineFullTitle", "{Title}\nState Machine"), Args), this);
		}
		return CachedFullTitle;
	}

	return FText::FromName(StateMachineGraph->GetFName());
}

FLinearColor UPaperZDAnimGraphNode_StateMachine::GetNodeTitleColor() const
{
	return FLinearColor(0.8f, 0.8f, 0.8f);
}

FText UPaperZDAnimGraphNode_StateMachine::GetTooltipText() const
{
	return LOCTEXT("StateMachineTooltip", "Animation State Machine");
}

void UPaperZDAnimGraphNode_StateMachine::PostPlacedNewNode()
{
	// Create a new animation graph
	check(StateMachineGraph == NULL);
	StateMachineGraph = CastChecked<UPaperZDStateMachineGraph>(FBlueprintEditorUtils::CreateNewGraph(this, NAME_None, UPaperZDStateMachineGraph::StaticClass(), UPaperZDStateMachineSchema::StaticClass()));
	check(StateMachineGraph);
	StateMachineGraph->OwnerAnimGraphNode = this;

	//Search for a valid name for it
	TSharedPtr<INameValidatorInterface> NameValidator = FNameValidatorFactory::MakeValidator(this);
	FBlueprintEditorUtils::RenameGraphWithSuggestion(StateMachineGraph, NameValidator, TEXT("New State Machine"));

	// Initialize the anim graph
	const UEdGraphSchema* Schema = StateMachineGraph->GetSchema();
	Schema->CreateDefaultNodesForGraph(*StateMachineGraph);

	//Add to the parent graph
	UEdGraph* ParentGraph = GetGraph();
	if (ParentGraph->SubGraphs.Find(StateMachineGraph) == INDEX_NONE)
	{
		ParentGraph->Modify();
		ParentGraph->SubGraphs.Add(StateMachineGraph);
	}
}

UObject* UPaperZDAnimGraphNode_StateMachine::GetJumpTargetForDoubleClick() const
{
	return StateMachineGraph;
}

void UPaperZDAnimGraphNode_StateMachine::JumpToDefinition() const
{
	if (UObject* HyperlinkTarget = GetJumpTargetForDoubleClick())
	{
		FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(HyperlinkTarget);
	}
}

void UPaperZDAnimGraphNode_StateMachine::DestroyNode()
{
	UEdGraph* GraphToRemove = StateMachineGraph;
	StateMachineGraph = NULL;
	Super::DestroyNode();

	//Remove the graph
	if (GraphToRemove)
	{
		UBlueprint* Blueprint = GetBlueprint();
		GraphToRemove->Modify();
		FPaperZDEditorUtils::RemoveGraph(Blueprint, GraphToRemove);
	}
}

void UPaperZDAnimGraphNode_StateMachine::PostPasteNode()
{
	Super::PostPasteNode();

	// Add the new graph as a child of our parent graph
	UEdGraph* ParentGraph = GetGraph();
	if (ParentGraph->SubGraphs.Find(StateMachineGraph) == INDEX_NONE)
	{
		ParentGraph->SubGraphs.Add(StateMachineGraph);
	}

	for (UEdGraphNode* GraphNode : StateMachineGraph->Nodes)
	{
		GraphNode->CreateNewGuid();
		GraphNode->PostPasteNode();
		GraphNode->ReconstructNode();
	}

	// Find an interesting name
	TSharedPtr<INameValidatorInterface> NameValidator = FNameValidatorFactory::MakeValidator(this);
	FBlueprintEditorUtils::RenameGraphWithSuggestion(StateMachineGraph, NameValidator, StateMachineGraph->GetName());

	//restore transactional flag that is lost during copy/paste process
	StateMachineGraph->SetFlags(RF_Transactional);
}

TSharedPtr<class INameValidatorInterface> UPaperZDAnimGraphNode_StateMachine::MakeNameValidator() const
{
	return MakeShareable(new FPaperZDAnimStateMachineNodeNameValidator(this));
}

void UPaperZDAnimGraphNode_StateMachine::OnRenameNode(const FString& NewName)
{
	FBlueprintEditorUtils::RenameGraph(StateMachineGraph, NewName);
}

bool UPaperZDAnimGraphNode_StateMachine::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const
{
	return !DesiredSchema->GetClass()->IsChildOf(UPaperZDAnimGraphSchema_Transitional::StaticClass()) && Super::CanCreateUnderSpecifiedSchema(DesiredSchema);
}

FString UPaperZDAnimGraphNode_StateMachine::GetNodeCategory() const
{
	return TEXT("State Machine");
}

void UPaperZDAnimGraphNode_StateMachine::OnProcessDuringCompilation(FPaperZDAnimBPCompilerAccess& InCompilationContext, FPaperZDAnimBPGeneratedClassAccess& OutCompiledData)
{
	//Bake the state machine 
	FPaperZDAnimBPCompilerHandle_StateMachine* Handle = InCompilationContext.GetHandle<FPaperZDAnimBPCompilerHandle_StateMachine>();
	const int32 StateMachineIndex = OutCompiledData.GetStateMachines().AddDefaulted();
	AnimNode.StateMachineIndex = StateMachineIndex;

	//We need to store the state machine name inside, so JumpNodes can selectively work by comparing the machine name
	//We never store the StateMachine variable anywhere, as it can become invalid due to internal state machine nodes modifying the containing array (by adding more machines to it)
	{
		FPaperZDAnimStateMachine& StateMachine = OutCompiledData.GetStateMachines()[StateMachineIndex];
		StateMachine.MachineName = *GetStateMachineName();
	}

	//Iterate over all the nodes
	UPaperZDStateGraphNode_Root* RootNode = nullptr;
	TArray<UPaperZDStateGraphNode_Transition*> Transitions;
	TArray<UPaperZDStateGraphNode_Jump*> JumpNodes;
	TMap<UPaperZDStateGraphNode*, int32> GraphNodeToStateMachineNodeId;

	//Do a first pass, processing State and Conduit nodes, and storing the other nodes for a second pass
	TArray<UPaperZDStateGraphNode*> StateMachineNodes;
	StateMachineGraph->GetNodesOfClass(StateMachineNodes);
	for (UPaperZDStateGraphNode* Node : StateMachineNodes)
	{
		if (Node->IsA(UPaperZDStateGraphNode_Root::StaticClass()))
		{
			if (RootNode == nullptr)
			{
				RootNode = Cast<UPaperZDStateGraphNode_Root>(Node);
			}
			else
			{
				InCompilationContext.GetMessageLog().Error(*LOCTEXT("DuplicatedRoot", "@@ has more than one root node.").ToString(), StateMachineGraph);
			}
		}
		else if (Node->IsA(UPaperZDStateGraphNode_Jump::StaticClass()))
		{
			JumpNodes.Add(Cast<UPaperZDStateGraphNode_Jump>(Node));
		}
		else if (Node->IsA(UPaperZDStateGraphNode_Transition::StaticClass()))
		{
			Transitions.Add(Cast<UPaperZDStateGraphNode_Transition>(Node));
		}
		else //Here, we only have either a state or a conduit
		{
			//Be mindful, this variable cannot be used after the "ProcessAnimationGraph" because it could have been invalidated due to array manipulation
			FPaperZDAnimStateMachine& StateMachine = OutCompiledData.GetStateMachines()[StateMachineIndex];

			//Create a node, which can either be a conduit or a state and store it for later use
			const int32 BakedNodeIndex = StateMachine.Nodes.AddDefaulted();
			FPaperZDAnimStateMachineNode& BakedNode = StateMachine.Nodes[BakedNodeIndex];
			GraphNodeToStateMachineNodeId.Add(Node, BakedNodeIndex);
			check(Node->GetBoundGraph());

			//Setup the node
			if (UPaperZDStateGraphNode_State* StateNode = Cast<UPaperZDStateGraphNode_State>(Node))
			{
				TArray<UPaperZDAnimGraphNode_Sink*> SinkNodes;
				Node->GetBoundGraph()->GetNodesOfClass(SinkNodes);
				if (SinkNodes.Num() > 0)
				{
					BakedNode.AnimNodeIndex = Handle->ProcessAnimationGraph(Node->GetBoundGraph(), SinkNodes[0], InCompilationContext, OutCompiledData);
					BakedNode.bConduit = false;
				}
				else
				{
					InCompilationContext.GetMessageLog().Error(*LOCTEXT("NoSinkNode", "@@ has no sink node associated to it.").ToString(), Node->GetBoundGraph());
				}

				//Pass the names of the optional custom events to call when the state relevancy changes
				BakedNode.OnStateEnterEventName = StateNode->OnStateEnterEventName;
				BakedNode.OnStateExitEventName = StateNode->OnStateExitEventName;
			}
			else if (UPaperZDStateGraphNode_Conduit* ConduitNode = Cast<UPaperZDStateGraphNode_Conduit>(Node))
			{
				//Bound graph should have a result node at least
				TArray<UPaperZDTransitionGraphNode_Result*> ResultNodes;
				ConduitNode->GetBoundGraph()->GetNodesOfClass(ResultNodes);
				if (ResultNodes.Num() > 0)
				{
					//Create a transition rule that will govern this transition
					int32 RuleIdx = StateMachine.TransitionRules.AddDefaulted();
					FPaperZDAnimStateMachineTransitionRule& TransitionRule = StateMachine.TransitionRules[RuleIdx];
					Handle->ProcessTransitionGraph(ConduitNode->GetBoundGraph(), TransitionRule, InCompilationContext, OutCompiledData);

					//Setup the conduit
					BakedNode.bConduit = true;
					BakedNode.ConduitRuleIndex = RuleIdx;
				}
				else
				{
					InCompilationContext.GetMessageLog().Error(*LOCTEXT("ConduitNoResultNode", "@@ has no result node associated to it.").ToString(), ConduitNode->GetBoundGraph());
				}
			}
		}

	}

	//~~ Second pass, setup transitions
	//We want the transitions to be already ordered on the baked nodes, so we have a chance to do it here
	Transitions.Sort([](const UPaperZDStateGraphNode_Transition& LHS, const UPaperZDStateGraphNode_Transition& RHS) -> bool
		{
			return LHS.Priority > RHS.Priority;
		});

	for (const UPaperZDStateGraphNode_Transition* GraphTransition : Transitions)
	{
		const UPaperZDStateGraphNode* FromNode = GraphTransition->GetFromNode();
		const UPaperZDStateGraphNode* ToNode = GraphTransition->GetToNode();
		check(GraphTransition->GetBoundGraph());
		check(FromNode && GraphNodeToStateMachineNodeId.Contains(FromNode) && ToNode && GraphNodeToStateMachineNodeId.Contains(ToNode));

		//Bound graph should have a result node at least
		TArray<UPaperZDTransitionGraphNode_Result*> ResultNodes;
		GraphTransition->GetBoundGraph()->GetNodesOfClass(ResultNodes);
		if (ResultNodes.Num() > 0)
		{
			//Create a transition rule that will govern this transition
			FPaperZDAnimStateMachine& StateMachine = OutCompiledData.GetStateMachines()[StateMachineIndex];
			int32 RuleIdx = StateMachine.TransitionRules.AddDefaulted();
			FPaperZDAnimStateMachineTransitionRule& TransitionRule = StateMachine.TransitionRules[RuleIdx];
			Handle->ProcessTransitionGraph(GraphTransition->GetBoundGraph(), TransitionRule, InCompilationContext, OutCompiledData);

			//Finally add the transition rule to the node
			FPaperZDAnimStateMachineNode& BakedFromNode = StateMachine.Nodes[GraphNodeToStateMachineNodeId[FromNode]];
			FPaperZDAnimStateMachineLink& Link = BakedFromNode.OutwardLinks.AddDefaulted_GetRef();
			Link.TargetNodeIndex = GraphNodeToStateMachineNodeId[ToNode];
			Link.TransitionRuleIndex = RuleIdx;

			//~~~~ Transitional AnimGraphs ~~~~
			if (GraphTransition->HasTransitionalAnimations())
			{
				TArray<UPaperZDAnimGraphNode_Sink*> SinkNodes;
				GraphTransition->GetAnimTransitionalGraph()->GetNodesOfClass(SinkNodes);
				if (SinkNodes.Num() > 0)
				{
					Link.TransitionalAnimNodeIndex = Handle->ProcessAnimationGraph(GraphTransition->GetAnimTransitionalGraph(), SinkNodes[0], InCompilationContext, OutCompiledData);
				}
				else
				{
					InCompilationContext.GetMessageLog().Error(*LOCTEXT("NoSinkNode", "@@ has no sink node associated to it.").ToString(), GraphTransition->GetAnimTransitionalGraph());
				}
			}
		}
		else
		{
			InCompilationContext.GetMessageLog().Error(*LOCTEXT("TransitionNoResultNode", "@@ has no result node associated to it.").ToString(), GraphTransition->GetBoundGraph());
		}
	}

	//~~~ Third pass, setup jumps
	for (const UPaperZDStateGraphNode_Jump* JumpLink : JumpNodes)
	{
		UEdGraphPin* OutputPin = JumpLink->GetOutputPin();
		if (OutputPin->LinkedTo.Num() > 0)
		{
			//Conduits are valid jump targets too, they get resolved to a state when the jump is taken
			UPaperZDStateGraphNode* JumpTarget = Cast<UPaperZDStateGraphNode>(OutputPin->LinkedTo[0]->GetOwningNode());
			if (JumpTarget && (JumpTarget->IsA<UPaperZDStateGraphNode_State>() || JumpTarget->IsA<UPaperZDStateGraphNode_Conduit>()))
			{
				int32* pNodeIdx = GraphNodeToStateMachineNodeId.Find(JumpTarget);
				check(pNodeIdx);

				FPaperZDAnimStateMachine& StateMachine = OutCompiledData.GetStateMachines()[StateMachineIndex];
				StateMachine.JumpLinks.Add(*JumpLink->GetNodeName(), *pNodeIdx);
			}
		}
	}

	//~~~ Setup the root
	if (RootNode)
	{
		UEdGraphPin* OutputPin = RootNode->GetOutputPin();
		if (OutputPin->LinkedTo.Num() > 0)
		{
			const UPaperZDStateGraphNode_State* Target = Cast<UPaperZDStateGraphNode_State>(OutputPin->LinkedTo[0]->GetOwningNode());
			if (Target)
			{
				int32* pNodeIdx = GraphNodeToStateMachineNodeId.Find(Target);
				check(pNodeIdx);

				FPaperZDAnimStateMachine& StateMachine = OutCompiledData.GetStateMachines()[StateMachineIndex];
				StateMachine.InitialState = *pNodeIdx;
			}
		}
	}
	else
	{
		InCompilationContext.GetMessageLog().Error(*LOCTEXT("NoRootNode", "FATAL: @@ has no sink node associated to it.").ToString(), this);
	}
}

FString UPaperZDAnimGraphNode_StateMachine::GetStateMachineName() const
{
	return StateMachineGraph ? *StateMachineGraph->GetName() : TEXT("(null)");
}

#undef LOCTEXT_NAMESPACE
