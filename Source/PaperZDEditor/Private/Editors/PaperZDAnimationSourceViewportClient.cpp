// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "Editors/PaperZDAnimationSourceViewportClient.h"
#include "AnimSequences/Players/PaperZDAnimPlayer.h"
#include "AnimSequences/PaperZDAnimSequence.h"
#include "AnimSequences/Sources/PaperZDAnimationSource.h"
#include "AssetEditorModeManager.h"
#include "Utils.h"
#include "Engine/CollisionProfile.h"
#include "AudioDevice.h"
#include "GameFramework/WorldSettings.h"

namespace FPaperZDViewportUtils
{
	const FVector PaperAxisX(1.0f, 0.0f, 0.0f);
	const FVector PaperAxisY(0.0f, 0.0f, 1.0f);
	const FVector PaperAxisZ(0.0f, 1.0f, 0.0f);
}

FPaperZDAnimationSourceViewportClient::FPaperZDAnimationSourceViewportClient(TWeakObjectPtr<const UPaperZDAnimationSource> InAnimationSourcePtr) 
	: FEditorViewportClient(nullptr)
{
	AnimationSourcePtr = InAnimationSourcePtr;
	PlaybackTime = 0.0f;
	check(AnimationSourcePtr.IsValid());

	//Config
	SetRealtime(true);
	DrawHelper.bDrawGrid = false;
	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.SetCompositeEditorPrimitives(false);
	bLooping = true;
		
	// Create a render component for the sprite being edited
	AnimatedRenderComponent = NewObject<UPrimitiveComponent>(GetTransientPackage(), AnimationSourcePtr->GetRenderComponentClass());
	AnimatedRenderComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	AnimatedRenderComponent->UpdateBounds();

	//defer zoom
	bSpriteZoomed = false;
	
	//Create the player and configure it
	Player = NewObject<UPaperZDAnimPlayer>();
	Player->SetIsPreviewPlayer(true);
	Player->Init(AnimationSourcePtr.Get());
	Player->RegisterRenderComponent(AnimatedRenderComponent.Get());
	
	//Add the player to the root set, when compiling the AnimBP the transient package gets collected, this makes sure the player only gets gc'ed when this window gets destroyed
	Player->AddToRoot();
	PreviewScene = &OwnedPreviewScene;
	PreviewScene->AddComponent(AnimatedRenderComponent.Get(), FTransform::Identity);

	//Set composite layers
	UpdateCompositeRenderComponents();

	// Set audio mute option
	UWorld* World = PreviewScene->GetWorld();
	if (World)
	{
		World->bAllowAudioPlayback = true;//!ConfigOption->bMuteAudio; @TODO: Add a new button for mutting

		if (FAudioDevice* AudioDevice = World->GetAudioDeviceRaw())
		{
			AudioDevice->SetUseAttenuationForNonGameWorlds(false);
		}
	}

	// We need to configure WorldSettings if we want to be able to see OneShot particles that aren't attached, as they get attached to this actor
	AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings(true);
	WorldSettings->bEnableWorldBoundsChecks = false;
	WorldSettings->SetIsTemporarilyHiddenInEditor(false);
	
	// Get the correct general direction of the perspective mode; the distance doesn't matter much as we've queued up a deferred zoom that will calculate a much better distance
	SetViewModes(VMI_Lit, VMI_Lit);
	SetViewportType(LVT_OrthoXZ);
	SetInitialViewTransform(LVT_Perspective, -100.0f * FPaperZDViewportUtils::PaperAxisZ, FPaperZDViewportUtils::PaperAxisZ.Rotation().GetInverse(), 1.0f);
}

FPaperZDAnimationSourceViewportClient::~FPaperZDAnimationSourceViewportClient()
{
	//Make sure the player gets gc'ed now that this view is no longer valid
	Player->RemoveFromRoot();
}

