// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "AnimNodes/PaperZDAnimNode_RandomPlayer.h"
#include "AnimSequences/Players/PaperZDAnimPlayer.h"
#include "PaperZDAnimInstance.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimNode_RandomPlayer)
#endif

FPaperZDAnimNode_RandomPlayer::FPaperZDAnimNode_RandomPlayer()
	: bShuffleMode(false)
	, PlaybackTime(0.0f)
	, CurrentEntryIdx(INDEX_NONE)
	, RemainingLoops(0)
	, PlayRate(1.0f)
	, ShuffleIndex(INDEX_NONE)
	, AggregatedChance(0.0f)
{}

void FPaperZDAnimNode_RandomPlayer::OnInitialize(const FPaperZDAnimationInitContext& InitContext)
{
	if (Entries.Num() > 0)
	{
		//Select the first animation to play
		GenerateOrderedList();
		PickNextEntry();
	}
}

void FPaperZDAnimNode_RandomPlayer::OnUpdate(const FPaperZDAnimationUpdateContext& UpdateContext)
{
	if (Entries.IsValidIndex(CurrentEntryIdx) && PlayRate != 0.0f)
	{
		//Independent of the weight we have, we should update the playback, to avoid losing sync
		const float PreviousTime = PlaybackTime;
 		UPaperZDAnimPlayer* Player = UpdateContext.AnimInstance->GetPlayer();
		Player->TickPlayback(Entries[CurrentEntryIdx].AnimSequence, PlaybackTime, UpdateContext.DeltaTime * PlayRate, true, UpdateContext.AnimInstance, UpdateContext.Weight, UpdateContext.bIsJumpUpdate);

		//Check if we have looped yet
		const bool bLoopComplete = PlayRate > 0.0f ? PreviousTime > PlaybackTime : PreviousTime < PlaybackTime;
		RemainingLoops = bLoopComplete ? RemainingLoops - 1 : RemainingLoops;
		if (RemainingLoops < 0)
		{
			//We finished with this entry, jump to the next one
			PickNextEntry();
		}
	}
}

void FPaperZDAnimNode_RandomPlayer::OnEvaluate(FPaperZDAnimationPlaybackData& OutData)
{
	if (Entries.IsValidIndex(CurrentEntryIdx))
	{
		//Forcefully add the animation as the only present
		OutData.SetAnimation(Entries[CurrentEntryIdx].AnimSequence, PlaybackTime);
	}
}

void FPaperZDAnimNode_RandomPlayer::GenerateOrderedList()
{
	//First build the indices
	AggregatedChance = 0.0f;
	OrderedList.Empty(Entries.Num());
	for (int32 i = 0; i < Entries.Num(); i++)
	{
		OrderedList.Add(i);
		AggregatedChance += Entries[i].ChanceToPlay;
	}

	//What type of order is inside the list depends on the play mode
	if (bShuffleMode)
	{
		//Simply shuffle the indices
		const int32 LastIndex = OrderedList.Num() - 1;
		for (int32 i = 0; i <= LastIndex; i++)
		{
			const int32 Index = FMath::RandRange(i, LastIndex);
			if (i != Index)
			{
				OrderedList.Swap(i, Index);
			}
		}
	}
	else
	{
		//We actually need to sort the array depending on the entries chance to play
		OrderedList.Sort([this](const int32& LH, const int32& RH) -> bool
		{
			return Entries[LH].ChanceToPlay > Entries[RH].ChanceToPlay;
		});
	}
}

void FPaperZDAnimNode_RandomPlayer::PickNextEntry()
{
	//First choose the entry itself, how to choose it depends if we're on shuffle mode or not
	if (bShuffleMode)
	{
		ShuffleIndex = (ShuffleIndex + 1) % OrderedList.Num();
		CurrentEntryIdx = OrderedList[ShuffleIndex];
	}
	else
	{
		//Select a random number that falls in between the universe of "chance" we have
		float RandResult = FMath::RandRange(0.0f, AggregatedChance);

		//Then search which sample got the big price
		float CurrentChance = 0.0f;
		for (int32 i = 0; i < OrderedList.Num(); i++)
		{
			CurrentEntryIdx = OrderedList[i];
			CurrentChance += Entries[CurrentEntryIdx].ChanceToPlay;

			//Done if we surpassed the picked value
			if (CurrentChance >= RandResult)
			{
				break;
			}
		}
	}

	//Initialize the entry itself
	const FPaperZDRandomPlayerEntry& Entry = Entries[CurrentEntryIdx];
	RemainingLoops = FMath::RandRange(Entry.MinLoopCount, Entry.MaxLoopCount);
	PlayRate = FMath::RandRange(Entry.MinPlayRate, Entry.MaxPlayRate);

	//Make sure we accidentally don't choose a play rate of 0 if we can avoid it (send it to one of the extremes in this case)
	if (PlayRate == 0.0f)
	{
		PlayRate = Entry.MinPlayRate != 0.0f ? Entry.MinPlayRate : Entry.MaxPlayRate;
	}
	
	//Initialize the starting time
	const float SeqDuration = Entry.AnimSequence->GetTotalDuration();
	PlaybackTime = PlayRate < 0.0f ? SeqDuration : 0.0f;
}
