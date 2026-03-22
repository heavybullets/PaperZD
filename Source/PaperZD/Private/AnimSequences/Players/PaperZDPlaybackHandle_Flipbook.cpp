// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "AnimSequences/Players/PaperZDPlaybackHandle_Flipbook.h"
#include "AnimSequences/Players/PaperZDAnimationPlaybackData.h"
#include "AnimSequences/Skins/PaperZDAnimationSkin.h"
#include "AnimSequences/PaperZDFlipbookAnimDataSource.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDPlaybackHandle_Flipbook)
#endif

void UPaperZDPlaybackHandle_Flipbook::UpdateRenderPlayback(UPrimitiveComponent* RenderComponent, const FPaperZDAnimationPlaybackData& PlaybackData, bool bIsPreviewPlayback /* = false */, int32 LayerIndex /* = 0 */, UPaperZDAnimationSkin* SkinOverride /* = nullptr */)
{
	UPaperFlipbookComponent* Sprite = Cast<UPaperFlipbookComponent>(RenderComponent);
	if (Sprite)
	{
		//Search for the primary animation, depending on the layer we're rendering
		const FPaperZDWeightedAnimation& PrimaryAnimation = PlaybackData.WeightedAnimations[0];
		
		//Check if we have a skin that wants to 'override' the default animation and skip the main 'render' logic if that's the case.
		//Note: Skins could be applied and still wish for the default animation to be played on certain animation sources.
		const bool bOverridenDefaultAnimation = SkinOverride && SkinOverride->ApplySkinToAnimation(PrimaryAnimation.AnimSequencePtr.Get(), RenderComponent, PlaybackData.DirectionalAngle); 
		if (!bOverridenDefaultAnimation)
		{
			//Use the AnimSequence default animation instead
			const FPaperZDFlipbookAnimDataSource& AnimDataSource = PrimaryAnimation.AnimSequencePtr->GetAnimationData<FPaperZDFlipbookAnimDataSource>(PlaybackData.DirectionalAngle, bIsPreviewPlayback);
			UPaperFlipbook* Flipbook = AnimDataSource.Animation.Get();;
			if (LayerIndex > 0)
			{
				Flipbook = AnimDataSource.CompositeLayerAnimations.IsValidIndex(LayerIndex - 1) ? AnimDataSource.CompositeLayerAnimations[LayerIndex - 1] : nullptr;
			}

			//Check if the flipbook hasn't changed
			if (Sprite->GetFlipbook() != Flipbook)
			{
				Sprite->SetFlipbook(Flipbook);
			}
		}

		//We manage the time manually
		Sprite->SetPlaybackPosition(PrimaryAnimation.PlaybackTime, false);
	}
}

void UPaperZDPlaybackHandle_Flipbook::ConfigureRenderComponent(UPrimitiveComponent* RenderComponent, bool bIsPreviewPlayback /* = false */)
{
	//Stop the flipbook from ticking itself, the playback is managed by the player now
	UPaperFlipbookComponent* Sprite = Cast<UPaperFlipbookComponent>(RenderComponent);
	if (Sprite)
	{
		Sprite->Stop();
		Sprite->SetLooping(false);
	}
}
