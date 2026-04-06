// Copyright 2017 ~ 2026 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PaperFlipbook.h"
#include "PaperZDFlipbookAnimDataSource.generated.h"

/* The animation data source to be used by the Flipbook AnimSequence. */
USTRUCT()
struct FPaperZDFlipbookAnimDataSource
{
	GENERATED_BODY()

	/* Main animation to render as the base layer. */
	UPROPERTY(EditAnywhere, Category = "AnimSequence")
	TObjectPtr<UPaperFlipbook> Animation;

	/* The additional layers to render alongside the main animation. */
	UPROPERTY(EditAnywhere, Category = "AnimSequence")
	TArray<UPaperFlipbook*> CompositeLayerAnimations;

	/* Keyframe indices that should render mirrored for this animation entry. */
	UPROPERTY(EditAnywhere, Category = "AnimSequence")
	TArray<int32> MirroredKeyFrames;

public:
	//ctor
	FPaperZDFlipbookAnimDataSource(UPaperFlipbook* InFlipbook = nullptr)
		: Animation(InFlipbook)
	{}

	/* Returns true if the provided keyframe index should be rendered mirrored. */
	bool IsKeyFrameMirrored(int32 KeyFrameIndex) const
	{
		return MirroredKeyFrames.Contains(KeyFrameIndex);
	}
};
