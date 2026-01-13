// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "AnimSequences/PaperZDAnimSequence.h"
#include "Notifies/PaperZDAnimNotify.h"
#include "Notifies/PaperZDAnimNotifyCustom.h"
#include "Components/PrimitiveComponent.h"
#include "IPaperZDEditorProxy.h"
#include "PaperZDCustomVersion.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimSequence)
#endif

//Setup static variables
const FName UPaperZDAnimSequence::DefaultCategory(TEXT("Default"));
const FName UPaperZDAnimSequence::AnimSourceMemberName(TEXT("AnimSource"));

#define MAX_NUM_TRACKS 10

//List of optional metadata specifiers
namespace FPaperZDAnimSequenceDefaults
{
	FName AnimDataSource = TEXT("AnimData");
}

UPaperZDAnimSequence::UPaperZDAnimSequence(const FObjectInitializer& ObjectInitializer)
	: Super()
	, CachedAnimDataSourceProperty(nullptr)
	, bDirectionalSequence(false)
	, DirectionalAngleOffset(0.0f)
	, DirectionalPreviewIndex(0)
{
#if WITH_EDITOR
	SetFlags(GetFlags() | RF_Transactional);
#endif
	Category = UPaperZDAnimSequence::DefaultCategory;
}

void UPaperZDAnimSequence::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	InitTracks();

	const int32 ZDVersion = GetLinkerCustomVersion(FPaperZDCustomVersion::GUID);
	if (ZDVersion < FPaperZDCustomVersion::AnimationSourceAdded)
	{
		IPaperZDEditorProxy::Get()->UpdateVersionToAnimationSourceAdded(this);
	}
#endif
}

void UPaperZDAnimSequence::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar.UsingCustomVersion(FPaperZDCustomVersion::GUID);
}

void UPaperZDAnimSequence::PostInitProperties()
{
	Super::PostInitProperties();

	//Initialize the data source, by caching the property pointer and making sure it holds at least one entry (the system assumes this)
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		InitDataSource();
	}
}

void UPaperZDAnimSequence::SetAnimSource(UPaperZDAnimationSource* InAnimSource)
{
	AnimSource = InAnimSource;
}

const TArray<UPaperZDAnimNotify_Base*>& UPaperZDAnimSequence::GetAnimNotifies() const
{
	return AnimNotifies;
}

FName UPaperZDAnimSequence::GetSequenceName() const
{
	return GetFName();
}

float UPaperZDAnimSequence::GetTotalDuration() const
{
	return 0.0f;
}

float UPaperZDAnimSequence::GetFramesPerSecond() const
{
	return 24.0f;
}

int32 UPaperZDAnimSequence::GetNumberOfFrames() const
{
	return FMath::CeilToInt(GetFramesPerSecond() * GetTotalDuration());
}

int32 UPaperZDAnimSequence::GetFrameAtTime(const float Time) const
{
	const int32 NumFrames = GetNumberOfFrames();
	const int32 Frame = NumFrames > 0 ? FMath::RoundToInt(Time * GetFramesPerSecond()) : 0;
	return FMath::Clamp(Frame, 0, NumFrames);
}

float UPaperZDAnimSequence::GetTimeAtFrame(const int32 Frame) const
{
	const int32 NumFrames = GetNumberOfFrames();
	const float Time = NumFrames > 0 ? Frame / GetFramesPerSecond() : 0.0f;
	return FMath::Clamp(Time, 0.0f, GetTotalDuration());
}

FArrayProperty* UPaperZDAnimSequence::GetAnimDataSourceProperty() const
{
	return CachedAnimDataSourceProperty;
}

FName UPaperZDAnimSequence::GetDataSourcePropertyName() const
{
	//Can be overridden, should use GET_MEMBER_NAME_CHECKED to ensure that the name is correct
	return FPaperZDAnimSequenceDefaults::AnimDataSource;
}

