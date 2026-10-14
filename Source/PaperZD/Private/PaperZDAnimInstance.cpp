// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "PaperZDAnimInstance.h"
#include "PaperZDAnimBPGeneratedClass.h"
#include "PaperZDCharacter.h"
#include "PaperZDStats.h"
#include "AnimSequences/Sources/PaperZDAnimationSource.h"
#include "AnimSequences/Players/PaperZDAnimPlayer.h"
#include "AnimNodes/PaperZDAnimNode_Sink.h"
#include "AnimNodes/PaperZDAnimNode_StateMachine.h"
#include "AnimNodes/PaperZDAnimNode_PlaySequence.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/MessageLog.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimInstance)
#endif

#define LOCTEXT_NAMESPACE "PaperZD"

//Stats declarations
DECLARE_CYCLE_STAT(TEXT("[TOTAL]"), STAT_TickAnimInstance, STATGROUP_PaperZD);
DECLARE_CYCLE_STAT(TEXT("Update AnimGraph"), STAT_UpdateAnimGraph, STATGROUP_PaperZD);
DECLARE_CYCLE_STAT(TEXT("Render Animations"), STAT_RenderAnimations, STATGROUP_PaperZD);
DECLARE_CYCLE_STAT(TEXT("Blueprint Tick"), STAT_AnimBPTick, STATGROUP_PaperZD);

UPaperZDAnimInstance::UPaperZDAnimInstance()
	: Super()
{
	//Setup CDO values
	bIgnoreTimeDilation = false;
	bAllowTransitionalStates = true;
	bSequencerOverride = false;
	bIsProcessingAnimations = false;
	AnimationUpdateCounter = 0;
}

UWorld* UPaperZDAnimInstance::GetWorld() const
{
	return Manager.GetObject() ? Manager->OnGetWorld() : nullptr;
}

UPaperZDAnimPlayer* UPaperZDAnimInstance::GetPlayer() const
{
	return AnimPlayer;
}

void UPaperZDAnimInstance::ApplyAnimationSkinToLayer(UPaperZDAnimationSkin* Skin, int32 Layer /* = 0 */)
{
	if (AnimPlayer)
	{
		AnimPlayer->ApplyAnimationSkinToLayer(Skin, Layer);
	}
}

void UPaperZDAnimInstance::RemoveSkinsFromLayer(int32 Layer /* = 0 */)
{
	if (AnimPlayer)
	{
		AnimPlayer->ApplyAnimationSkinToLayer(nullptr, Layer);
	}
}

bool UPaperZDAnimInstance::AllowsTransitionalStates() const
{
	return bAllowTransitionalStates;
}

UFunction* UPaperZDAnimInstance::FindAnimNotifyFunction(FName AnimNotifyName) const
{
	UPaperZDAnimBPGeneratedClass* AnimClass = CastChecked<UPaperZDAnimBPGeneratedClass>(GetClass());
	return AnimClass->FindAnimNotifyFunction(AnimNotifyName);
}

void UPaperZDAnimInstance::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_TickAnimInstance);
	if (bIgnoreTimeDilation)
	{
		//Modify the DeltaTime to use an non-dilated value
		DeltaTime = GetDeltaTimeIgnoredDilation(DeltaTime);
	}

	//Process the animation nodes
	ProcessAnimations(DeltaTime);

	//Call the blueprint method, if it exists
	{
		SCOPE_CYCLE_COUNTER(STAT_AnimBPTick);
		OnTick(DeltaTime);
	}
}

void UPaperZDAnimInstance::OnTick_Implementation(float DeltaTime)
{
	//Empty implementation for child classes
}

