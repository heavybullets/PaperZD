// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PaperZDComponentReference.h"
#include "PaperZDLayerLink.generated.h"

//fwd
class UPrimitiveComponent;

/* A layer link that has its linked component cached for faster processing. Used internally by the AnimPlayer */
USTRUCT(BlueprintType)
struct FPaperZDLayerLinkData
{
	GENERATED_BODY()

	/* Render component linked to the layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PaperZD")
	TWeakObjectPtr<UPrimitiveComponent> LinkedComponentPtr;

	/* The layer index that we're linked to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PaperZD")
	int32 LayerIndex;

public:
	FPaperZDLayerLinkData()
		: LayerIndex(0)
	{}

	FPaperZDLayerLinkData(UPrimitiveComponent* InPrimitiveComponent, int32 InLayerIndex)
		: LinkedComponentPtr(InPrimitiveComponent)
		, LayerIndex(InLayerIndex)
	{}
};
