// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "SEditorViewport.h"
#include "SScrubControlPanel.h"
#include "PreviewScene.h"

class UPaperZDAnimPlayer;
class UPaperZDAnimSequence;
class UPaperZDAnimationSource;
class UPrimitiveComponent;

/**
 * Viewport client that is responsible for animations from an animation source.
 */
class FPaperZDAnimationSourceViewportClient : public FEditorViewportClient
{
	// The preview scene
	FPreviewScene OwnedPreviewScene;

	//The PaperZDPlayer that will manage the AnimSequence
	TObjectPtr<UPaperZDAnimPlayer> Player;

	// The animation source that drives this viewport
	TWeakObjectPtr<const UPaperZDAnimationSource> AnimationSourcePtr;

	// Current animation asset being displayed
	TWeakObjectPtr<const UPaperZDAnimSequence> CurrentAnimSequencePtr;

	// Render component for the sprite being edited
	TWeakObjectPtr<UPrimitiveComponent> AnimatedRenderComponent;

	// List of composite components being edited
	TArray<TWeakObjectPtr<UPrimitiveComponent>> CompositeRenderComponents;

	//Flag to mark the zoom of sprite (on first tick)
	bool bSpriteZoomed;

	//If we are looping
	bool bLooping;

	//Current playback time marker
	float PlaybackTime;

	// Range of times currently being viewed
	mutable float ViewInputMin;
	mutable float ViewInputMax;
	mutable float LastObservedSequenceLength;

public:
	FPaperZDAnimationSourceViewportClient(TWeakObjectPtr<const UPaperZDAnimationSource> InAnimationSourcePtr);
	~FPaperZDAnimationSourceViewportClient();

	// FViewportClient interface
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupViewForRendering(FSceneViewFamily& ViewFamily, FSceneView& View) override;
	// End of FViewportClient interface

	/* Sets the AnimSequence to render, should belong to the animation source (otherwise ignored). */
	void SetAnimSequence(const UPaperZDAnimSequence* AnimSequence);

	/* Called when we need to refresh the animation source due to an external change. */
	void RefreshAnimationSource();

	//Playback Related
	float GetFramesPerSecond() const;
	int32 GetCurrentFrame() const;
	void SetCurrentFrame(int32 NewIndex);
	float GetPlaybackPosition() const;
	uint32 GetTotalFrameCount() const;
	uint32 GetTotalFrameCountPlusOne() const;
	float GetTotalSequenceLength() const;
	void SetPlaybackPosition(float NewTime);
	bool IsLooping() const;
	EPlaybackMode::Type GetPlaybackMode() const; 
	bool IsPlaying() const;
	void SetPlaying(bool bPlaying);
	float GetViewRangeMin() const;
	float GetViewRangeMax() const;
	void SetViewRange(float NewMin, float NewMax);

	FReply OnClick_Forward();
	FReply OnClick_Forward_Step();
	FReply OnClick_Forward_End();
	FReply OnClick_Backward();
	FReply OnClick_Backward_Step();
	FReply OnClick_Backward_End();
	FReply OnClick_ToggleLoop();
	//End of PlaybackRelated

private:
	/* Required so we can have audio notifies. */
	void UpdateAudioListener(const FSceneView& View);

	/* Updates the composite renders component so they are in sync with the animation source. */
	void UpdateCompositeRenderComponents();
};