void UPaperZDAnimInstance::Init(TScriptInterface<IPaperZDAnimInstanceManager> InManager)
{
	//Store the manager for later use
	Manager = InManager;

	//Cache the root node and supported sequence
	RootNode = nullptr;
	const UPaperZDAnimationSource* AnimSource = nullptr;
	if (UPaperZDAnimBPGeneratedClass* AnimClass = Cast<UPaperZDAnimBPGeneratedClass>(GetClass()))
	{
		RootNode = AnimClass->GetRootNode(this);
		AnimSource = AnimClass->GetSupportedAnimationSource();
	}

	//Init the player
	AnimPlayer = NewObject<UPaperZDAnimPlayer>(this);
	AnimPlayer->Init(AnimSource);
	AnimPlayer->RegisterRenderComponent(Manager->GetRenderComponent());
	AnimPlayer->RegisterLayerLinks(Manager->GetLayerLinkData());
	Manager->OnSetupAnimPlayer(AnimPlayer);

	//Bind the corresponding delegates
	AnimPlayer->OnPlaybackSequenceChanged.AddDynamic(this, &UPaperZDAnimInstance::OnAnimSequenceUpdated);
	AnimPlayer->OnPlaybackSequenceComplete.AddDynamic(this, &UPaperZDAnimInstance::OnAnimSequencePlaybackComplete);

	//Let the blueprint initialize any variables we might need for updates
	//We do this first as some AnimNodes might require access to blueprint logic on their initialization methods
	OnInit();

	//Initialize every animation node
	if (RootNode)
	{
		FPaperZDAnimationInitContext InitContext(this);
		RootNode->Initialize(InitContext);
	}
}

void UPaperZDAnimInstance::OnInit_Implementation()
{
	//Empty implementation for child classes
}

AActor* UPaperZDAnimInstance::GetOwningActor() const
{
	return Manager.GetObject() ? Manager->GetOwningActor() : nullptr;
}

APaperZDCharacter* UPaperZDAnimInstance::GetPaperCharacter() const
{
	return Cast<APaperZDCharacter>(GetOwningActor());
}

void UPaperZDAnimInstance::JumpToNode(FName JumpName, FName StateMachineName /* = NAME_None */)
{
	PendingJumpRequests.Emplace(JumpName, StateMachineName);

	//If this is called from an animation callback, the active ProcessAnimations call will flush the request
	//after its current pass. Otherwise commit it synchronously so player/render state is coherent on return.
	if (!bIsProcessingAnimations)
	{
		ProcessAnimations(0.0f, true);
	}
}

bool UPaperZDAnimInstance::ApplyJumpToNode(FName JumpName, FName StateMachineName)
{
	bool bJumped = false;
	UPaperZDAnimBPGeneratedClass* AnimClass = Cast<UPaperZDAnimBPGeneratedClass>(GetClass());
	if (AnimClass)
	{
		FPaperZDAnimationBaseContext Context(this);
		for (FPaperZDAnimNode_StateMachine* StateMachineNode : AnimClass->GetStateMachineNodes(this))
		{
			if (StateMachineName != NAME_None && StateMachineNode->GetMachineName() == StateMachineName)
			{
				bJumped = StateMachineNode->JumpToNode(JumpName, Context);
				break;
			}
			else if (StateMachineName == NAME_None)
			{ 
				bJumped |= StateMachineNode->JumpToNode(JumpName, Context);
			}
		}
	}

	return bJumped;
}

void UPaperZDAnimInstance::ProcessAnimations(float DeltaTime, bool bOnlyProcessPendingJumps /* = false */)
{
	if (!RootNode || bIsProcessingAnimations)
	{
		return;
	}

	TGuardValue<bool> ProcessingGuard(bIsProcessingAnimations, true);

	//Handle any request left by a callback before running the regular animation update.
	FlushPendingJumps();

	if (!bOnlyProcessPendingJumps)
	{
		RunAnimationPass(DeltaTime, false);

		//A notify, sequence-changed callback, or state event may have requested a jump during the pass.
		FlushPendingJumps();
	}
}

