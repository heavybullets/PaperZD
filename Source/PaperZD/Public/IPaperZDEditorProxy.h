// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class UEdGraph;
class UPaperZDAnimBP;
class UPaperZDAnimationSource;
class UPaperZDAnimSequence;

/**
 * Proxy interface that handles operations called from the runtime section to the editor section.
 * Given that the runtime module cannot have dependence on the Editor Module, this proxy serves as the Editor-only connection
 */
class PAPERZD_API IPaperZDEditorProxy
{
	// Ptr to interface to animation editor operations. 
	static TSharedPtr<IPaperZDEditorProxy> EditorProxy;
	
public:
	virtual ~IPaperZDEditorProxy() {}

	// Sets the Editor Proxy
	static void SetEditorProxy(TSharedPtr<IPaperZDEditorProxy> InEditorProxy);

	// Gets the sound cue graph editor implementation.
	static TSharedPtr<IPaperZDEditorProxy> Get();

	// Called when creating a new AnimBP. 
	virtual UEdGraph* CreateNewAnimationGraph(UPaperZDAnimBP* InAnimBP) = 0;

	//Notify functions related
	virtual void UpdateNotifyFunctions(UPaperZDAnimBP* InAnimBP) = 0;
	virtual void UpdateNotifyFunctions(UPaperZDAnimationSource* InAnimSource) = 0;
	virtual void RenameCustomNotifyGraph(UPaperZDAnimBP* InAnimBP, FName OldName, FName NewName) = 0;

	//Methods for updating version
	virtual void UpdateVersionToAnimSequences(UPaperZDAnimBP* InAnimBP) = 0;
	virtual void UpdateVersionToAnimNodeOuterFix(UPaperZDAnimBP* InAnimBP) = 0;
	virtual void UpdateVersionToAnimSequenceCategoryAdded(UPaperZDAnimBP* InAnimBP) = 0;
	virtual void UpdateVersionToAnimSequenceAsStandaloneAsset(UPaperZDAnimBP* InAnimBP) = 0;
	virtual void UpdateVersionToAnimBlueprintRework(UPaperZDAnimBP* InAnimBP) = 0;
	virtual void UpdateVersionToAnimationSourceAdded(UPaperZDAnimBP* InAnimBP) = 0;
	virtual void UpdateVersionToAnimationSourceAdded(UPaperZDAnimSequence* InAnimSequence) = 0;

	//Helper for getting the animation sequences of a graph
	virtual TArray<FAssetData> GetAnimSequencesForSource(UPaperZDAnimationSource* Source) = 0;
};
