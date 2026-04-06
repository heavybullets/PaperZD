// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "AnimSequences/Players/PaperZDPlaybackHandle_Flipbook.h"
#include "AnimSequences/Players/PaperZDAnimationPlaybackData.h"
#include "AnimSequences/Skins/PaperZDAnimationSkin.h"
#include "AnimSequences/PaperZDFlipbookAnimDataSource.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"
#include "Components/SceneComponent.h"

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
		const FPaperZDFlipbookAnimDataSource& AnimDataSource = PrimaryAnimation.AnimSequencePtr->GetAnimationData<FPaperZDFlipbookAnimDataSource>(PlaybackData.DirectionalAngle, bIsPreviewPlayback);
		
		//Check if we have a skin that wants to 'override' the default animation and skip the main 'render' logic if that's the case.
		//Note: Skins could be applied and still wish for the default animation to be played on certain animation sources.
		const bool bOverridenDefaultAnimation = SkinOverride && SkinOverride->ApplySkinToAnimation(PrimaryAnimation.AnimSequencePtr.Get(), RenderComponent, PlaybackData.DirectionalAngle); 
		if (!bOverridenDefaultAnimation)
		{
			//Use the AnimSequence default animation instead
			UPaperFlipbook* Flipbook = AnimDataSource.Animation.Get();
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

		const UPaperFlipbook* ActiveFlipbook = Sprite->GetFlipbook();
		const int32 KeyFrameIndex = ActiveFlipbook ? ActiveFlipbook->GetKeyFrameIndexAtTime(PrimaryAnimation.PlaybackTime, true) : INDEX_NONE;
		ApplyFrameMirroring(RenderComponent, AnimDataSource.IsKeyFrameMirrored(KeyFrameIndex));
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
		ClearFrameMirroring(RenderComponent);
	}
}

void UPaperZDPlaybackHandle_Flipbook::ApplyFrameMirroring(UPrimitiveComponent* RenderComponent, bool bMirrorSprite)
{
	if (USceneComponent* SceneComponent = Cast<USceneComponent>(RenderComponent))
	{
		const bool bWasMirrored = MirroringStatePerComponent.FindRef(RenderComponent);
		FVector RelativeScale = SceneComponent->GetRelativeScale3D();

		if (bWasMirrored)
		{
			RelativeScale.X *= -1.0f;
		}

		RelativeScale.X = bMirrorSprite ? -FMath::Abs(RelativeScale.X) : FMath::Abs(RelativeScale.X);
		SceneComponent->SetRelativeScale3D(RelativeScale);
		MirroringStatePerComponent.Add(RenderComponent, bMirrorSprite);
	}
}

void UPaperZDPlaybackHandle_Flipbook::ClearFrameMirroring(UPrimitiveComponent* RenderComponent)
{
	if (!RenderComponent)
	{
		return;
	}

	if (MirroringStatePerComponent.FindRef(RenderComponent))
	{
		ApplyFrameMirroring(RenderComponent, false);
	}

	MirroringStatePerComponent.Remove(RenderComponent);
}
