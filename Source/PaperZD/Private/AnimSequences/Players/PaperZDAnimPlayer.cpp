// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "AnimSequences/Players/PaperZDAnimPlayer.h"
#include "AnimSequences/Players/PaperZDPlaybackHandle.h"
#include "AnimSequences/Sources/PaperZDAnimationSource.h"
#include "AnimSequences/PaperZDAnimSequence.h"
#include "Components/PrimitiveComponent.h"
#include "Notifies/PaperZDAnimNotify_Base.h"
#include "PaperZDStats.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimPlayer)
#endif

#define MIN_RELEVANT_WEIGHT 0.35f

//Stats declarations
DECLARE_CYCLE_STAT(TEXT("Execute AnimNotifies"), STAT_AnimNotifyTick, STATGROUP_PaperZD);

UPaperZDAnimPlayer::UPaperZDAnimPlayer() : Super()
{
	bPlaying = true;
	PlaybackMode = EAnimPlayerPlaybackMode::Forward;
	bPreviewPlayer = false;
	bFireSequenceChangedEvents = false;
}

float UPaperZDAnimPlayer::GetCurrentPlaybackTime() const
{
	return LastWeightedAnimation.PlaybackTime;
}

float UPaperZDAnimPlayer::GetPlaybackProgress() const
{	
	const UPaperZDAnimSequence* PrimaryAnimSequence = LastWeightedAnimation.AnimSequencePtr.Get();
	return PrimaryAnimSequence && PrimaryAnimSequence->GetTotalDuration() > 0.0f ? LastWeightedAnimation.PlaybackTime / PrimaryAnimSequence->GetTotalDuration() : 0.0f;
}

const UPaperZDAnimSequence* UPaperZDAnimPlayer::GetCurrentAnimSequence() const
{
	return LastWeightedAnimation.AnimSequencePtr.Get();
}

void UPaperZDAnimPlayer::ClearCachedAnimationData()
{
	LastPlaybackData = FPaperZDAnimationPlaybackData();
	LastWeightedAnimation = FPaperZDWeightedAnimation();
}

void UPaperZDAnimPlayer::ResumePlayback()
{
	bPlaying = true;
}

void UPaperZDAnimPlayer::PausePlayback()
{
	bPlaying = false;
}

void UPaperZDAnimPlayer::Init(const UPaperZDAnimationSource* AnimationSource)
{
	if (AnimationSource && AnimationSource->GetPlaybackHandleClass())
	{
		//Create the playback handle  and initialize it via the animation source.
		PlaybackHandle = NewObject<UPaperZDPlaybackHandle>(this, AnimationSource->GetPlaybackHandleClass());
		AnimationSource->InitPlaybackHandle(PlaybackHandle);

		//By default allow the sequence changed events to trigger only on animation sources that do not blend, as those are the only ones that make sense to support.
		bFireSequenceChangedEvents = !AnimationSource->SupportsBlending();

		//Optionally configure the render component if it was setup before initializing
		if (RegisteredRenderComponent.IsValid())
		{
			PlaybackHandle->ConfigureRenderComponent(RegisteredRenderComponent.Get(), bPreviewPlayer);
		}

		//Add enough skin slots to accommodate the composite layers
		if (AnimationSource->SupportsCompositeAnimationLayers())
		{
			//Create skin slots for the main and composite layers
			const TArray<FPaperZDCompositeLayerData>& LayerData = AnimationSource->GetCompositeLayerData();
			RegisteredPerLayerSkins.AddDefaulted(LayerData.Num() + 1);
		}
		else
		{
			RegisteredPerLayerSkins.AddDefaulted();
		}
	}
}

void UPaperZDAnimPlayer::SetIsPreviewPlayer(bool bInPreviewPlayer)
{
	bPreviewPlayer = bInPreviewPlayer;
}

bool UPaperZDAnimPlayer::IsRelevantWeight(float Weight) const
{
	return Weight > MIN_RELEVANT_WEIGHT;
}

