// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPaperZDEditorProxy.h"

class UEdGraph;
class UPaperZDStateGraphNode;

/**
 * Proxy class used to make Editor related changes that need to be called from the Runtime module of this plugin.
 * Normally called by the Animation Blueprint to create nodes, update versions and handle AnimSequence duplication
 */
class FPaperZDRuntimeEditorProxy : public IPaperZDEditorProxy
{
public:
	FPaperZDRuntimeEditorProxy() {}
	~FPaperZDRuntimeEditorProxy() {}

	//Registers an instance of the RuntimeEditorProxy with the AnimBP class
	static void Register();

	// ~IPaperZDEditorProxy
	virtual UEdGraph* CreateNewAnimationGraph(UPaperZDAnimBP* InAnimBP) override;
	virtual TArray<FAssetData> GetAnimSequencesForSource(UPaperZDAnimationSource* Source) override;
	virtual void UpdateNotifyFunctions(UPaperZDAnimationSource* InAnimSource) override;
	virtual void UpdateNotifyFunctions(UPaperZDAnimBP* InAnimBP) override;
	virtual void RenameCustomNotifyGraph(UPaperZDAnimBP* InAnimBP, FName OldName, FName NewName) override;
	virtual void UpdateVersionToAnimSequences(UPaperZDAnimBP *InAnimBP) override;
	virtual void UpdateVersionToAnimNodeOuterFix(UPaperZDAnimBP *InAnimBP) override;
	virtual void UpdateVersionToAnimSequenceCategoryAdded(UPaperZDAnimBP *InAnimBP) override;
	virtual void UpdateVersionToAnimSequenceAsStandaloneAsset(UPaperZDAnimBP* InAnimBP) override;
	virtual void UpdateVersionToAnimBlueprintRework(UPaperZDAnimBP* InAnimBP) override;
	virtual void UpdateVersionToAnimationSourceAdded(UPaperZDAnimBP* InAnimBP) override;
	virtual void UpdateVersionToAnimationSourceAdded(UPaperZDAnimSequence* InAnimSequence) override;
	// ~IPaperZDEditorProxy

private:
	/* Requests every K2 node to be refreshed on the Animation Blueprint. */
	void RefreshK2Nodes(UPaperZDAnimBP* InAnimBP);
};