void UPaperZDAnimInstance::RunAnimationPass(float DeltaTime, bool bIsJumpUpdate)
{
	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateAnimGraph);

		//Update any animation override first
		UpdateAnimationOverrides(DeltaTime, bIsJumpUpdate);

		//First do a pass and update any animation node
		FPaperZDAnimationUpdateContext UpdateContext(this, DeltaTime);
		UpdateContext.GraphUpdateId = ++AnimationUpdateCounter;
		UpdateContext.bIsJumpUpdate = bIsJumpUpdate;
		RootNode->Update(UpdateContext);

		//We now clear the processed animation override data for next tick
		ProcessedOverrideData.Empty(AnimationOverrideHandles.Num());
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_RenderAnimations);

		//Then evaluate the sink node, obtaining the final animation data
		FPaperZDAnimationPlaybackData PlaybackData;
		RootNode->Evaluate(PlaybackData);

		//Pass to the AnimPlayer. Jump passes collect no notifies, so notify reconciliation must be skipped
		//or every active notify state would be aborted even though its animation is still playing.
		AnimPlayer->Play(PlaybackData, !bIsJumpUpdate);
	}
}

void UPaperZDAnimInstance::FlushPendingJumps()
{
	//A state event or sequence-changed callback can request another jump while a request is being
	//committed. Bound the loop so a Blueprint callback cycle cannot lock the game thread forever.
	static constexpr int32 MaxJumpFlushIterations = 16;
	bool bNeedsJumpUpdate = false;

	for (int32 Iteration = 0; Iteration < MaxJumpFlushIterations; ++Iteration)
	{
		if (PendingJumpRequests.Num() > 0)
		{
			TArray<FPendingJumpRequest> Requests = MoveTemp(PendingJumpRequests);
			for (const FPendingJumpRequest& Request : Requests)
			{
				bNeedsJumpUpdate |= ApplyJumpToNode(Request.JumpName, Request.StateMachineName);
			}

			//Apply requests queued by state callbacks before rendering an intermediate state.
			continue;
		}

		if (bNeedsJumpUpdate)
		{
			bNeedsJumpUpdate = false;
			RunAnimationPass(0.0f, true);
			continue;
		}

		return;
	}

	//Render the last state that was successfully applied, but discard any callback cycle that
	//continued past the safety bound.
	if (bNeedsJumpUpdate)
	{
		RunAnimationPass(0.0f, true);
	}

	if (PendingJumpRequests.Num() > 0)
	{
		ensureMsgf(false, TEXT("PaperZD stopped a recursive JumpToNode callback cycle after %d iterations."), MaxJumpFlushIterations);
		PendingJumpRequests.Reset();
	}
}

