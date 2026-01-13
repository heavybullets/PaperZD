// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimSequences/Players/PaperZDPlaybackHandle.h"
#include "PaperZDPlaybackHandle_Flipbook.generated.h"

/**
 * Playback handle that manages rendering of Paper2D flipbook components.
 */
UCLASS()
class PAPERZD_API UPaperZDPlaybackHandle_Flipbook : public UPaperZDPlaybackHandle
{
	GENERATED_BODY()
	
	//~ Begin UPaperZDPlaybackHandle Interface
	virtual void UpdateRenderPlayback(UPrimitiveComponent* RenderComponent, const FPaperZDAnimationPlaybackData& PlaybackData, bool bIsPreviewPlayback = false, int32 LayerIndex = 0, UPaperZDAnimationSkin* SkinOverride = nullptr) override;
	virtual void ConfigureRenderComponent(UPrimitiveComponent* RenderComponent, bool bIsPreviewPlayback = false) override;
	//~ End UPaperZDPlaybackHandle Interface
};
