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
		ApplyFrameMirroring(
			RenderComponent,
			AnimDataSource.IsKeyFrameMirroredHorizontal(KeyFrameIndex),
			AnimDataSource.IsKeyFrameMirroredVertical(KeyFrameIndex));
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

void UPaperZDPlaybackHandle_Flipbook::ApplyFrameMirroring(UPrimitiveComponent* RenderComponent, bool bMirrorHorizontal, bool bMirrorVertical)
{
	if (USceneComponent* SceneComponent = Cast<USceneComponent>(RenderComponent))
	{
		const FPaperZDFlipbookMirroringState PreviousState = MirroringStatePerComponent.FindRef(RenderComponent);
		FVector RelativeScale = SceneComponent->GetRelativeScale3D();

		//Normalize axes that were flipped on the previous frame so we can re-derive the absolute scale cleanly.
		//Paper sprites face along Y, so visual vertical flipping is a Z-scale flip (not Y).
		if (PreviousState.bHorizontal)
		{
			RelativeScale.X *= -1.0f;
		}
		if (PreviousState.bVertical)
		{
			RelativeScale.Z *= -1.0f;
		}

		//Apply the new per-axis state. Working from the absolute value keeps sign correct regardless of the prior frame.
		RelativeScale.X = bMirrorHorizontal ? -FMath::Abs(RelativeScale.X) : FMath::Abs(RelativeScale.X);
		RelativeScale.Z = bMirrorVertical ? -FMath::Abs(RelativeScale.Z) : FMath::Abs(RelativeScale.Z);
		SceneComponent->SetRelativeScale3D(RelativeScale);

		FPaperZDFlipbookMirroringState NewState;
		NewState.bHorizontal = bMirrorHorizontal;
		NewState.bVertical = bMirrorVertical;
		MirroringStatePerComponent.Add(RenderComponent, NewState);
	}
}

void UPaperZDPlaybackHandle_Flipbook::ClearFrameMirroring(UPrimitiveComponent* RenderComponent)
{
	if (!RenderComponent)
	{
		return;
	}

	const FPaperZDFlipbookMirroringState* ExistingState = MirroringStatePerComponent.Find(RenderComponent);
	if (ExistingState && (ExistingState->bHorizontal || ExistingState->bVertical))
	{
		//Unflip any currently-mirrored axes back to their positive form.
		ApplyFrameMirroring(RenderComponent, false, false);
	}

	MirroringStatePerComponent.Remove(RenderComponent);
}
