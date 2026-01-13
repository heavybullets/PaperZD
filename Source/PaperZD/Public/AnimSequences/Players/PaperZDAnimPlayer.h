// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "AnimSequences/PaperZDAnimSequence.h"
#include "AnimSequences/Players/PaperZDAnimationPlaybackData.h"
#include "AnimSequences/Players/PaperZDLayerLink.h"
#include "PaperZDAnimPlayer.generated.h"

class UPrimitiveComponent;
class UPaperZDAnimInstance;
class UPaperZDPlaybackHandle;
class UPaperZDAnimationSource;
class UPaperZDAnimNotifyState;
class UPaperZDAnimationSkin;

/**
 * The type of playbacks available for the player
 */
UENUM(BlueprintType)
enum class EAnimPlayerPlaybackMode : uint8
{
	Forward UMETA(DisplayName="Forward"),
	Reversed UMETA(DisplayName = "Reversed")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlaybackSequenceChangedSignature, const UPaperZDAnimSequence*, From, const UPaperZDAnimSequence*, To, float, CurrentProgress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlaybackSequenceCompleteSignature, const UPaperZDAnimSequence*, AnimSequence);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlaybackSequenceLoopedSignature, const UPaperZDAnimSequence*, AnimSequence);

//Native delegates (used for transitional animation notifies)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlaybackSequenceCompleteSignature_Native, const UPaperZDAnimSequence*);

/**
 * Object responsible for driving the animation sequences playback, ticking notifies and parsing the animation data into the final animation mix.
 */
UCLASS()
class PAPERZD_API UPaperZDAnimPlayer : public UObject
{
	GENERATED_BODY()

	/* Pointer to the handle that manages render playback for the animation instance. */
	UPROPERTY(Transient)
	TObjectPtr<UPaperZDPlaybackHandle> PlaybackHandle;

	/* Layer indexed list of skins currently affecting the playback of the animation. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPaperZDAnimationSkin>> RegisteredPerLayerSkins;

	/* The component that the player is using for rendering the animations. */
	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> RegisteredRenderComponent;

	/* Additional components that are linked to specific layers for rendering. */
	UPROPERTY(Transient)
	TArray<FPaperZDLayerLinkData> RegisteredLayerLinks;

	/* Copy of the last stored playback data. */
	FPaperZDAnimationPlaybackData LastPlaybackData;

	/* The main animation sent by the playback data struct, which we use as basis for getting the playback progress information. */
	FPaperZDWeightedAnimation LastWeightedAnimation;

	/**
	 * Holds the information of AnimNotifies that will be triggered after the render pass.
	 * AnimNotifies will be registered for updates during the 'update' pass on the plugin, but need to be actually triggered after the 'render' pass, so any notify
	 * attempting to use updated render data (like Sockets), does have the most up to date information.
	 */
	struct FAnimNotifyUpdateHandle
	{
		TWeakObjectPtr<UPaperZDAnimNotify_Base> AnimNotifyPtr;
		float DeltaTime;
		float CurrentTime;
		float PreviousTime;
		UPaperZDAnimInstance* OwningInstance;

		FAnimNotifyUpdateHandle(UPaperZDAnimNotify_Base* InAnimNotify, float InDeltaTime, float InCurrentTime, float InPreviousTime, UPaperZDAnimInstance* InOwningInstance)
			: AnimNotifyPtr(InAnimNotify)
			, DeltaTime(InDeltaTime)
			, CurrentTime(InCurrentTime)
			, PreviousTime(InPreviousTime)
			, OwningInstance(InOwningInstance)
		{}

		/* Type hash for set. */
		friend uint32 GetTypeHash(const FAnimNotifyUpdateHandle& Handle)
		{
			return GetTypeHash(Handle.AnimNotifyPtr.Get());
		}

		/* operator override for set. */
		bool operator==(const FAnimNotifyUpdateHandle& Other) const
		{
			return AnimNotifyPtr == Other.AnimNotifyPtr;
		}
	};
	TArray<FAnimNotifyUpdateHandle> DeferredAnimNotifyUpdateHandles;

	/* Notifies that are considered 'active' for next update. */
	TSet<FAnimNotifyUpdateHandle> ActiveNotifies;

	//State variables
	bool bPlaying;
	bool bPreviewPlayer;

public:
	/**
	 * Delegate called when the player just starts playing a new sequence, different from the one played last frame.
	 * This delegate only gets called for non-blendable animations (like flipbooks), and will be disabled (not called) for blendable sources (skeleton based animations).
	 * This behavior can be overridden by changing the "bFireqSequenceChangedEvents" flag on the AnimPlayer.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Playback")
	FOnPlaybackSequenceChangedSignature OnPlaybackSequenceChanged;

	/**
	 * Called when the currently played AnimSequence reaches its end. Will only be called for non-looping sequences.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Playback")
	FOnPlaybackSequenceCompleteSignature OnPlaybackSequenceComplete;
	FOnPlaybackSequenceCompleteSignature_Native OnPlaybackSequenceComplete_Native;

	/**
	 * Called when the currently played AnimSequence loops. Only called if the playback is set to loop.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Playback")
	FOnPlaybackSequenceLoopedSignature OnPlaybackSequenceLooped;
		
	/* Current playback mode. */
	UPROPERTY(BlueprintReadWrite, Category = "Playback")
	EAnimPlayerPlaybackMode PlaybackMode;