void UPaperZDAnimInstance::UpdateAnimationOverrides(float DeltaTime, bool bIsJumpUpdate /* = false */)
{
	for (int32 i = AnimationOverrideHandles.Num() - 1; i >= 0; i--)
	{
		FAnimationOverrideHandle& Handle = AnimationOverrideHandles[i];

		//Jump passes run with zero delta time and only exist to commit an explicitly requested state.
		//Re-publish the slot data so override slots keep rendering their animation, but do not advance playback,
		//trigger notifies, nor complete overrides: completion callbacks should never fire from inside JumpToNode.
		if (bIsJumpUpdate)
		{
			if (Handle.AnimSequencePtr.IsValid())
			{
				FPaperZDAnimationPlaybackData PlaybackData;
				PlaybackData.SetAnimation(Handle.AnimSequencePtr.Get(), Handle.PlaybackTime);
				SetAnimationOverrideDataBySlot(Handle.SlotName, PlaybackData);
			}
			continue;
		}

		AnimPlayer->TickPlayback(Handle.AnimSequencePtr.Get(), Handle.PlaybackTime, DeltaTime * Handle.PlayRate, false, this, 1.0f);

		//It could happen that after we tick playback of the AnimSequence, the AnimationOverrideHandles array changed somehow,
		//either by removal of any of the Handles (by stopping an override) or by adding another handle on top (which would change the memory address of the array).
		//Because of this we need to re-validate the handle before going further.
		if (AnimationOverrideHandles.IsValidIndex(i))
		{
			//Re-grab the handle to make sure we aren't pointing to stale data.
			Handle = AnimationOverrideHandles[i];

			//Check if the override completed
			if ((Handle.PlayRate > 0.0f && Handle.PlaybackTime >= Handle.AnimSequencePtr->GetTotalDuration())
				|| (Handle.PlayRate < 0.0f && Handle.PlaybackTime <= 0.0f))
			{
				//If we call the delegate right away it might happen that the user triggers another animation callback on the same slot/group,
				//the system will then incorrectly assume that a "cancel" should be triggered as the OverrideHandle hasn't been unregistered yet.
				//Because of this we need to make sure the handle has been removed before we trigger the callback.
				// 
				//We thus make a copy of the delegate object before removing the animation override.
				FZDOnAnimationOverrideEndSignature OnOverrideEnd_Copy = Handle.OnOverrideEnd;

				//Remove from the array, so the handle gets correctly unregistered
				AnimationOverrideHandles.RemoveAt(i);

				//Now call the delegate
				OnOverrideEnd_Copy.ExecuteIfBound(true);
			}
			else
			{
				//The override is still valid and we should pass it to the array of processed ones for slots to grab
				FPaperZDAnimationPlaybackData PlaybackData;
				PlaybackData.SetAnimation(Handle.AnimSequencePtr.Get(), Handle.PlaybackTime);
				SetAnimationOverrideDataBySlot(Handle.SlotName, PlaybackData);
			}
		}
		else
		{
			//The data we were iterating is stale, it's safer to skip the updates until the next tick
			break;
		}
	}
}

float UPaperZDAnimInstance::GetDeltaTimeIgnoredDilation(float DeltaTime)
{
	const float timeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
	return DeltaTime / timeDilation;
}

void UPaperZDAnimInstance::PrepareForMovieSequence()
{
	bSequencerOverride = true;
}

void UPaperZDAnimInstance::RestorePreMovieSequenceState()
{
	bSequencerOverride = false;
}

void UPaperZDAnimInstance::UpdateSequencerPreview()
{
	if (bSequencerOverride)
	{
		//We do a 'virtual tick', which won't update anything but force a new evaluation and updates of the animation nodes.
		ProcessAnimations(0.0f);
	}
}

bool UPaperZDAnimInstance::PlayAnimationOverride(const UPaperZDAnimSequence* AnimSequence, FName SlotName /* = "DefaultSlot" */, float PlayRate /* = 1.0f */, float StartingPosition /* = 0.0f */, FZDOnAnimationOverrideEndSignature OnOverrideEnd /* = FZDOnAnimationOverrideEndSignature() */)
{
	//First make sure the slot actually exists
	UPaperZDAnimBPGeneratedClass* AnimClass = Cast<UPaperZDAnimBPGeneratedClass>(GetClass());
	if (AnimClass && AnimSequence)
	{
		FPaperZDOverrideSlotDescriptor SlotDescriptor;
		if (!AnimClass->GetOverrideSlotDescriptor(SlotName, SlotDescriptor))
		{
			//Slot doesn't exist
			FMessageLog PIELogger = FMessageLog(FName("PIE"));
			PIELogger.Warning(FText::Format(LOCTEXT("InvalidOverrideSlot", "Tried to play animation override on inexistent slot '{0}'."), FText::FromName(SlotName)));
			return false;
		}

		//Find if there's already another slot on the same group being played (in which case it should be overridden)
		FAnimationOverrideHandle* pHandle = AnimationOverrideHandles.FindByPredicate([&SlotDescriptor](const FAnimationOverrideHandle& Handle) -> bool { return Handle.GroupName == SlotDescriptor.GroupName; });
		if (pHandle)
		{
			//Send the "cancel" message
			pHandle->OnOverrideEnd.ExecuteIfBound(false);
		}
		else
		{
			pHandle = &AnimationOverrideHandles.AddDefaulted_GetRef();
		}

		//Initialize the handle
		const float StartTime = PlayRate > 0.0f ? StartingPosition : AnimSequence->GetTotalDuration() - StartingPosition;
		pHandle->AnimSequencePtr = AnimSequence;
		pHandle->SlotName = SlotDescriptor.SlotName;
		pHandle->GroupName = SlotDescriptor.GroupName;
		pHandle->PlaybackTime = FMath::Clamp(StartTime, 0.0f, AnimSequence->GetTotalDuration());
		pHandle->PlayRate = PlayRate;
		pHandle->OnOverrideEnd = OnOverrideEnd;

		return true;
	}

	return false;
}

