// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "Graphs/PaperZDStateMachineSchema.h"
#include "Graphs/PaperZDStateMachineGraph.h"
#include "Graphs/Nodes/PaperZDStateGraphNode_Root.h"
#include "Graphs/Nodes/PaperZDStateGraphNode_Transition.h"
#include "Graphs/Nodes/PaperZDStateGraphNode_State.h"
#include "Graphs/Nodes/PaperZDStateGraphNode_Conduit.h"
#include "Graphs/Nodes/PaperZDStateGraphNode_Jump.h"
#include "Graphs/PaperZDStateMachineConnectionDrawingPolicy.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "PaperZDAnimBP.h"
#include "EdGraphNode_Comment.h"
#include "ScopedTransaction.h"
#include "GraphEditorActions.h"
#include "ToolMenus.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDStateMachineSchema)
#endif


#define LOCTEXT_NAMESPACE "PaperZDStateMachineSchema"

/////////////////////////////////////////////////////
// FPaperZDStateMachineSchemaAction_NewNode

UEdGraphNode* FPaperZDStateMachineSchemaAction_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	check(NodeClass);

	UBlueprint* AnimBP = FBlueprintEditorUtils::FindBlueprintForGraph(ParentGraph);
	const FScopedTransaction Transaction(LOCTEXT("PaperZDStateMachine_NewNode", "PaperZD AnimBP: New StateMachine Node"));
	ParentGraph->Modify();
	AnimBP->Modify();

	//We need to use FGraphNodeCreator which doesn't allow us to specify the node class by variable, only by template.
	UPaperZDStateGraphNode* NewNode = nullptr;
	if (NodeClass == UPaperZDStateGraphNode_State::StaticClass())
	{
		FGraphNodeCreator<UPaperZDStateGraphNode_State> NodeCreator(*ParentGraph);
		NewNode = NodeCreator.CreateNode(bSelectNewNode);
		NodeCreator.Finalize();
	}
	else if (NodeClass == UPaperZDStateGraphNode_Transition::StaticClass())
	{
		FGraphNodeCreator<UPaperZDStateGraphNode_Transition> NodeCreator(*ParentGraph);
		NewNode = NodeCreator.CreateNode(bSelectNewNode);
		NodeCreator.Finalize();
	}
	else if (NodeClass == UPaperZDStateGraphNode_Conduit::StaticClass())
	{
		FGraphNodeCreator<UPaperZDStateGraphNode_Conduit> NodeCreator(*ParentGraph);
		NewNode = NodeCreator.CreateNode(bSelectNewNode);
		NodeCreator.Finalize();
	}
	else if (NodeClass == UPaperZDStateGraphNode_Jump::StaticClass())
	{
		FGraphNodeCreator<UPaperZDStateGraphNode_Jump> NodeCreator(*ParentGraph);
		NewNode = NodeCreator.CreateNode(bSelectNewNode);
		NodeCreator.Finalize();
	}

	//Should have a node created by now
	check(NewNode);

	//Setup the location
	NewNode->NodePosX = Location.X;
	NewNode->NodePosY = Location.Y;

	//Create the autowire
	NewNode->AutowireNewNode(FromPin);

	//Mark AnimBP as modified
	AnimBP->PostEditChange();
	AnimBP->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);

	return NewNode;
}

/////////////////////////////////////////////////////
// FPaperZDStateMachineSchemaAction_NewComment

