// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PaperZDPlaybackHandle.generated.h"

struct FPaperZDAnimationPlaybackData;
class UPrimitiveComponent;
class UPaperZDAnimationSkin;

/**
 * Object that manages the playback of a certain AnimSequence type and ensures that it renders with the given playback data given by the AnimPlayer.
 * The handle should support the same features as the AnimSequence it serves specifies.
 */
UCLASS(abstract)
class PAPERZD_API UPaperZDPlaybackHandle : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Method called each time the playback of the sequence changes, responsible of updating the render component.
	 * @param RenderComponent		Component which should be updated to reflect the animation changes.
	 * @param PlaybackData			Animation data to be setup on the render component.
	 * @param bIsPreviewPlayback	If this component will be used on an editor preview player.
	 */
	virtual void UpdateRenderPlayback(UPrimitiveComponent* RenderComponent, const FPaperZDAnimationPlaybackData& PlaybackData, bool bIsPreviewPlayback = false, int32 LayerIndex = 0, UPaperZDAnimationSkin* SkinOverride = nullptr) {}

	/**
	 * Called when initializing or adding a primitive component for playback with this handle.
	 * @param RenderComponent		Component to be configured.
	 * @param bIsPreviewPlayback	If this component will be used on an editor preview player.
	 */
	virtual void ConfigureRenderComponent(UPrimitiveComponent* RenderComponent, bool bIsPreviewPlayback = false) {}
};