//Playback controls
void UPaperZDAnimPlayer::TickPlayback(const UPaperZDAnimSequence* AnimSequence, float& PlaybackMarker, float DeltaTime, bool bLooping, UPaperZDAnimInstance* OwningInstance /* = nullptr */, float EffectiveWeight /* = 1.0f */, bool bSkipNotifies /* = false */)
{
	if (AnimSequence && AnimSequence->GetTotalDuration() > 0.0f && bPlaying)
	{
		float PreviousTime = PlaybackMarker;
		bool bSequencePlaybackComplete = false;
		UPrimitiveComponent* RenderComponent = nullptr;

		//Find the render component for this class, if it exists
		if (RegisteredRenderComponent.IsValid())
		{
			RenderComponent = RegisteredRenderComponent.Get();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Trying to tick AnimSequence '%s' without having registered a PrimitiveComponent as Renderer on AnimPlayer '%s'"), *AnimSequence->GetName(), *GetName());
		}
		
		//Adjust for forward/backwards
		if (PlaybackMode == EAnimPlayerPlaybackMode::Reversed)
		{
			DeltaTime *= -1.0f;
		}

		//Depending on the direction the animation is going, we loop differently
		if (DeltaTime > 0.0f)
		{
			PlaybackMarker += DeltaTime;
			bSequencePlaybackComplete = PlaybackMarker >= AnimSequence->GetTotalDuration();
			PlaybackMarker = bLooping ? FMath::Fmod(PlaybackMarker, AnimSequence->GetTotalDuration()) : FMath::Min(PlaybackMarker, AnimSequence->GetTotalDuration());
		}
		else if (DeltaTime < 0.0f)
		{
			PlaybackMarker += DeltaTime;
			bSequencePlaybackComplete = PlaybackMarker <= 0.0f;
			const float Duration = AnimSequence->GetTotalDuration();
			PlaybackMarker = bLooping && bSequencePlaybackComplete ? Duration + PlaybackMarker : FMath::Max(PlaybackMarker, 0);
		}
		
		//With the new playback marker set, we can go ahead and collect every AnimNotify object that should be triggered
		const bool bIsRelevant = IsRelevantWeight(EffectiveWeight);
		if (bIsRelevant && !bSkipNotifies && RenderComponent)
		{
			ProcessAnimSequenceNotifies(AnimSequence, DeltaTime, PlaybackMarker, PreviousTime, EffectiveWeight, OwningInstance);
		}

		//Notify anyone who needs to know about looping or sequence playback completion
		if (bSequencePlaybackComplete)
		{
			//We separate the delegates into two, one for looping, and one for playback completion
			if (bLooping)
			{
				OnPlaybackSequenceLooped.Broadcast(AnimSequence);
			}
			else if (PlaybackMarker != PreviousTime) //Make sure the animation actually just updated, instead of being stopped on the final frame of the animation due to a previous update
			{
				OnPlaybackSequenceComplete.Broadcast(AnimSequence);
				OnPlaybackSequenceComplete_Native.Broadcast(AnimSequence);
			}
		}
	}
}

void UPaperZDAnimPlayer::ProcessAnimSequenceNotifies(const UPaperZDAnimSequence* AnimSequence, float DeltaTime, float CurrentTime, float PreviousTime, float Weight /* = 1.0f */, UPaperZDAnimInstance* OwningInstance /* = nullptr */)
{
	const bool bIsRelevant = IsRelevantWeight(Weight);
	if (bIsRelevant && RegisteredRenderComponent.IsValid())
	{
		for (UPaperZDAnimNotify_Base* Notify : AnimSequence->GetAnimNotifies())
		{
#if WITH_EDITOR
			//Prevent from firing in editor if specifically requested
			if (OwningInstance != nullptr || Notify->bShouldFireInEditor)
#endif
			{
				//Add the notify to the list of deferred updates to do after the render pass
				DeferredAnimNotifyUpdateHandles.Add(FAnimNotifyUpdateHandle(Notify, DeltaTime, CurrentTime, PreviousTime, OwningInstance));
			}
		}
	}
}

void UPaperZDAnimPlayer::PlaySingleAnimation(const UPaperZDAnimSequence* AnimSequence, float Playtime)
{
	if (AnimSequence && PlaybackHandle && RegisteredRenderComponent.IsValid())
	{
		//Because we're not given a playback data struct, we must create one from the given data
		const UPaperZDAnimSequence* PreviousAnimSequence = LastWeightedAnimation.AnimSequencePtr.Get();
		LastPlaybackData.SetAnimation(AnimSequence, Playtime);
		LastWeightedAnimation = LastPlaybackData.WeightedAnimations[0];

		//Potentially trigger the SequenceChanged events (backwards compatibility)
		if (bFireSequenceChangedEvents && OnPlaybackSequenceChanged.IsBound() && LastWeightedAnimation.AnimSequencePtr.Get() != PreviousAnimSequence)
		{
			OnPlaybackSequenceChanged.Broadcast(PreviousAnimSequence, LastWeightedAnimation.AnimSequencePtr.Get(), GetPlaybackProgress());
		}
		
		//Update the playback
		PlaybackHandle->UpdateRenderPlayback(RegisteredRenderComponent.Get(), LastPlaybackData, bPreviewPlayer);

		//Process any secondary linked layer
		for (const FPaperZDLayerLinkData& LinkData : RegisteredLayerLinks)
		{
			if (LinkData.LinkedComponentPtr.IsValid())
			{
				//Update playback, no playback callbacks are triggered for secondary components
				PlaybackHandle->UpdateRenderPlayback(LinkData.LinkedComponentPtr.Get(), LastPlaybackData, bPreviewPlayer, LinkData.LayerIndex);
			}
		}
	}

	//Process any pending notify that was deferred to after the render pass
	ProcessDeferredAnimNotifies();
}