void UPaperZDAnimInstance::K2_PlayAnimationOverride(const UPaperZDAnimSequence* AnimSequence, float& AnimationLength, FName SlotName /* = "DefaultSlot" */, float PlayRate /* = 1.0f */, float StartingPosition /* = 0.0f */)
{
	if (AnimSequence)
	{
		PlayAnimationOverride(AnimSequence, SlotName, PlayRate, StartingPosition);
		AnimationLength = AnimSequence->GetTotalDuration();
	}
}

bool UPaperZDAnimInstance::GetAnimationOverrideDataBySlot(FName SlotName, FPaperZDAnimationPlaybackData& OutPlaybackData) const
{
	const FProcessedAnimationOverrideData* pHandle = ProcessedOverrideData.FindByPredicate([SlotName](const FProcessedAnimationOverrideData& Handle) -> bool { return Handle.SlotName == SlotName; });
	if (pHandle)
	{
		OutPlaybackData = pHandle->PlaybackData;
	}

	return pHandle != nullptr;
}

void UPaperZDAnimInstance::SetAnimationOverrideDataBySlot(FName SlotName, const FPaperZDAnimationPlaybackData& PlaybackData, bool bOverwriteExisting /* = false */)
{
	FProcessedAnimationOverrideData* pHandle = ProcessedOverrideData.FindByPredicate([SlotName](const FProcessedAnimationOverrideData& Handle) -> bool { return Handle.SlotName == SlotName; });
	if (pHandle && bOverwriteExisting)
	{
		pHandle->PlaybackData = PlaybackData;
	}
	else
	{
		FProcessedAnimationOverrideData Data;
		Data.SlotName = SlotName;
		Data.PlaybackData = PlaybackData;
		ProcessedOverrideData.Add(Data);
	}
}

void UPaperZDAnimInstance::StopAnimationOverrideByGroup(FName GroupToStop)
{
	for (int32 i = AnimationOverrideHandles.Num() - 1; i >= 0; i--)
	{
		const FAnimationOverrideHandle& Handle = AnimationOverrideHandles[i];
		if (Handle.GroupName == GroupToStop)
		{
			//Cancel the override. Need to make a copy of the delegate so it triggers after it unregisters.
			FZDOnAnimationOverrideEndSignature OnOverrideEnd_Copy = Handle.OnOverrideEnd;
			AnimationOverrideHandles.RemoveAt(i);
			OnOverrideEnd_Copy.ExecuteIfBound(false);
			break;
		}
	}
}

void UPaperZDAnimInstance::StopAllAnimationOverrides()
{
	//Because we want to avoid stopping any animation override added by the callbacks we might trigger, we work on a temporal copy
	TArray<FAnimationOverrideHandle> HandleCopies = MoveTemp(AnimationOverrideHandles);
	for (int32 i = HandleCopies.Num() - 1; i >= 0; i--)
	{
		const FAnimationOverrideHandle& Handle = HandleCopies[i];
		Handle.OnOverrideEnd.ExecuteIfBound(false);
	}
}