	/**
	 * If true, the system will fire the "OnPlaybackSequenceChanged".
	 * Defaults to true for "non-blendable" animation sources (like flipbooks).
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Playback")
	bool bFireSequenceChangedEvents;

public:
	//ctor
	UPaperZDAnimPlayer();

	/**
	 * Obtain the last played time of the primary animation that is currently being played.
	 * 
	 * Non blending animation sources (like flipbooks) can use this method to know which animation is playing.
	 * Blending enabled animation sources should be wary that it will only take the most relevant animation into account.
	 */
	UFUNCTION(BlueprintPure, Category = "Playback")
	float GetCurrentPlaybackTime() const;

	/**
	 * Obtain the progress, ranging from [0-1] of the the primary animation that is currently being played.
	 *
	 * Non blending animation sources (like flipbooks) can use this method to know which animation is playing without much concern.
	 * Blending enabled animation sources (2d bones) should be wary that it will only take the most relevant animation into account.
	 */
	UFUNCTION(BlueprintPure, Category = "Playback")
	float GetPlaybackProgress() const;

	/**
	 * Obtain the current, biggest weighted, animation sequence that was rendered this frame.
	 * 
	 * Non blending animation sources (like flipbooks) can use this method to know which animation is playing without much concern.
	 * Blending enabled animation sources (2d bones) should be wary that it will only take the most relevant animation into account.
	 */
	 UFUNCTION(BlueprintPure, Category = "Playback")
	 const UPaperZDAnimSequence* GetCurrentAnimSequence() const;

	/* Resets the cached current animation to none. */
	void ClearCachedAnimationData();

	/* Initializes this player for use with the given Animation Source. */
	void Init(const UPaperZDAnimationSource* AnimationSource);

	/* Changes if this is a preview player or not. Some AnimSequences play or configure differently on PreviewMode. */
	void SetIsPreviewPlayer(bool bInPreviewPlayer);

	/**
	 * Ticks the playback of the given animation asset.
	 * @param AnimSequence		Asset which will be ticked.
	 * @param PlaybackMarker	A reference to the current playback marker that points at the current progress, will get updated with the new value.
	 * @param DeltaTime			The delta time to tick.
	 * @param bLooping			Whether the animation should start again after reaching the end.
	 * @param OwningInstance	The AnimInstance that owns the playback.
	 * @param EffectiveWeight	The weight of the sequence that is playing, if below a given value the sequence will only update the time but not trigger any notifies.
	 * @param bIgnoreNotifies	If no notifies should be triggered, even if their EffectiveWeight allows them to be called.
	 */
	void TickPlayback(const UPaperZDAnimSequence* AnimSequence, float& PlaybackMarker, float DeltaTime, bool bLooping, UPaperZDAnimInstance* OwningInstance = nullptr, float EffectiveWeight = 1.0f, bool bSkipNotifies = false);

	/**
	 * Processes the given AnimSequence and triggers all the notifies that are relevant in the playback window.
	 */
	 void ProcessAnimSequenceNotifies(const UPaperZDAnimSequence* AnimSequence, float DeltaTime, float CurrentTime, float PreviousTime, float Weight = 1.0f, UPaperZDAnimInstance* OwningInstance = nullptr);

	/**
	 * Plays the given single animation
	 */
	void PlaySingleAnimation(const UPaperZDAnimSequence* AnimSequence, float Playtime);

	/**
	 * Evaluates the given animation data structure and potentially mixes all the given animations into a final pose, if the AnimSequence source supports it.
	 */
	 void Play(const FPaperZDAnimationPlaybackData& PlaybackData);

	/**
	 * Registers the render component to use for rendering the animation sequences.
	 * @param RenderComponent			RenderComponent to be registered.
	 */
	UFUNCTION(BlueprintCallable, Category = "Playback")
	void RegisterRenderComponent(UPrimitiveComponent* RenderComponent);

	/**
	 * Registers the additional render components that will be linked to the layer system.
	 * @param LayerLinkData				The layer data to use.
	 */
	 UFUNCTION(BlueprintCallable, Category = "Playback")
	 void RegisterLayerLinks(const TArray<FPaperZDLayerLinkData>& LayerLinkData);

	 /* Applies a skin on a specific animation layer. */
	 UFUNCTION(BlueprintCallable, Category = "Playback")
	 void ApplyAnimationSkinToLayer(UPaperZDAnimationSkin* Skin, int32 Layer = 0);

	/* Returns true if the player is currently running. */
	UFUNCTION(BlueprintPure, Category = "Playback")
	bool IsPlaying() const { return bPlaying; }

	/* Resumes the updating of the animation nodes. */
	UFUNCTION(BlueprintCallable, Category = "Playback")
	void ResumePlayback();

	/* Pauses the updating of the animation nodes. */
	UFUNCTION(BlueprintCallable, Category = "Playback")
	void PausePlayback();
	
	//@Deprecated Function: The playback progress is now managed by each "PlaySequence" node and thus, this method will not do anything.
	UFUNCTION(BlueprintCallable, Category = "Playback", meta = (DeprecatedFunction, DeprecationMessage = "Playback progress is now managed and stored by each PlaySequence node. This method will have no effect and will be removed in a later version."))
	void SetCurrentPlaybackTime(float InCurrentTime) {}

private:
	/* True if the given weight can be considered as "relevant" for triggering notifies and calling events. */
	bool IsRelevantWeight(float Weight) const;

	/* Process the list of notifies that are deferred for updates after the render pass of the plugin is completed. */
	void ProcessDeferredAnimNotifies();
};