void UPaperZDAnimPlayer::Play(const FPaperZDAnimationPlaybackData& PlaybackData)
{
	if (PlaybackData.WeightedAnimations.Num() && PlaybackHandle && RegisteredRenderComponent.IsValid())
	{
		//Update playback
		UPaperZDAnimationSkin* Skin = RegisteredPerLayerSkins.IsValidIndex(0) ? RegisteredPerLayerSkins[0] : nullptr;
		PlaybackHandle->UpdateRenderPlayback(RegisteredRenderComponent.Get(), PlaybackData, bPreviewPlayer, 0 , Skin);

		//Store information for backwards support 
		const UPaperZDAnimSequence* PreviousAnimSequence = LastWeightedAnimation.AnimSequencePtr.Get();
		LastPlaybackData = PlaybackData;
		LastWeightedAnimation = LastPlaybackData.WeightedAnimations[0];

		//Potentially trigger the SequenceChanged events (backwards compatibility)
		if (bFireSequenceChangedEvents && OnPlaybackSequenceChanged.IsBound() && LastWeightedAnimation.AnimSequencePtr.Get() != PreviousAnimSequence)
		{
			OnPlaybackSequenceChanged.Broadcast(PreviousAnimSequence, LastWeightedAnimation.AnimSequencePtr.Get(), GetPlaybackProgress());
		}

		//Process any secondary linked layer
		for (const FPaperZDLayerLinkData& LinkData : RegisteredLayerLinks)
		{
			if (LinkData.LinkedComponentPtr.IsValid())
			{
				//Update playback, no playback callbacks are triggered for secondary components
				Skin = RegisteredPerLayerSkins.IsValidIndex(LinkData.LayerIndex) ? RegisteredPerLayerSkins[LinkData.LayerIndex] : nullptr;
				PlaybackHandle->UpdateRenderPlayback(LinkData.LinkedComponentPtr.Get(), PlaybackData, bPreviewPlayer, LinkData.LayerIndex, Skin);
			}
		}
	}

	//Process any pending notify that was deferred to after the render pass
	ProcessDeferredAnimNotifies();
}

void UPaperZDAnimPlayer::RegisterRenderComponent(UPrimitiveComponent* RenderComponent)
{
	RegisteredRenderComponent = RenderComponent; 

	//Chance to configure the given render component
	if (PlaybackHandle)
	{
		PlaybackHandle->ConfigureRenderComponent(RenderComponent, bPreviewPlayer);
	}
}

void UPaperZDAnimPlayer::RegisterLayerLinks(const TArray<FPaperZDLayerLinkData>& LayerLinkData)
{
	RegisteredLayerLinks = LayerLinkData;

	//Chance to configure the given render component
	if (PlaybackHandle)
	{
		for (const FPaperZDLayerLinkData& Link : RegisteredLayerLinks)
		{
			if (Link.LinkedComponentPtr.IsValid())
			{
				PlaybackHandle->ConfigureRenderComponent(Link.LinkedComponentPtr.Get(), bPreviewPlayer);
			}
		}
	}
}

void UPaperZDAnimPlayer::ApplyAnimationSkinToLayer(UPaperZDAnimationSkin* Skin, int32 Layer /* = 0 */)
{
	if (RegisteredPerLayerSkins.Num() > Layer)
	{
		RegisteredPerLayerSkins[Layer] = Skin;
	}
}

void UPaperZDAnimPlayer::ProcessDeferredAnimNotifies()
{
	SCOPE_CYCLE_COUNTER(STAT_AnimNotifyTick);

	UPrimitiveComponent* RenderComponent = RegisteredRenderComponent.Get();
	TSet<FAnimNotifyUpdateHandle> LastFrameActiveNotifies = MoveTemp(ActiveNotifies);
	for (const FAnimNotifyUpdateHandle& Handle : DeferredAnimNotifyUpdateHandles)
	{
		if (Handle.AnimNotifyPtr.IsValid())
		{
			//Obtain the current state and clear from the list of active notifies as this now got processed
			bool bCurrentlyActive = LastFrameActiveNotifies.Contains(Handle);
			LastFrameActiveNotifies.Remove(Handle);

			//Process the notify and persist the state
			Handle.AnimNotifyPtr->TickNotify(Handle.DeltaTime, Handle.CurrentTime, Handle.PreviousTime, RenderComponent, bCurrentlyActive, Handle.OwningInstance);
			if (bCurrentlyActive)
			{
				ActiveNotifies.Add(Handle);
			}
		}
	}

	//Then let any notify that got interrupted by not being processed this frame that they got aborted
	for (FAnimNotifyUpdateHandle& Handle : LastFrameActiveNotifies)
	{
		if (Handle.AnimNotifyPtr.IsValid())
		{
			Handle.AnimNotifyPtr->OnNotifyAborted(Handle.OwningInstance);
		}
	}

	DeferredAnimNotifyUpdateHandles.Empty();
}
