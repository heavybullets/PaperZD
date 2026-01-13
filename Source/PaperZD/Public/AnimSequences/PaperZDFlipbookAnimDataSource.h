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

public:
	//ctor
	FPaperZDFlipbookAnimDataSource(UPaperFlipbook* InFlipbook = nullptr)
		: Animation(InFlipbook)
	{}
};
