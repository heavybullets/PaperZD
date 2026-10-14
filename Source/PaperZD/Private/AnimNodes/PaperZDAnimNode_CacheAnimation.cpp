// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "AnimNodes/PaperZDAnimNode_CacheAnimation.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimNode_CacheAnimation)
#endif

FPaperZDAnimNode_CacheAnimation::FPaperZDAnimNode_CacheAnimation()
	: LastGraphUpdateId(INDEX_NONE)
	, bEverInitialized(false)
	, bStaleAnimationData(true)
{}

void FPaperZDAnimNode_CacheAnimation::OnInitialize(const FPaperZDAnimationInitContext& InitContext)
{
	//Will only initialize once, independent if called again (which will happen if any state machine node uses this cache).
	if (!bEverInitialized)
	{
		Animation.Initialize(InitContext);
		bEverInitialized = true;

		//Make sure the next update triggers.
		LastGraphUpdateId = INDEX_NONE;
	}
}

void FPaperZDAnimNode_CacheAnimation::OnUpdate(const FPaperZDAnimationUpdateContext& UpdateContext)
{
	//Only update once per graph traversal. A jump can run an additional traversal in the same engine
	//frame, so the engine frame number alone is not sufficient to identify valid cached data.
	if (LastGraphUpdateId != UpdateContext.GraphUpdateId)
	{
		Animation.Update(UpdateContext);
		LastGraphUpdateId = UpdateContext.GraphUpdateId;
		bStaleAnimationData = true;
	}
}

void FPaperZDAnimNode_CacheAnimation::OnEvaluate(FPaperZDAnimationPlaybackData& OutData)
{
	if (bStaleAnimationData)
	{
		Animation.Evaluate(OutData);
		CachedAnimationData = OutData;
		bStaleAnimationData = false;
	}
	else
	{
		OutData = CachedAnimationData;
	}
}
