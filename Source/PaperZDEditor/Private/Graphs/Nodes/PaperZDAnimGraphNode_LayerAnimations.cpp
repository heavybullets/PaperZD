// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "Graphs/Nodes/PaperZDAnimGraphNode_LayerAnimations.h"
#include "AnimSequences/Sources/PaperZDAnimationSource.h"
#include "Kismet2/CompilerResultsLog.h"
#include "ToolMenu.h"
#include "ScopedTransaction.h"
#include "Kismet2/BlueprintEditorUtils.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimGraphNode_LayerAnimations)
#endif

#define LOCTEXT_NAMESPACE "ZDNodes"

UPaperZDAnimGraphNode_LayerAnimations::UPaperZDAnimGraphNode_LayerAnimations()
	: Super()
{}

FText UPaperZDAnimGraphNode_LayerAnimations::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Blend Animations by Layers"));
}

FLinearColor UPaperZDAnimGraphNode_LayerAnimations::GetNodeTitleColor() const
{
	return FLinearColor(0.2f, 0.8f, 0.2f);
}


FString UPaperZDAnimGraphNode_LayerAnimations::GetNodeCategory() const
{
	return TEXT("Layers");
}

void UPaperZDAnimGraphNode_LayerAnimations::ValidateAnimNodeDuringCompilation(const UPaperZDAnimBP* ForAnimBP, FCompilerResultsLog& MessageLog)
{
	Super::ValidateAnimNodeDuringCompilation(ForAnimBP, MessageLog);

	//We should check that this node isn't being used on an animation source that doesn't support it.
	if (!ForAnimBP->GetSupportedAnimationSource()->SupportsBlendLayers())
	{
		MessageLog.Warning(TEXT("Animation layers aren't supported by the parent animation source, @@ node will not work."), this);
	}
}

bool UPaperZDAnimGraphNode_LayerAnimations::IsCompatibleWithAnimationSource(UPaperZDAnimationSource* InAnimSource) const
{
	return InAnimSource->SupportsBlendLayers();
}

void UPaperZDAnimGraphNode_LayerAnimations::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	//If the reconstruction is due to a pending pin deletion, then we must reorder the connection list to patch it up
	if (RemovedPinIndex != INDEX_NONE)
	{
		TArray<FString> RemovedPropertyNames;
		TArray<FName> NewPinNames;

		// Store new pin names to compare with old pin names
		for (int32 NewPinIndx = 0; NewPinIndx < Pins.Num(); NewPinIndx++)
		{
			NewPinNames.Add(Pins[NewPinIndx]->PinName);
		}

		// don't know which pins are removed yet so find removed pins comparing NewPins and OldPins
		for (int32 OldPinIdx = 0; OldPinIdx < OldPins.Num(); OldPinIdx++)
		{
			const FName OldPinName = OldPins[OldPinIdx]->PinName;
			if (!NewPinNames.Contains(OldPinName))
			{
				const FString OldPinNameStr = OldPinName.ToString();
				const int32 UnderscoreIndex = OldPinNameStr.Find(TEXT("_"), ESearchCase::CaseSensitive);
				if (UnderscoreIndex != INDEX_NONE)
				{
					FString PropertyName = OldPinNameStr.Left(UnderscoreIndex);
					RemovedPropertyNames.Add(MoveTemp(PropertyName));
				}
			}
		}

		for (int32 PinIdx = 0; PinIdx < OldPins.Num(); PinIdx++)
		{
			// Separate the pin name into property name and index
			const FString OldPinNameStr = OldPins[PinIdx]->PinName.ToString();
			const int32 UnderscoreIndex = OldPinNameStr.Find(TEXT("_"), ESearchCase::CaseSensitive);

			if (UnderscoreIndex != INDEX_NONE)
			{
				const FString PropertyName = OldPinNameStr.Left(UnderscoreIndex);

				if (RemovedPropertyNames.Contains(PropertyName))
				{
					const int32 ArrayIndex = FCString::Atoi(*(OldPinNameStr.Mid(UnderscoreIndex + 1)));

					// if array index is matched, removes pins 
					// and if array index is greater than removed index, decrease index
					if (ArrayIndex == RemovedPinIndex)
					{
						OldPins[PinIdx]->MarkAsGarbage();
						OldPins.RemoveAt(PinIdx);
						--PinIdx;
					}
					else if (ArrayIndex > RemovedPinIndex)
					{
						OldPins[PinIdx]->PinName = *FString::Printf(TEXT("%s_%d"), *PropertyName, ArrayIndex - 1);
					}
				}
			}
		}

		//Pin removal complete
		RemovedPinIndex = INDEX_NONE;
	}
}

void UPaperZDAnimGraphNode_LayerAnimations::GetNodeContextMenuActions(UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	if (!Context->bIsDebugging)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("PaperZDSelectByInt", NSLOCTEXT("ZDNodes", "SelectListHeader", "PaperZD"));
			if (Context->Pin != NULL)
			{
				if (Context->Pin->Direction == EGPD_Input)
				{
					Section.AddMenuEntry(
						"RemoveAnimationListPin",
						LOCTEXT("RemoveAnimationListPin_Title", "Remove animation pin."),
						LOCTEXT("RemoveAnimationListPin_Tooltip", "Removes the given pin from the animation entry list."),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateUObject(const_cast<UPaperZDAnimGraphNode_LayerAnimations*>(this), &UPaperZDAnimGraphNode_LayerAnimations::RemoveLayerPin, const_cast<UEdGraphPin*>(Context->Pin)))
					);
				}
			}
			else
			{
				Section.AddMenuEntry(
					"AddAnimationListPin",
					LOCTEXT("AddAnimationListPin_Title", "Add animation pin."),
					LOCTEXT("AddAnimationListPin_Tooltip", "Add a new animation entry to the list."),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateUObject(const_cast<UPaperZDAnimGraphNode_LayerAnimations*>(this), &UPaperZDAnimGraphNode_LayerAnimations::AddLayerPin))
				);
			}
		}
	}
}

void UPaperZDAnimGraphNode_LayerAnimations::AddLayerPin()
{
	FScopedTransaction Transaction(NSLOCTEXT("ZDNodes", "AddLayerListPin", "Add Layer Pin"));
	Modify();

	AnimNode.AnimationLayer.AddDefaulted();
	AnimNode.LayerWeight.Add(1.0f);
	ReconstructNode();

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void UPaperZDAnimGraphNode_LayerAnimations::RemoveLayerPin(UEdGraphPin* Pin)
{
	FScopedTransaction Transaction(NSLOCTEXT("ZDNodes", "RemoveLayerListPin", "Remove Layer Pin"));
	Modify();

	FProperty* AssociatedProperty;
	int32 ArrayIndex;
	GetPinAssociatedProperty(GetFNodeType(), Pin, /*out*/ AssociatedProperty, /*out*/ ArrayIndex);

	if (ArrayIndex != INDEX_NONE)
	{
		RemovedPinIndex = ArrayIndex;
		AnimNode.AnimationLayer.RemoveAt(ArrayIndex);
		AnimNode.LayerWeight.RemoveAt(ArrayIndex);
		ReconstructNode();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

#undef LOCTEXT_NAMESPACE