// FViewportClient interface
void FPaperZDAnimationSourceViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);

	if (AnimatedRenderComponent.IsValid())
	{
		FUnrealEdUtils::DrawWidget(View, PDI, AnimatedRenderComponent->GetComponentToWorld().ToMatrixWithScale(), 0, 0, EAxisList::Screen, EWidgetMovementMode::WMM_Translate);
	}
}

void FPaperZDAnimationSourceViewportClient::DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas)
{
	FEditorViewportClient::DrawCanvas(InViewport, View, Canvas);
}

void FPaperZDAnimationSourceViewportClient::Tick(float DeltaSeconds)
{
	//Make the player manager tick, if we have a valid sequence setup
	if (CurrentAnimSequencePtr.IsValid())
	{
		Player->TickPlayback(CurrentAnimSequencePtr.Get(), PlaybackTime, DeltaSeconds, bLooping);
		Player->PlaySingleAnimation(CurrentAnimSequencePtr.Get(), PlaybackTime);

		//Adjust size whenever we need to, but only do so if we have an animation, otherwise the size will be invalid
		FIntPoint Size = Viewport->GetSizeXY();
		if (!bSpriteZoomed && (Size.X > 0) && (Size.Y > 0))
		{
			//Calculate the Zoomed Out Bounds
			const FBox BoundsToFocus = AnimatedRenderComponent->Bounds.GetBox();
			FocusViewportOnBox(BoundsToFocus, true /*bDeferZoomToSpriteIsInstant*/);
			bSpriteZoomed = true;
		}
	}

	FEditorViewportClient::Tick(DeltaSeconds);

	if (!GIntraFrameDebuggingGameThread)
	{
		OwnedPreviewScene.GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

void FPaperZDAnimationSourceViewportClient::SetupViewForRendering(FSceneViewFamily& ViewFamily, FSceneView& View)
{
	FEditorViewportClient::SetupViewForRendering(ViewFamily, View);

	if (bHasAudioFocus)
	{
		UpdateAudioListener(View);
	}
}
// End of FViewportClient interface

void FPaperZDAnimationSourceViewportClient::UpdateAudioListener(const FSceneView& View)
{
	UWorld* ViewportWorld = GetWorld();

	if (ViewportWorld)
	{
		if (FAudioDevice* AudioDevice = ViewportWorld->GetAudioDeviceRaw())
		{
			const FVector& ViewLocation = GetViewLocation();
			const FRotator& ViewRotation = GetViewRotation();

			FTransform ListenerTransform(ViewRotation);
			ListenerTransform.SetLocation(ViewLocation);

			AudioDevice->SetListener(ViewportWorld, 0, ListenerTransform, 0.0f);
		}
	}
}

void FPaperZDAnimationSourceViewportClient::RefreshAnimationSource()
{
	//Just request another init, it should take care of any setup
	Player->Init(AnimationSourcePtr.Get());
	UpdateCompositeRenderComponents();
}

void FPaperZDAnimationSourceViewportClient::SetAnimSequence(const UPaperZDAnimSequence* AnimSequence)
{
	CurrentAnimSequencePtr = AnimSequence;
	PlaybackTime = 0.0f;
}

float FPaperZDAnimationSourceViewportClient::GetPlaybackPosition() const
{
	return PlaybackTime;
}

uint32 FPaperZDAnimationSourceViewportClient::GetTotalFrameCount() const
{
	return (uint32)FMath::CeilToInt(GetFramesPerSecond() * GetTotalSequenceLength());
}

uint32 FPaperZDAnimationSourceViewportClient::GetTotalFrameCountPlusOne() const
{
	return GetTotalFrameCount() + 1;
}

float FPaperZDAnimationSourceViewportClient::GetTotalSequenceLength() const
{
	return CurrentAnimSequencePtr.IsValid() ? CurrentAnimSequencePtr->GetTotalDuration() : 0.0f;
}

void FPaperZDAnimationSourceViewportClient::SetPlaybackPosition(float NewTime)
{
	PlaybackTime = FMath::Clamp<float>(NewTime, 0.0f, GetTotalSequenceLength());
}

float FPaperZDAnimationSourceViewportClient::GetFramesPerSecond() const
{
	return CurrentAnimSequencePtr.IsValid() ? CurrentAnimSequencePtr->GetFramesPerSecond() : 15.0f;
}

int32 FPaperZDAnimationSourceViewportClient::GetCurrentFrame() const
{
	const int32 TotalLengthInFrames = GetTotalFrameCount();

	if (TotalLengthInFrames == 0)
	{
		return INDEX_NONE;
	}
	else
	{
		return FMath::Clamp<int32>((int32)(GetPlaybackPosition() * GetFramesPerSecond()), 0, TotalLengthInFrames - 1);
	}
}

void FPaperZDAnimationSourceViewportClient::SetCurrentFrame(int32 NewIndex)
{
	const int32 TotalLengthInFrames = GetTotalFrameCount();
	if (TotalLengthInFrames > 0)
	{
		int32 ClampedIndex = FMath::Clamp<int32>(NewIndex, 0, TotalLengthInFrames - 1);
		SetPlaybackPosition(ClampedIndex / GetFramesPerSecond());
	}
	else
	{
		SetPlaybackPosition(0.0f);
	}
}

FReply FPaperZDAnimationSourceViewportClient::OnClick_Forward()
{
	const bool bIsReverse = Player->PlaybackMode == EAnimPlayerPlaybackMode::Reversed;
	const bool bIsPlaying = Player->IsPlaying();
		
	if (bIsReverse && bIsPlaying)
	{
		// Play forwards instead of backwards
		Player->PlaybackMode = EAnimPlayerPlaybackMode::Forward;
	}
	else if (bIsPlaying)
	{
		// Was already playing forwards, so pause
		Player->PausePlayback();
	}
	else
	{
		// Was paused, start playing
		Player->ResumePlayback();
	}
	
	return FReply::Handled();
}

FReply FPaperZDAnimationSourceViewportClient::OnClick_Forward_Step()
{
	SetCurrentFrame(GetCurrentFrame() + 1);
	return FReply::Handled();
}

FReply FPaperZDAnimationSourceViewportClient::OnClick_Forward_End()
{
	Player->PausePlayback();
	SetPlaybackPosition(GetTotalSequenceLength());
	return FReply::Handled();
}

FReply FPaperZDAnimationSourceViewportClient::OnClick_Backward()
{
	const bool bIsReverse = Player->PlaybackMode == EAnimPlayerPlaybackMode::Reversed;
	const bool bIsPlaying = Player->IsPlaying();

	if (bIsReverse && bIsPlaying)
	{
		// Was already playing backwards, so pause
		Player->PausePlayback();
	}
	else if (bIsPlaying)
	{
		// Play backwards instead of forwards
		Player->PlaybackMode = EAnimPlayerPlaybackMode::Reversed;
	}
	else
	{
		// Was paused, start reversing
		Player->PlaybackMode = EAnimPlayerPlaybackMode::Reversed;
		Player->ResumePlayback();
	}

	return FReply::Handled();
}

FReply FPaperZDAnimationSourceViewportClient::OnClick_Backward_Step()
{
	Player->PausePlayback();
	SetCurrentFrame(GetCurrentFrame() - 1);
	return FReply::Handled();
}

FReply FPaperZDAnimationSourceViewportClient::OnClick_Backward_End()
{
	Player->PausePlayback();
	SetPlaybackPosition(0.0f);
	return FReply::Handled();
}

FReply FPaperZDAnimationSourceViewportClient::OnClick_ToggleLoop()
{
	bLooping = !bLooping;
	return FReply::Handled();
}

EPlaybackMode::Type FPaperZDAnimationSourceViewportClient::GetPlaybackMode() const
{
	if (Player->IsPlaying())
	{
		return Player->PlaybackMode == EAnimPlayerPlaybackMode::Reversed ? EPlaybackMode::PlayingReverse : EPlaybackMode::PlayingForward;
	}
	else
	{
		return EPlaybackMode::Stopped;
	}
}

bool FPaperZDAnimationSourceViewportClient::IsPlaying() const
{
	return Player->IsPlaying();
}

void FPaperZDAnimationSourceViewportClient::SetPlaying(bool bPlaying)
{
	if (bPlaying)
	{
		Player->ResumePlayback();
	}
	else
	{
		Player->PausePlayback();
	}
}

bool FPaperZDAnimationSourceViewportClient::IsLooping() const
{
	return bLooping;
}

float FPaperZDAnimationSourceViewportClient::GetViewRangeMin() const
{
	return ViewInputMin;
}

float FPaperZDAnimationSourceViewportClient::GetViewRangeMax() const
{
	// See if the flipbook changed length, and if so reframe the scrub bar to include the full length
	//@TODO: This is a pretty odd place to put it, but there's no callback for a modified timeline at the moment, so...
	const float SequenceLength = GetTotalSequenceLength();
	if (SequenceLength != LastObservedSequenceLength)
	{
		LastObservedSequenceLength = SequenceLength;
		ViewInputMin = 0.0f;
		ViewInputMax = SequenceLength;
	}

	return ViewInputMax;
}

void FPaperZDAnimationSourceViewportClient::SetViewRange(float NewMin, float NewMax)
{
	ViewInputMin = FMath::Max<float>(NewMin, 0.0f);
	ViewInputMax = FMath::Min<float>(NewMax, GetTotalSequenceLength());
}

void FPaperZDAnimationSourceViewportClient::UpdateCompositeRenderComponents()
{
	//Create the composite render components if they exist
	if (AnimationSourcePtr->SupportsCompositeAnimationLayers())
	{
		TArray<FPaperZDCompositeLayerData> LayerData = AnimationSourcePtr->GetCompositeLayerData();
		TArray<FPaperZDLayerLinkData> LinkData;

		//Check whether we need to add or destroy components
		int32 ComponentsToAdd = LayerData.Num() - CompositeRenderComponents.Num();
		if (ComponentsToAdd > 0)
		{
			CompositeRenderComponents.Reserve(LayerData.Num());
			while(LayerData.Num() > CompositeRenderComponents.Num())
			{
				const int32 Index = CompositeRenderComponents.Num();
				const FPaperZDCompositeLayerData& Data = LayerData[Index];
				UPrimitiveComponent* CompositeComponent = NewObject<UPrimitiveComponent>(GetTransientPackage(), AnimationSourcePtr->GetRenderComponentClass());
				PreviewScene->AddComponent(CompositeComponent, FTransform(Data.Offset));
				LinkData.Emplace(CompositeComponent, Index + 1);
				CompositeRenderComponents.Add(CompositeComponent);
			}
		}
		else if (ComponentsToAdd < 0)
		{
			//Remove
			while (LayerData.Num() < CompositeRenderComponents.Num())
			{
				const int32 Index = CompositeRenderComponents.Num() - 1;
				PreviewScene->RemoveComponent(CompositeRenderComponents[Index].Get());
				CompositeRenderComponents.RemoveAt(Index, EAllowShrinking::No);
			}

			CompositeRenderComponents.Shrink();
		}

		//Make sure the composite layers are synced
		check(LayerData.Num() == CompositeRenderComponents.Num());
		LinkData.Reserve(LayerData.Num());
		for (int32 i = 0; i < LayerData.Num(); i++)
		{
			const FPaperZDCompositeLayerData& Data = LayerData[i];
			CompositeRenderComponents[i]->SetRelativeLocation(Data.Offset);
		}

		//Link the layers to the player
		Player->RegisterLayerLinks(LinkData);
	}
}
