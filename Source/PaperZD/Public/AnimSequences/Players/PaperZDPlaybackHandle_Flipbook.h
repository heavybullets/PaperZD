// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimSequences/Players/PaperZDPlaybackHandle.h"
#include "PaperZDPlaybackHandle_Flipbook.generated.h"

/* Per-component mirroring state tracked across frames so we can unflip cleanly when a frame toggles. */
USTRUCT()
struct FPaperZDFlipbookMirroringState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHorizontal = false;

	UPROPERTY()
	bool bVertical = false;
};

/**
 * Playback handle that manages rendering of Paper2D flipbook components.
 */
UCLASS()
class PAPERZD_API UPaperZDPlaybackHandle_Flipbook : public UPaperZDPlaybackHandle
{
	GENERATED_BODY()

private:
	/* Last mirroring state applied to each render component. */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UPrimitiveComponent>, FPaperZDFlipbookMirroringState> MirroringStatePerComponent;

	void ApplyFrameMirroring(UPrimitiveComponent* RenderComponent, bool bMirrorHorizontal, bool bMirrorVertical);
	void ClearFrameMirroring(UPrimitiveComponent* RenderComponent);

	//~ Begin UPaperZDPlaybackHandle Interface
	virtual void UpdateRenderPlayback(UPrimitiveComponent* RenderComponent, const FPaperZDAnimationPlaybackData& PlaybackData, bool bIsPreviewPlayback = false, int32 LayerIndex = 0, UPaperZDAnimationSkin* SkinOverride = nullptr) override;
	virtual void ConfigureRenderComponent(UPrimitiveComponent* RenderComponent, bool bIsPreviewPlayback = false) override;
	//~ End UPaperZDPlaybackHandle Interface
};