void UPaperZDAnimSequence::InitDataSource()
{
	//Refresh the anim data source
	CachedAnimDataSourceProperty = FindFProperty<FArrayProperty>(GetClass(), *GetDataSourcePropertyName().ToString());

	//If we do have a valid data source, we need to make sure its initial state is valid as well
	if (CachedAnimDataSourceProperty)
	{
		FScriptArrayHelper ArrayHelper(CachedAnimDataSourceProperty, CachedAnimDataSourceProperty->ContainerPtrToValuePtr<uint8>(this));

		//If we have no entries, this sequence was just created and we need to make sure its initial state is valid
		if (ArrayHelper.Num() == 0)
		{
			ArrayHelper.AddValue();
			bDirectionalSequence = false;
		}
	}
}

#if WITH_EDITOR
void UPaperZDAnimSequence::PostEditUndo()
{
	Super::PostEditUndo();

	//@TODO: we can get away with removing this by adding a listener on the editor window instead
	OnPostEditUndo.ExecuteIfBound();
}

int32 UPaperZDAnimSequence::CreateTrack(int32 InsertInto /* = INDEX_NONE */)
{
	//Create the track metadata
	int32 NewIndex = InsertInto;
	if (InsertInto != INDEX_NONE)
	{
		AnimTrackData.InsertDefaulted(InsertInto);
	}
	else
	{
		NewIndex = AnimTrackData.AddDefaulted();
	}

	//Should have a valid index now
	check(NewIndex != INDEX_NONE);

	//Push up any notify that should make room for the new track
	for (UPaperZDAnimNotify_Base* Notify : AnimNotifies)
	{
		if (Notify->TrackIndex >= NewIndex)
		{
			Notify->TrackIndex++;
		}
	}

	return NewIndex;
}

void UPaperZDAnimSequence::RemoveTrack(int32 TrackIndex)
{
	check(AnimTrackData.Num() > TrackIndex);
	
	for (int32 i = AnimNotifies.Num() - 1; i >= 0; i--)
	{
		UPaperZDAnimNotify_Base* Notify = AnimNotifies[i];
		if (Notify->TrackIndex == TrackIndex)
		{
			//Delete any notify that lives on the deleted track.
			AnimNotifies.RemoveAt(i);
		}
		else if (Notify->TrackIndex > TrackIndex)
		{
			//Collapse the tracks that are on top of the deleted one, as we're gonna shrink the track array
			Notify->Modify();
			Notify->TrackIndex--;
		}
	}

	//Finally remove the track metadata.
	AnimTrackData.RemoveAt(TrackIndex);
}

void UPaperZDAnimSequence::AddNotifyToTrack(TSubclassOf<UPaperZDAnimNotify_Base> NotifyClass, int32 TrackIndex, FName NotifyName, float InitTime)
{
	check(AnimTrackData.Num() > TrackIndex);

	UPaperZDAnimNotify_Base* NewNotify = NewObject<UPaperZDAnimNotify_Base>(this, NotifyClass, NAME_None, RF_Transactional);
	NewNotify->Time = InitTime;
	NewNotify->Name = NotifyName;
	NewNotify->TrackIndex = TrackIndex;

	AnimNotifies.Add(NewNotify);
	OnNotifyChange.Broadcast();
}

void UPaperZDAnimSequence::RemoveNotify(UPaperZDAnimNotify_Base* Notify)
{
	AnimNotifies.Remove(Notify);
	OnNotifyChange.Broadcast();
}

void UPaperZDAnimSequence::MoveNotify(UPaperZDAnimNotify_Base* Notify, int32 ToTrack)
{
	//Mark as modified, so we can UNDO this if needed.
	check(Notify);
	Notify->Modify();
	Notify->TrackIndex = ToTrack;
	OnNotifyChange.Broadcast();
}

void UPaperZDAnimSequence::ImportNotifies(TArray<UPaperZDAnimNotify_Base*> InNotifies)
{
	//Start adding the notifies onto the target array, making sure all the notifies are correctly setup
	for (UPaperZDAnimNotify_Base* Notify : InNotifies)
	{
		if (Notify->GetOuter() == this)
		{
			AnimNotifies.Add(Notify);
		}
	}

	OnNotifyChange.Broadcast();
}