UEdGraphNode* FPaperZDStateMachineSchemaAction_NewComment::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	// Add menu item for creating comment boxes
	UEdGraphNode_Comment* CommentTemplate = NewObject<UEdGraphNode_Comment>();
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(ParentGraph);

	FVector2D SpawnLocation = Location;
	FSlateRect Bounds;
	if ((Blueprint != nullptr) && FKismetEditorUtilities::GetBoundsForSelectedNodes(Blueprint, Bounds, 50.0f))
	{
		CommentTemplate->SetBounds(Bounds);
		SpawnLocation.X = CommentTemplate->NodePosX;
		SpawnLocation.Y = CommentTemplate->NodePosY;
	}

	UEdGraphNode* NewNode = FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UEdGraphNode_Comment>(ParentGraph, CommentTemplate, SpawnLocation, bSelectNewNode);

	// Update Analytics for these nodes
	FBlueprintEditorUtils::AnalyticsTrackNewNode(NewNode);

	// Mark Blueprint as structurally modified since
	// UK2Node_Comment::NodeCausesStructuralBlueprintChange used to return true
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	return NewNode;
}

//////////////////////////////////////////////////////////////////////////
//// PaperZD State Machine Schema
//////////////////////////////////////////////////////////////////////////
UPaperZDStateMachineSchema::UPaperZDStateMachineSchema(const FObjectInitializer& ObjectInitializer)
	: Super()
{
}

FConnectionDrawingPolicy* UPaperZDStateMachineSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const
{
	return new FPaperZDStateMachineConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

void UPaperZDStateMachineSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	//Create default node
 	FGraphNodeCreator<UPaperZDStateGraphNode_Root> NodeCreator(Graph);
 	UPaperZDStateGraphNode_Root* RootNode = NodeCreator.CreateNode();
	NodeCreator.Finalize();
	SetNodeMetaData(RootNode, FNodeMetadata::DefaultGraphNode);

// 	if (UPaperZDStateMachineGraph* StateMachineGraph = CastChecked<UPaperZDStateMachineGraph>(&Graph))
// 	{
// 		StateMachineGraph->EntryNode = EntryNode;
// 	}
}
	
void UPaperZDStateMachineSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	//State Action
	TSharedPtr<FPaperZDStateMachineSchemaAction_NewNode> AddStateAction(new FPaperZDStateMachineSchemaAction_NewNode(
		LOCTEXT("PaperZDStateMachineNodeCategory", "Nodes"),
		LOCTEXT("PaperZDStateMachineNodeState","Animation State"), 
		LOCTEXT("NewPaperZDStateMachineNodeTooltipState", "Adds an Animation Node node here"),
		0));
	AddStateAction->NodeClass = UPaperZDStateGraphNode_State::StaticClass();
	ContextMenuBuilder.AddAction(AddStateAction);

	//Conduit Action
	TSharedPtr<FPaperZDStateMachineSchemaAction_NewNode> AddConduitAction(new FPaperZDStateMachineSchemaAction_NewNode(
		LOCTEXT("PaperZDStateMachineNodeCategory", "Nodes"),
		LOCTEXT("PaperZDStateMachineNodeConduit", "Conduit"),
		LOCTEXT("NewPaperZDStateMachineNodeTooltipConduit", "Adds a Conduit node here"),
		0));
	AddConduitAction->NodeClass = UPaperZDStateGraphNode_Conduit::StaticClass();
	ContextMenuBuilder.AddAction(AddConduitAction);

	//The following actions cannot be added when dragging from a pin
	if (ContextMenuBuilder.FromPin == nullptr)
	{
		//Jump Action
		TSharedPtr<FPaperZDStateMachineSchemaAction_NewNode> AddJumpAction(new FPaperZDStateMachineSchemaAction_NewNode(
			LOCTEXT("PaperZDStateMachineNodeCategory", "Nodes"),
			LOCTEXT("PaperZDStateMachineNodeJump", "Jump"),
			LOCTEXT("NewPaperZDStateMachineNodeTooltipJump", "Adds a Jump node here"),
			0));
		AddJumpAction->NodeClass = UPaperZDStateGraphNode_Jump::StaticClass();
		ContextMenuBuilder.AddAction(AddJumpAction);

		//Comment Action
		TSharedPtr<FPaperZDStateMachineSchemaAction_NewComment> AddCommentAction(new FPaperZDStateMachineSchemaAction_NewComment(
			LOCTEXT("PaperZDStateMachineNodeCategory", "Nodes"),
			LOCTEXT("PaperZDStateMachineNodeComment", "Comment"),
			LOCTEXT("NewPaperZDStateMachineNodeTooltipComment", "Adds a Comment Node node here"),
			0));
		ContextMenuBuilder.AddAction(AddCommentAction);
	}
}

void UPaperZDStateMachineSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (Context && Context->Node)
	{
		UBlueprint* OwnerBlueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(Context->Graph);
		FToolMenuSection& Section = Menu->AddSection("ZDAnimationStateMachineNodeActions", LOCTEXT("NodeActionsMenuHeader", "Node Actions"));
		if (!Context->bIsDebugging)
		{
			// Node contextual actions
			Section.AddMenuEntry(FGenericCommands::Get().Delete);
			Section.AddMenuEntry(FGenericCommands::Get().Cut);
			Section.AddMenuEntry(FGenericCommands::Get().Copy);
			Section.AddMenuEntry(FGenericCommands::Get().Duplicate);
			Section.AddMenuEntry(FGraphEditorCommands::Get().ReconstructNodes);
			Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
			if (Context->Node->bCanRenameNode)
			{
				Section.AddMenuEntry(FGenericCommands::Get().Rename);
			}
		}
	}

	Super::GetContextMenuActions(Menu, Context);
}

const FPinConnectionResponse UPaperZDStateMachineSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	if (!PinA || !PinB)
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("No entry pin!"));

	UPaperZDStateGraphNode *NodeA = Cast<UPaperZDStateGraphNode>(PinA->GetOwningNode());
	UPaperZDStateGraphNode *NodeB = Cast<UPaperZDStateGraphNode>(PinB->GetOwningNode());

	//Abort on same nodes
	if (NodeA == NodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot connect to the same node"));
	}

	//Create a transition for state nodes
	if (NodeA->IsA(UPaperZDStateGraphNode_State::StaticClass()) && NodeB->IsA(UPaperZDStateGraphNode_State::StaticClass()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, TEXT("Create a Transition"));
	}
	
	//Don't allow transition-transition connections
	if (NodeA->IsA(UPaperZDStateGraphNode_Transition::StaticClass()) && NodeB->IsA(UPaperZDStateGraphNode_Transition::StaticClass()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot connect transition to transition"));
	}

	//Create Transition - State Connection
	if (NodeA->IsA(UPaperZDStateGraphNode_Transition::StaticClass()) && NodeB->IsA(UPaperZDStateGraphNode_State::StaticClass()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, TEXT("Connect Transition to State"));
	}

	//Create State - Conduit
	if (NodeA->IsA(UPaperZDStateGraphNode_State::StaticClass()) && NodeB->IsA(UPaperZDStateGraphNode_Conduit::StaticClass()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, TEXT("Create a Transition"));
	}

	//Create Conduit - State
	if (NodeA->IsA(UPaperZDStateGraphNode_Conduit::StaticClass()) && NodeB->IsA(UPaperZDStateGraphNode_State::StaticClass()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, TEXT("Create a Transition"));
	}

	//Create Conduit - Conduit
	if (NodeA->IsA(UPaperZDStateGraphNode_Conduit::StaticClass()) && NodeB->IsA(UPaperZDStateGraphNode_Conduit::StaticClass()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, TEXT("Create a Transition"));
	}

	//Create State - Transition Connection
	if (NodeA->IsA(UPaperZDStateGraphNode_State::StaticClass()) && NodeB->IsA(UPaperZDStateGraphNode_Transition::StaticClass()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, TEXT("Connect State to Transition"));
	}

	//Create Root - State Connection
	if (NodeA->IsA(UPaperZDStateGraphNode_Root::StaticClass()) && NodeB->IsA(UPaperZDStateGraphNode_State::StaticClass()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, TEXT("Connect Root to State"));
	}

	//Create Jump - State / Jump - Conduit, the latter entering whichever state the conduit's rules pick
	if (NodeA->IsA(UPaperZDStateGraphNode_Jump::StaticClass()) && (NodeB->IsA(UPaperZDStateGraphNode_State::StaticClass()) || NodeB->IsA(UPaperZDStateGraphNode_Conduit::StaticClass())))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, TEXT("Connect Jump to State"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Connection not Allowed"));
}

bool UPaperZDStateMachineSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	//Transform an OUTPUT - OUTPUT connection into an INPUT - OUTPUT Connection (SPaperZDAnimGraphNode_State has only output as a draggable node, input node is invisible)
	if (PinB->Direction == PinA->Direction)
	{
		if (UPaperZDStateGraphNode* Node = Cast<UPaperZDStateGraphNode>(PinB->GetOwningNode()))
		{
			if (PinA->Direction == EGPD_Input)
			{
				PinB = Node->GetOutputPin();
			}
			else
			{
				PinB = Node->GetInputPin();
			}
		}
	}

	const bool bModified = UEdGraphSchema::TryCreateConnection(PinA, PinB);

	if (bModified)
	{
		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(PinA->GetOwningNode());
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}

	return bModified;
}

