// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "AnimSequences/Sources/PaperZDAnimationSource.h"
#include "PaperZDAnimationSource_Flipbook.generated.h"

/**
 * An animation source to be used with Flipbook components.
 */
UCLASS()
class PAPERZD_API UPaperZDAnimationSource_Flipbook : public UPaperZDAnimationSource
{
	GENERATED_BODY()

	/* Offsets when using layered sprites. */
	UPROPERTY(EditAnywhere, Category = "Layers")
	TArray<FPaperZDCompositeLayerData> AdditionalLayerData;

public:
	//ctor
	UPaperZDAnimationSource_Flipbook();
	
	//~ Begin UPaperZDAnimationSource Interface
	virtual TSubclassOf<UPaperZDPlaybackHandle> GetPlaybackHandleClass() const override;
	virtual TSubclassOf<UPrimitiveComponent> GetRenderComponentClass() const override;
	virtual TArray<FPaperZDCompositeLayerData> GetCompositeLayerData() const override;
	//~ End UPaperZDAnimationSource Interface
};