int32 UPaperZDAnimSequence::GetNumTracks() const
{
	return AnimTrackData.Num();
}

bool UPaperZDAnimSequence::GetTrackMetadata(int32 Index, FPaperZDAnimTrackMetadata& OutMetadata) const
{
	bool bFound = false;
	if (AnimTrackData.IsValidIndex(Index))
	{
		OutMetadata = AnimTrackData[Index];
		bFound = true;
	}

	return bFound;
}

const FPaperZDAnimTrackMetadata& UPaperZDAnimSequence::GetTrackMetadataChecked(int32 Index) const
{
	check(AnimTrackData.IsValidIndex(Index));
	return AnimTrackData[Index];
}

FPaperZDAnimTrackMetadata& UPaperZDAnimSequence::GetMutableTrackMetadataChecked(int32 Index)
{
	check(AnimTrackData.IsValidIndex(Index));
	return AnimTrackData[Index];
}

bool UPaperZDAnimSequence::GetTrack(int32 Index, FPaperZDAnimTrack& OutTrack) const
{
	bool bFound = false;
	if (AnimTrackData.IsValidIndex(Index))
	{
		OutTrack = GetTrackChecked(Index);
		bFound = true;
	}

	return bFound;
}

FPaperZDAnimTrack UPaperZDAnimSequence::GetTrackChecked(int32 Index) const
{
	check(AnimTrackData.IsValidIndex(Index));

	//Obtain the notifies that belong to the track
	TArray<UPaperZDAnimNotify_Base*> TrackNotifies;
	for (UPaperZDAnimNotify_Base* Notify : AnimNotifies)
	{
		//Clamp the notify track if needed
		int32 TrackIndex = FMath::Clamp(Notify->TrackIndex, 0, MAX_NUM_TRACKS - 1);
		if (TrackIndex == Index)
		{
			TrackNotifies.Add(Notify);
		}
	}

	//Create the track
	return FPaperZDAnimTrack(TrackNotifies, AnimTrackData[Index]);
}

void UPaperZDAnimSequence::InitTracks()
{
	//Filter the tracks that are invalid due to load, or were marked as deprecated (those get invalidated on next load, but its better if we remove them now)
	AnimNotifies = AnimNotifies.FilterByPredicate([](const UPaperZDAnimNotify_Base* Notify)
	{
		return Notify && !Notify->GetClass()->HasAnyClassFlags(CLASS_Deprecated);
	});

	//Make sure we have enough tracks for every notify that we're storing.
	int32 RequiredTrackNum = 1;
	for (const UPaperZDAnimNotify_Base* Notify : AnimNotifies)
	{
		if (Notify->TrackIndex >= RequiredTrackNum)
		{
			RequiredTrackNum = FMath::Clamp(Notify->TrackIndex + 1, 0, MAX_NUM_TRACKS);
		}
	}

	//Create any tracks that we need, do it without the CreateTrack method, otherwise we would push every notify up
	const int32 TracksToCreate = RequiredTrackNum - AnimTrackData.Num();
	for (int32 i = 0; i < TracksToCreate; i++)
	{
		FPaperZDAnimTrackMetadata& CreatedTrack = AnimTrackData.AddDefaulted_GetRef();
		CreatedTrack.DisplayName = *FString::FromInt(AnimTrackData.Num());
	}
}

void UPaperZDAnimSequence::PurgeInvalidCustomNotifies(const TArray<FName>& ValidRegisteredNames)
{
	//Make sure any of the custom notifies that exist are linked to a valid custom notify function
	for (int32 i = AnimNotifies.Num() - 1; i >= 0; i--)
	{
		UPaperZDAnimNotifyCustom* CustomNotify = Cast<UPaperZDAnimNotifyCustom>(AnimNotifies[i]);
		if (CustomNotify && !ValidRegisteredNames.Contains(CustomNotify->Name))
		{
			AnimNotifies.RemoveAt(i);
		}
	}
}

#endif