bool UPaperZDStateMachineSchema::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	UPaperZDStateGraphNode* NodeA = Cast<UPaperZDStateGraphNode>(PinA->GetOwningNode());
	UPaperZDStateGraphNode* NodeB = Cast<UPaperZDStateGraphNode>(PinB->GetOwningNode());

	if ((NodeA->IsA(UPaperZDStateGraphNode_State::StaticClass()) || NodeA->IsA(UPaperZDStateGraphNode_Conduit::StaticClass())) && (NodeB->IsA(UPaperZDStateGraphNode_State::StaticClass()) || NodeB->IsA(UPaperZDStateGraphNode_Conduit::StaticClass())))
	{
		UPaperZDStateGraphNode_Transition* TransitionNode = FPaperZDStateMachineSchemaAction_NewNode::SpawnGraphNode<UPaperZDStateGraphNode_Transition>(NodeA->GetGraph(), FVector2D(0.0f, 0.0f), false);
		if (PinA->Direction == EEdGraphPinDirection::EGPD_Output)
		{
			TransitionNode->CreateConnections(NodeA, NodeB);
		}
		else
		{
			TransitionNode->CreateConnections(NodeB, NodeA);
		}

		return true;
	}

	return false;
}

void UPaperZDStateMachineSchema::GetGraphDisplayInformation(const UEdGraph& Graph, FGraphDisplayInfo& DisplayInfo) const
{
	DisplayInfo.PlainName = FText::FromString(Graph.GetName());
	DisplayInfo.DisplayName = DisplayInfo.PlainName;
}

TSharedPtr<FEdGraphSchemaAction> UPaperZDStateMachineSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FPaperZDStateMachineSchemaAction_NewComment>(new FPaperZDStateMachineSchemaAction_NewComment);
}

void UPaperZDStateMachineSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("PaperZD", "StateMachine_BreakNodeLinks", "Break Node Links"));

	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(&TargetNode);
	Super::BreakNodeLinks(TargetNode);
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

void UPaperZDStateMachineSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("PaperZD", "StateMachine_BreakPinLinks", "Break Pin Links"));
	// cache this here, as BreakPinLinks can trigger a node reconstruction invalidating the TargetPin references
	UBlueprint* const Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(TargetPin.GetOwningNode());
	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

void UPaperZDStateMachineSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("PaperZD", "StateMachine_BreakSinglePinLink", "Break Pin Link"));
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(TargetPin->GetOwningNode());
	Super::BreakSinglePinLink(SourcePin, TargetPin);
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

#undef LOCTEXT_NAMESPACE