float UPaperZDAnimInstance::GetInstanceAssetPlayerLength(int32 AssetPlayerIndex)
{
	UPaperZDAnimBPGeneratedClass* AnimClass = Cast<UPaperZDAnimBPGeneratedClass>(GetClass());
	if (AnimClass)
	{
		FPaperZDAnimNode_PlaySequence* AssetPlayerNode = AnimClass->GetAnimNodeByPropertyIndex<FPaperZDAnimNode_PlaySequence>(this, AssetPlayerIndex);
		if (AssetPlayerNode && AssetPlayerNode->GetAnimSequence())
		{
			return AssetPlayerNode->GetAnimSequence()->GetTotalDuration();
		}
	}

	return 0.0f;
}

float UPaperZDAnimInstance::GetInstanceAssetPlayerTime(int32 AssetPlayerIndex)
{
	UPaperZDAnimBPGeneratedClass* AnimClass = Cast<UPaperZDAnimBPGeneratedClass>(GetClass());
	if (AnimClass)
	{
		FPaperZDAnimNode_PlaySequence* AssetPlayerNode = AnimClass->GetAnimNodeByPropertyIndex<FPaperZDAnimNode_PlaySequence>(this, AssetPlayerIndex);
		if (AssetPlayerNode)
		{
			return AssetPlayerNode->GetPlaybackTime();
		}
	}

	return 0.0f;
}

float UPaperZDAnimInstance::GetInstanceAssetPlayerTimeFraction(int32 AssetPlayerIndex)
{
	UPaperZDAnimBPGeneratedClass* AnimClass = Cast<UPaperZDAnimBPGeneratedClass>(GetClass());
	if (AnimClass)
	{
		FPaperZDAnimNode_PlaySequence* AssetPlayerNode = AnimClass->GetAnimNodeByPropertyIndex<FPaperZDAnimNode_PlaySequence>(this, AssetPlayerIndex);
		if (AssetPlayerNode && AssetPlayerNode->GetAnimSequence() && AssetPlayerNode->GetAnimSequence()->GetTotalDuration() > 0.0f)
		{
			return AssetPlayerNode->PlaybackTime / AssetPlayerNode->GetAnimSequence()->GetTotalDuration();
		}
	}

	return 0.0f;
}

float UPaperZDAnimInstance::GetInstanceAssetPlayerTimeFromEnd(int32 AssetPlayerIndex)
{
	UPaperZDAnimBPGeneratedClass* AnimClass = Cast<UPaperZDAnimBPGeneratedClass>(GetClass());
	if (AnimClass)
	{
		FPaperZDAnimNode_PlaySequence* AssetPlayerNode = AnimClass->GetAnimNodeByPropertyIndex<FPaperZDAnimNode_PlaySequence>(this, AssetPlayerIndex);
		if (AssetPlayerNode && AssetPlayerNode->GetAnimSequence() && AssetPlayerNode->GetAnimSequence()->GetTotalDuration() > 0.0f)
		{
			return AssetPlayerNode->GetAnimSequence()->GetTotalDuration() - AssetPlayerNode->PlaybackTime;
		}
	}

	return 0.0f;
}

float UPaperZDAnimInstance::GetInstanceAssetPlayerTimeFromEndFraction(int32 AssetPlayerIndex)
{
	UPaperZDAnimBPGeneratedClass* AnimClass = Cast<UPaperZDAnimBPGeneratedClass>(GetClass());
	if (AnimClass)
	{
		FPaperZDAnimNode_PlaySequence* AssetPlayerNode = AnimClass->GetAnimNodeByPropertyIndex<FPaperZDAnimNode_PlaySequence>(this, AssetPlayerIndex);
		if (AssetPlayerNode && AssetPlayerNode->GetAnimSequence() && AssetPlayerNode->GetAnimSequence()->GetTotalDuration() > 0.0f)
		{
			return 1.0f - AssetPlayerNode->PlaybackTime / AssetPlayerNode->GetAnimSequence()->GetTotalDuration();
		}
	}

	return 0.0f;
}

#undef LOCTEXT_NAMESPACE
