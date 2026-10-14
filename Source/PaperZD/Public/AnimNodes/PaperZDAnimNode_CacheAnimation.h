// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "AnimNodes/PaperZDAnimNode_Base.h"
#include "PaperZDAnimNode_CacheAnimation.generated.h"

/**
 * Caches the animation data enabling it to be read from multiple nodes at a time, making sure the animation sub-tree is only updated and evaluated once per graph update.
 * To access the data use the "Use Cached Animation" node.
 */
USTRUCT()
struct PAPERZD_API FPaperZDAnimNode_CacheAnimation : public FPaperZDAnimNode_Base
{
	GENERATED_BODY()

	/* Animation data. */
	UPROPERTY(EditAnywhere, Category = "Input")
	FPaperZDAnimDataLink Animation;

	/* The animation data that we're caching, stored so we don't evaluate the same data multiple times during the same graph update. */
	FPaperZDAnimationPlaybackData CachedAnimationData;

	/* Identifier of the graph update that produced the current cached data. */
	uint64 LastGraphUpdateId;

	/* Flag for knowing if this node has ever been initialized, needed due to state nodes re-initializing whenever they become relevant. */
	bool bEverInitialized;

	/* Flag for knowing if the cached data is still valid or should be re-evaluated. */
	bool bStaleAnimationData;

public:
	//ctor
	FPaperZDAnimNode_CacheAnimation();

	//~Begin FPaperZDAnimNode_Base Interface
	virtual void OnInitialize(const FPaperZDAnimationInitContext& InitContext) override;
	virtual void OnUpdate(const FPaperZDAnimationUpdateContext& UpdateContext) override;
	virtual void OnEvaluate(FPaperZDAnimationPlaybackData& OutData) override;
	//~End FPaperZDAnimNode_Base Interface
};
